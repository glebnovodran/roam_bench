#include "crosscore.hpp"
#include "scene.hpp"
#include "demo.hpp"
#include "smprig.hpp"
#include "smpchar.hpp"
#include "pint.hpp"

DEMO_PROG_BEGIN

static cxStopWatch s_execStopWatch;
static float s_medianExecMillis = -1.0f;
static int s_exerep = 1;

enum class RoamProgKind {
	NATIVE,
	PINT,
	PLOP
};

enum class BindIdx {
	EXECONTEXT = 0,
	FUNCLIB = 1
};

static RoamProgKind s_roamProgKind = RoamProgKind::NATIVE;

static struct CharPintProg {
	const char* pActName;
	const char* pProgName;
	char* pSrc;
	size_t srcSize;
} s_chrProgs[] = {
	{ "ACT_STAND", "ACT_STAND", nullptr, 0 },
	{ "ACT_TURN_L", "ACT_TURN", nullptr, 0 },
	{ "ACT_TURN_R", "ACT_TURN", nullptr, 0 },
	{ "ACT_WALK", "ACT_WALK", nullptr, 0 },
	{ "ACT_RETREAT", "ACT_RETREAT",nullptr, 0 },
	{ "ACT_RUN", "ACT_RUN", nullptr, 0 }
};

static char* load_prog(const char* pName, size_t& srcSize) {
	char* pProg = nullptr;
	cxResourceManager* pRsrcMgr = Scene::get_rsrc_mgr();
	bool res = false;
	const char* pDataPath = pRsrcMgr ? pRsrcMgr->get_data_path() : nullptr;
	if (pName) {
		char path[256];
		char* pSrcPath = path;
		size_t bufSize = sizeof(path);
		size_t pathSize = (pDataPath ? nxCore::str_len(pDataPath) : 1) + 6 + nxCore::str_len(pName) + 5 + 1;
		if (bufSize < pathSize) {
			pSrcPath = reinterpret_cast<char*>(nxCore::mem_alloc(pathSize, "chr_prog_path"));
			bufSize = pathSize;
		}

		XD_SPRINTF(XD_SPRINTF_BUF(pSrcPath, bufSize), "%s/%s/%s.pint", pDataPath ? pDataPath : ".", "acts", pName);
		srcSize = 0;
		pProg = reinterpret_cast<char*>(nxCore::raw_bin_load(pSrcPath, &srcSize));
		if (pSrcPath != path) {
			nxCore::mem_free(pSrcPath);
		}
		res = true;
	}
	return pProg;
}

static void free_pint_progs() {
	size_t nprogs = XD_ARY_LEN(s_chrProgs);
	for (int i = 0; i < nprogs; ++i) {
		char* pSrc = s_chrProgs[i].pSrc;
		if (pSrc) {
			nxCore::bin_unload(pSrc);
			pSrc = nullptr;
		}
	}
}

static void load_pint_progs() {
	size_t nprogs = XD_ARY_LEN(s_chrProgs);
	for (int i = 0; i < nprogs; ++i) {
		char* pProg = load_prog(s_chrProgs[i].pProgName, s_chrProgs[i].srcSize);
		if (pProg) {
			s_chrProgs[i].pSrc = pProg;
		} else {
			nxCore::dbg_msg("Couldn't load %s character program\n", s_chrProgs[i].pProgName);
		}
	}
}

static struct STAGE {
	Pkg* pPkg;
	sxCollisionData* pCol;
} s_stage = {};

static void add_stg_obj(sxModelData* pMdl, void* pWkData) {
	ScnObj* pObj = Scene::add_obj(pMdl);
	if (pObj) {
		pObj->set_base_color_scl(1.0f);
	}
}

static void init_stage() {
	Pkg* pPkg = Scene::load_pkg("slope_room");
	s_stage.pPkg = pPkg;
	s_stage.pCol = nullptr;
	if (pPkg) {
		Scene::for_all_pkg_models(pPkg, add_stg_obj, &s_stage);
		SmpCharSys::set_collision(pPkg->find_collision("col"));
	}
}

Pint::Value scn_rng_next(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	uint64_t rnd = Scene::glb_rng_next();
	rnd &= 0xffffffff;
	res.set_num(double(rnd));
	return res;
}
static const Pint::FuncDef s_df_rng_next_desc = {
	"glb_rng_next", scn_rng_next, 0, Pint::Value::Type::NUM, {Pint::Value::Type::NUM}
};

Pint::Value scn_rng_01(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	float rnd = Scene::glb_rng_f01();
	res.set_num(double(rnd));
	return res;
}
static const Pint::FuncDef s_df_rng_01_desc = {
	"glb_rng_01", scn_rng_01, 0, Pint::Value::Type::NUM, {Pint::Value::Type::NUM}
};

Pint::Value ck_act_timeout(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	SmpChar* pChar = reinterpret_cast<SmpChar*>(ctx.get_local_binding());
	bool isTimeOut = pChar->check_act_time_out();
	res.set_num(isTimeOut);
	return res;
}

static const Pint::FuncDef s_df_ck_act_timeout_desc = {
	"check_act_timeout", ck_act_timeout, 0, Pint::Value::Type::NUM, {Pint::Value::Type::NUM}
};

Pint::Value get_obj_touch_duration_secs(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	SmpChar* pChar = reinterpret_cast<SmpChar*>(ctx.get_local_binding());
	res.set_num(pChar->get_obj_touch_duration_secs());
	return res;
}

static const Pint::FuncDef s_df_get_obj_touch_duration_secs_desc = {
	"get_obj_touch_duration_secs", get_obj_touch_duration_secs, 0, Pint::Value::Type::NUM, {Pint::Value::Type::NUM}
};

Pint::Value get_wall_touch_duration_secs(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	SmpChar* pChar = reinterpret_cast<SmpChar*>(ctx.get_local_binding());
	res.set_num(pChar->get_wall_touch_duration_secs());
	return res;
}

static const Pint::FuncDef s_df_get_wall_touch_duration_secs_desc = {
	"get_wall_touch_duration_secs", get_wall_touch_duration_secs, 0, Pint::Value::Type::NUM, {Pint::Value::Type::NUM}
};

Pint::Value math_fit(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	double val = pArgs[0].val.num;
	double oldMin = pArgs[1].val.num;
	double oldMax = pArgs[2].val.num;
	double newMin = pArgs[3].val.num;
	double newMax = pArgs[4].val.num;
	res.set_num(nxCalc::fit(val, oldMin, oldMax, newMin, newMax));
	return res;
}

static const Pint::FuncDef s_df_math_fit_desc = {
	"math_fit", math_fit, 5, Pint::Value::Type::NUM,
	{Pint::Value::Type::NUM, Pint::Value::Type::NUM, Pint::Value::Type::NUM, Pint::Value::Type::NUM, Pint::Value::Type::NUM}
};

static int find_pint_prog(const char* pActName, CharPintProg* pProg) {
	int idx = -1;
	size_t nacts = XD_ARY_LEN(s_chrProgs);
	for (int i = 0; i < nacts; ++i) {
		if (nxCore::str_eq(pActName, s_chrProgs[i].pActName)) {
			pProg = &s_chrProgs[i];
			idx = i;
			break;
		}
	}
	return idx;
}

static void char_pint_roam_ctrl(SmpChar* pChar) {
	if (!pChar) return;

	size_t nprogs = XD_ARY_LEN(s_chrProgs);

	if (pChar->mAction < nprogs) {
		const char* pSrc = s_chrProgs[pChar->mAction].pSrc;
		size_t srcSize = s_chrProgs[pChar->mAction].srcSize;
		Pint::ExecContext* pCtx = pChar->get_ptr_wk<Pint::ExecContext>(int(BindIdx::EXECONTEXT));
		Pint::FuncLibrary* pFuncLib = pChar->get_ptr_wk<Pint::FuncLibrary>(int(BindIdx::FUNCLIB));
		Pint::interp(pSrc, srcSize, pCtx, pFuncLib);
		pCtx->print_vars();
		Pint::Value* pNewActVal = pCtx->var_val(pCtx->find_var("newActName"));
		const char* pNewActName = pNewActVal ? pNewActVal->val.pStr : "";
		if (!nxCore::str_eq(pNewActName, "")) {
			CharPintProg* pProg = nullptr;
			int actId = find_pint_prog(pNewActName, pProg);
			if (actId >= 0) {
				double newActDuration = 0.0;
				Pint::Value* pDurationVal = pCtx->var_val(pCtx->find_var("newActDuration"));
				newActDuration = pDurationVal ? pDurationVal->val.num : 0.0;
				if (nxCore::str_eq(pNewActName, "TURN_L") || nxCore::str_eq(pNewActName, "TURN_R")) {
					nxCore::dbg_msg("Turning\n");
				}
				pChar->change_act(actId, newActDuration);
			}
		}

		Pint::Value* pWTResetVal = pCtx->var_val(pCtx->find_var("wallTouchReset"));
		bool wallTouchReset = pWTResetVal ? pWTResetVal->val.num == 1.0 : false;
		if (wallTouchReset) {
			pChar->reset_wall_touch();
		}
	}
}

static void char_roam_ctrl(SmpChar* pChar) {
	if (!pChar) return;
	double objTouchDT = pChar->get_obj_touch_duration_secs();
	double wallTouchDT = pChar->get_wall_touch_duration_secs();
	switch (pChar->mAction) {
		case SmpChar::ACT_STAND:
			if (pChar->check_act_time_out()) {
				if ((Scene::glb_rng_next() & 0x3F) < 0xF) {
					pChar->change_act(SmpChar::ACT_RUN, 2.0f);
				} else {
					pChar->change_act(SmpChar::ACT_WALK, 5.0f);
					pChar->reset_wall_touch();
				}
			}
			break;
		case SmpChar::ACT_WALK:
			if (pChar->check_act_time_out() || objTouchDT > 0.2 || wallTouchDT > 0.25) {
				if (objTouchDT > 0 && ((Scene::glb_rng_next() & 0x3F) < 0x1F)) {
					pChar->change_act(SmpChar::ACT_RETREAT, 0.5f);
				} else {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 0.5f, 2.0f);
					if (Scene::glb_rng_next() & 0x1) {
						pChar->change_act(SmpChar::ACT_TURN_L, t);
					} else {
						pChar->change_act(SmpChar::ACT_TURN_R, t);
					}
				}
			}
			break;
		case SmpChar::ACT_RUN:
			if (pChar->check_act_time_out() || wallTouchDT > 0.1 || objTouchDT > 0.1) {
				pChar->change_act(SmpChar::ACT_STAND, 2.0f);
			}
			break;
		case SmpChar::ACT_RETREAT:
			if (pChar->check_act_time_out() || wallTouchDT > 0.1) {
				pChar->change_act(SmpChar::ACT_STAND, 2.0f);
			}
			break;
		case SmpChar::ACT_TURN_L:
		case SmpChar::ACT_TURN_R:
			if (pChar->check_act_time_out()) {
				pChar->change_act(SmpChar::ACT_STAND, 1.0f);
			}
			break;
	}
}

static void init_single_char() {
	SmpChar::Descr descr;
	descr.reset();
	bool disableSl = nxApp::get_bool_opt("scl_off", false);

	SmpChar::CtrlFunc ctrlFunc = char_roam_ctrl;
	if (s_roamProgKind == RoamProgKind::PINT) {
		ctrlFunc = char_pint_roam_ctrl;
	}
	float x = -5.57f;
	ScnObj* pObj = SmpCharSys::add_f(descr, ctrlFunc);
	pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(x, 0.0f, 0.0f));
}

static void init_two_chars() {
	SmpChar::Descr descr;
	descr.reset();
	bool disableSl = nxApp::get_bool_opt("scl_off", false);

	SmpChar::CtrlFunc ctrlFunc = char_roam_ctrl;
	if (s_roamProgKind == RoamProgKind::PINT) {
		ctrlFunc = char_pint_roam_ctrl;
	}
	float x = -5.57f;
	ScnObj* pObj = SmpCharSys::add_f(descr, ctrlFunc);
	pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(x, 0.0f, 0.0f));
	x += 0.7f;
	pObj = SmpCharSys::add_m(descr, ctrlFunc);
	pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(x, 0.0f, 0.0f));
}

static void init_chars() {
	SmpChar::Descr descr;
	descr.reset();
	bool disableSl = nxApp::get_bool_opt("scl_off", false);

	SmpChar::CtrlFunc ctrlFunc = char_roam_ctrl;
	if (s_roamProgKind == RoamProgKind::PINT) {
		ctrlFunc = char_pint_roam_ctrl;
	}
	float x = -5.57f;
	for (int i = 0; i < 10; ++i) {
		descr.variation = i;
		if (disableSl) {
			descr.scale = 1.0f;
		} else {
			if (!(i & 1)) {
				descr.scale = 0.95f;
			} else {
				descr.scale = 1.0f;
			}
		}
		ScnObj* pObj = SmpCharSys::add_f(descr, ctrlFunc);
		pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(x, 0.0f, 0.0f));
		x += 0.7f;
	}

	cxVec pos(-4.6f, 0.0f, 7.0f);
	cxQuat q = nxQuat::from_degrees(0.0f, 110.0f, 0.0f);
	cxVec add = q.apply(cxVec(0.7f, 0.0f, 0.0f));
	for (int i = 0; i < 10; ++i) {
		descr.variation = i;
		if (disableSl) {
			descr.scale = 1.0f;
		} else {
			if (!(i & 1)) {
				descr.scale = 1.0f;
			} else {
				descr.scale = 1.04f;
			}
		}
		ScnObj* pObj = SmpCharSys::add_m(descr, ctrlFunc);
		if (pObj) {
			pObj->set_world_quat_pos(q, pos);
		}
		pos += add;
	}

	int mode = nxApp::get_int_opt("mode", 1);
	if (mode == 1) {
		cxVec pos(-8.0f, 0.0f, -2.0f);
		cxVec add(0.75f, 0.0f, 0.0f);
		cxQuat q = nxQuat::identity();
		for (int i = 0; i < 20; ++i) {
			ScnObj* pObj = nullptr;
			descr.scale = 1.0f;
			descr.variation = 9 - (i >> 1);
			if (i & 1) {
				pObj = SmpCharSys::add_f(descr, ctrlFunc);
			} else {
				pObj = SmpCharSys::add_m(descr, ctrlFunc);
			}
			if (pObj) {
				pObj->set_world_quat_pos(q, pos);
			}
			pos += add;
			if (i == 9) {
				pos.x = -8.0f;
				pos.z += -1.25f;
			}
		}
	}
}

static Pint::FuncLibrary* s_pFuncLib = nullptr;

static void init() {
	s_roamProgKind = RoamProgKind::NATIVE;
	const char* pRoamProgOpt = nxApp::get_opt("roamprog");
	if (pRoamProgOpt) {
		if (nxCore::str_eq(pRoamProgOpt, "pint")) {
			s_roamProgKind = RoamProgKind::PINT;
		} else if (nxCore::str_eq(pRoamProgOpt, "plop")) {
			s_roamProgKind = RoamProgKind::PLOP;
		}
	}

	//Scene::alloc_global_heap(1024 * 1024 * 2);
	Scene::alloc_local_heaps(1024 * 1024 * 2);
	SmpCharSys::init();

	if (s_roamProgKind == RoamProgKind::PINT) {
		Pint::set_mem_lock(Scene::get_glb_mem_lock());
		load_pint_progs();
	}

	//init_single_char();
	//init_two_chars();
	init_chars();
	init_stage();
	s_execStopWatch.alloc(120);
	Scene::enable_split_move(nxApp::get_bool_opt("split_move", false));
	nxCore::dbg_msg("Scene::split_move: %s\n", Scene::is_split_move_enabled() ? "Yes" : "No");
	s_exerep = nxCalc::clamp(nxApp::get_int_opt("exerep", 1), 1, 100);
	nxCore::dbg_msg("exerep: %d\n", s_exerep);
	nxCore::dbg_msg("Scene::speed: %.2f\n", Scene::speed());
	int ncpus = nxSys::num_active_cpus();
	nxCore::dbg_msg("num active CPUs: %d\n", ncpus);

	nxCore::dbg_msg("roam prog kind: ");
	if (s_roamProgKind == RoamProgKind::PINT) {
		nxCore::dbg_msg("PINT");
		s_pFuncLib = nxCore::tMem<Pint::FuncLibrary>::alloc();
		s_pFuncLib->init();

		s_pFuncLib->register_func(s_df_rng_next_desc);
		s_pFuncLib->register_func(s_df_rng_01_desc);
		s_pFuncLib->register_func(s_df_ck_act_timeout_desc);
		s_pFuncLib->register_func(s_df_get_obj_touch_duration_secs_desc);
		s_pFuncLib->register_func(s_df_get_wall_touch_duration_secs_desc);
		s_pFuncLib->register_func(s_df_math_fit_desc);
	} else if (s_roamProgKind == RoamProgKind::PLOP) {
		nxCore::dbg_msg("PLOP");
	} else {
		nxCore::dbg_msg("NATIVE");
	}
	nxCore::dbg_msg("\n");
}

static struct ViewWk {
	float t;
	float dt;

	void update() {
		t += dt * SmpCharSys::get_motion_speed() * 2.0f;
		if (t > 1.0f) {
			t = 1.0f;
			dt = -dt;
		} else if (t <= 0.0f) {
			t = 0.0f;
			dt = -dt;
		}
	}
} s_view;

static void view_exec() {
	if (s_view.dt == 0.0f) {
		s_view.t = 0.0f;
		s_view.dt = 0.0025f;
	}
	float tz = nxCalc::ease(s_view.t);
	cxVec tgt = cxVec(0.0f, 1.0f, nxCalc::fit(tz, 0.0f, 1.0f, 0.0f, 6.0f));
	cxVec pos = cxVec(3.9f, 1.9f, 3.5f);
	Scene::set_view(pos, tgt);
	s_view.update();
}

static void set_scene_ctx() {
	Scene::set_shadow_dir_degrees(70.0f, 140.0f);
	Scene::set_shadow_proj_params(20.0f, 10.0f, 50.0f);
	Scene::set_shadow_fade(28.0f, 35.0f);
	Scene::set_shadow_uniform(false);

	Scene::set_spec_dir_to_shadow();
	Scene::set_spec_shadowing(0.75f);

	Scene::set_hemi_upper(2.5f, 2.46f, 2.62f);
	Scene::set_hemi_lower(0.32f, 0.28f, 0.26f);
	Scene::set_hemi_up(Scene::get_shadow_dir().neg_val());
	Scene::set_hemi_exp(2.5f);
	Scene::set_hemi_gain(0.7f);

	Scene::set_fog_rgb(0.748f, 0.79f, 0.87f);
	Scene::set_fog_density(1.0f);
	Scene::set_fog_range(1.0f, 200.0f);
	Scene::set_fog_curve(0.1f, 0.1f);

	Scene::set_exposure_rgb(0.75f, 0.85f, 0.5f);

	Scene::set_linear_white_rgb(0.65f, 0.65f, 0.55f);
	Scene::set_linear_gain(1.32f);
	Scene::set_linear_bias(-0.025f);
}

static void draw_2d() {
	char str[512];
	const char* fpsStr = "FPS: ";
	const char* exeStr = "EXE: ";
	float refSizeX = 550.0f;
	float refSizeY = 350.0f;
	Scene::set_ref_scr_size(refSizeX, refSizeY);
	if (Scene::get_view_mode_width() < Scene::get_view_mode_height()) {
		Scene::set_ref_scr_size(refSizeY, refSizeX);
	}

	float sx = 10.0f;
	float sy = Scene::get_ref_scr_height() - 20.0f;

	xt_float2 bpos[4];
	xt_float2 btex[4];
	btex[0].set(0.0f, 0.0f);
	btex[1].set(1.0f, 0.0f);
	btex[2].set(1.0f, 1.0f);
	btex[3].set(0.0f, 1.0f);
	float bx = 4.0f;
	float by = 4.0f;
	float bw = 280.0f;
	float bh = 12.0f;
	cxColor bclr(0.0f, 0.0f, 0.0f, 0.75f);
	bpos[0].set(sx - bx, sy - by);
	bpos[1].set(sx + bw + bx, sy - by);
	bpos[2].set(sx + bw + bx, sy + bh + by);
	bpos[3].set(sx - bx, sy + bh + by);
	Scene::quad(bpos, btex, bclr);

	float fps = SmpCharSys::get_fps();
	if (fps <= 0.0f) {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s--", fpsStr);
	} else {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s%.2f", fpsStr, fps);
	}
	Scene::print(sx, sy, cxColor(0.1f, 0.75f, 0.1f, 1.0f), str);
	sx += 120.0f;

	float exe = s_medianExecMillis;
	if (exe <= 0.0f) {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s--", exeStr);
	} else {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s%.4f millis", exeStr, exe);
	}
	Scene::print(sx, sy, cxColor(0.5f, 0.4f, 0.1f, 1.0f), str);
}

static void profile_start() {
	s_execStopWatch.begin();
}

static void profile_end() {
	if (s_execStopWatch.end()) {
		double us = s_execStopWatch.median();
		s_execStopWatch.reset();
		double millis = us / 1000.0;
		s_medianExecMillis = float(millis);
		Scene::thermal_info();
		Scene::battery_info();
	}
}

static bool chr_exec_init_pint_func(ScnObj* pObj, void* pWkMem) {
	SmpChar* pChr = SmpCharSys::char_from_obj(pObj);
	if (pChr) {
		Pint::ExecContext* pCtx = nxCore::tMem<Pint::ExecContext>::alloc();
		pCtx->init(pChr);
		pChr->set_ptr_wk<Pint::ExecContext>(int(BindIdx::EXECONTEXT), pCtx);
		pChr->set_ptr_wk<Pint::FuncLibrary>(int(BindIdx::FUNCLIB), s_pFuncLib);
	}
	return true;
}

static void chr_exec_init() {
	if (s_roamProgKind == RoamProgKind::PINT) {
		Scene::for_each_obj(chr_exec_init_pint_func, nullptr);
	}
}

static bool chr_exec_reset_pint_func(ScnObj* pObj, void* pWkMem) {
	SmpChar* pChr = SmpCharSys::char_from_obj(pObj);
	if (pChr) {
		Pint::ExecContext* pCtx = pChr->get_ptr_wk<Pint::ExecContext>(int(BindIdx::EXECONTEXT));
		nxCore::tMem<Pint::ExecContext>::free(pCtx);
	}
	return true;
}

static void chr_exec_reset() {
	if (s_roamProgKind == RoamProgKind::PINT) {
		Scene::for_each_obj(chr_exec_reset_pint_func, nullptr);
	}
}

static void scn_exec() {
	for (int i = 0; i < s_exerep; ++i) {
		chr_exec_init();
		Scene::exec();
		chr_exec_reset();
	}
}

static void loop(void* pLoopCtx) {
	SmpCharSys::start_frame();
	set_scene_ctx();
	profile_start();
	scn_exec();
	view_exec();
	Scene::visibility();
	profile_end();
	Scene::frame_begin(cxColor(0.25f));
	Scene::draw();
	draw_2d();
	Scene::frame_end();
}

static void reset() {
	SmpCharSys::reset();
	s_execStopWatch.free();

	if (s_roamProgKind == RoamProgKind::PINT) {
		if (s_pFuncLib) {
			s_pFuncLib->reset();
			nxCore::tMem<Pint::FuncLibrary>::free(s_pFuncLib, 1);
		}

		free_pint_progs();
	}

}

DEMO_REGISTER(default);

DEMO_PROG_END
