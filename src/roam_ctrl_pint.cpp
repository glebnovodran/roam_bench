#include "crosscore.hpp"
#include "scene.hpp"
#include "smprig.hpp"
#include "smpchar.hpp"
#include "pint.hpp"

#include "roam_ctrl.hpp"

enum class BindIdx {
	EXECONTEXT = 0,
	FUNCLIB = 1
};

static void init_func_lib(Pint::FuncLibrary** ppFuncLib);

struct PintProg {
	char* pSrc;
	size_t srcSz;
};

static struct ActPintProg {
	const char* pActName;
	const char* pProgName;
	PintProg prog;
} s_actProgs[] = {
	{ "ACT_STAND", "ACT_STAND", { nullptr, 0 } },
	{ "ACT_TURN_L", "ACT_TURN", { nullptr, 0 } },
	{ "ACT_TURN_R", "ACT_TURN", { nullptr, 0 } },
	{ "ACT_WALK", "ACT_WALK", { nullptr, 0 } },
	{ "ACT_RETREAT", "ACT_RETREAT", { nullptr, 0 } },
	{ "ACT_RUN", "ACT_RUN", { nullptr, 0 } }
};

static char* load_prog(const char* pName, size_t& srcSize) {
	char* pProg = reinterpret_cast<char*>(Scene::load_bin_file(pName, &srcSize, "acts", "pint"));
	if (pProg && srcSize > 0) {
		if (nxApp::get_bool_opt("pint_cache", false)) {
			Pint::cache(pProg, srcSize);
		}
	}
	return pProg;
}

static void free_pint_prog(PintProg& prog) {
	if (prog.pSrc) {
		Scene::unload_bin_file(prog.pSrc);
		prog.pSrc = nullptr;
	}
}
static void free_act_pint_progs(ActPintProg* pActProgs, size_t nprogs) {
	if (pActProgs) {
		for (size_t i = 0; i < nprogs; ++i) {
			free_pint_prog(pActProgs[i].prog);
			char* pSrc = pActProgs[i].prog.pSrc;
			if (pSrc) {
				nxCore::bin_unload(pSrc);
				pSrc = nullptr;
			}
		}
	}
}

static void load_act_pint_progs(ActPintProg* pActProgs, size_t nprogs) {
	for (size_t i = 0; i < nprogs; ++i) {
		char* pProgSrc = load_prog(pActProgs[i].pProgName, pActProgs[i].prog.srcSz);
		if (pProgSrc) {
			pActProgs[i].prog.pSrc = pProgSrc;
		} else {
			nxCore::dbg_msg("Couldn't load %s character program\n", pActProgs[i].pProgName);
		}
	}
}


static struct PintWk {
	ActPintProg* mpActProgs;
	PintProg mMoodProg;
	Pint::FuncLibrary* mpFuncLib;
	size_t mNumActProgs;

	void init() {
		Pint::set_mem_lock(Scene::get_glb_mem_lock());
		init_func_lib(&mpFuncLib);
		load_act_pint_progs(mpActProgs, mNumActProgs);
		mMoodProg.pSrc = load_prog("mood", mMoodProg.srcSz);
		if (mMoodProg.pSrc == nullptr) {
			nxCore::dbg_msg("Can't load mood program\n");
		}
	}

	void reset() {
		if (mpFuncLib) {
			mpFuncLib->reset();
			nxCore::tMem<Pint::FuncLibrary>::free(mpFuncLib, 1);
			mpFuncLib = nullptr;
		}
		free_act_pint_progs(mpActProgs, mNumActProgs);
		free_pint_prog(mMoodProg);
	}

} s_pintWk = {
	s_actProgs,
	{ nullptr, 0 },
	nullptr,
	XD_ARY_LEN(s_actProgs)
};

static Pint::Value scn_rng_next(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	uint64_t rnd = Scene::glb_rng_next();
	rnd &= 0xffffffff;
	res.set_num(double(rnd));
	return res;
}
static const Pint::FuncDef s_df_rng_next_desc = {
	"glb_rng_next", scn_rng_next, 0, Pint::Value::Type::NUM, {Pint::Value::Type::NUM}
};

static Pint::Value scn_rng_01(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	float rnd = Scene::glb_rng_f01();
	res.set_num(double(rnd));
	return res;
}
static const Pint::FuncDef s_df_rng_01_desc = {
	"glb_rng_01", scn_rng_01, 0, Pint::Value::Type::NUM, {Pint::Value::Type::NUM}
};

static Pint::Value ck_act_timeout(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	SmpChar* pChar = reinterpret_cast<SmpChar*>(ctx.get_local_binding());
	bool isTimeOut = pChar->check_act_time_out();
	res.set_num(isTimeOut);
	return res;
}

static const Pint::FuncDef s_df_ck_act_timeout_desc = {
	"check_act_timeout", ck_act_timeout, 0, Pint::Value::Type::NUM, {Pint::Value::Type::NUM}
};

static Pint::Value get_obj_touch_duration_secs(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	SmpChar* pChar = reinterpret_cast<SmpChar*>(ctx.get_local_binding());
	res.set_num(pChar->get_obj_touch_duration_secs());
	return res;
}

static const Pint::FuncDef s_df_get_obj_touch_duration_secs_desc = {
	"get_obj_touch_duration_secs", get_obj_touch_duration_secs, 0, Pint::Value::Type::NUM, {Pint::Value::Type::NUM}
};

static Pint::Value get_wall_touch_duration_secs(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	SmpChar* pChar = reinterpret_cast<SmpChar*>(ctx.get_local_binding());
	res.set_num(pChar->get_wall_touch_duration_secs());
	return res;
}

static const Pint::FuncDef s_df_get_wall_touch_duration_secs_desc = {
	"get_wall_touch_duration_secs", get_wall_touch_duration_secs, 0, Pint::Value::Type::NUM, {Pint::Value::Type::NUM}
};

static Pint::Value math_fit(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	res.set_none();
	if (nargs < 5) {
		nxCore::dbg_break("Error: math_fit - bad argument number\n");
	} else {
		double val = pArgs[0].val.num;
		double oldMin = pArgs[1].val.num;
		double oldMax = pArgs[2].val.num;
		double newMin = pArgs[3].val.num;
		double newMax = pArgs[4].val.num;
		res.set_num(nxCalc::fit(val, oldMin, oldMax, newMin, newMax));
	}
	return res;
}

static const Pint::FuncDef s_df_math_fit_desc = {
	"math_fit", math_fit, 5, Pint::Value::Type::NUM,
	{Pint::Value::Type::NUM, Pint::Value::Type::NUM, Pint::Value::Type::NUM, Pint::Value::Type::NUM, Pint::Value::Type::NUM}
};

static Pint::Value get_mood_arg(Pint::ExecContext& ctx, const uint32_t nargs, Pint::Value* pArgs) {
	Pint::Value res;
	SmpChar* pChar = reinterpret_cast<SmpChar*>(ctx.get_local_binding());
	double nowTime = pChar->mMoodTimer;
	double t = calc_mood_arg(nowTime);
	res.set_num(t);
	return res;
}

static const Pint::FuncDef s_df_get_mood_arg_desc = {
	"get_mood_arg", get_mood_arg, 0, Pint::Value::Type::NUM, {Pint::Value::Type::NUM}
};

static void init_func_lib(Pint::FuncLibrary** ppFuncLib) {
	(*ppFuncLib) = nxCore::tMem<Pint::FuncLibrary>::alloc();
	(*ppFuncLib)->init();

	(*ppFuncLib)->register_func(s_df_rng_next_desc);
	(*ppFuncLib)->register_func(s_df_rng_01_desc);
	(*ppFuncLib)->register_func(s_df_ck_act_timeout_desc);
	(*ppFuncLib)->register_func(s_df_get_obj_touch_duration_secs_desc);
	(*ppFuncLib)->register_func(s_df_get_wall_touch_duration_secs_desc);
	(*ppFuncLib)->register_func(s_df_math_fit_desc);
	(*ppFuncLib)->register_func(s_df_get_mood_arg_desc);
}

static int find_pint_prog(const char* pActName, ActPintProg** ppProg) {
	int idx = -1;
	size_t nacts = XD_ARY_LEN(s_actProgs);
	for (size_t i = 0; i < nacts; ++i) {
		if (nxCore::str_eq(pActName, s_actProgs[i].pActName)) {
			if (ppProg) {
				(*ppProg) = &s_actProgs[i];
			}
			idx = i;
			break;
		}
	}
	return idx;
}

static float char_mood_calc_pint(SmpChar* pChar) {
	Pint::ExecContext* pCtx = pChar->get_ptr_wk<Pint::ExecContext>(int(BindIdx::EXECONTEXT));
	Pint::FuncLibrary* pFuncLib = pChar->get_ptr_wk<Pint::FuncLibrary>(int(BindIdx::FUNCLIB));
	float mood = 0.0f;

	if (pCtx && pFuncLib) {
		pCtx->clear_vars();
		Pint::interp(s_pintWk.mMoodProg.pSrc, s_pintWk.mMoodProg.srcSz, pCtx, pFuncLib);
		mood = pCtx->get_num_val("y", 0.0);
	}

	return mood;
}

void roam_ctrl_pint(SmpChar* pChar) {
	if (!pChar) return;

	size_t nprogs = XD_ARY_LEN(s_actProgs);

	if (pChar->mAction < int(nprogs)) {
		const char* pSrc = s_actProgs[pChar->mAction].prog.pSrc;
		size_t srcSize = s_actProgs[pChar->mAction].prog.srcSz;
		Pint::ExecContext* pCtx = pChar->get_ptr_wk<Pint::ExecContext>(int(BindIdx::EXECONTEXT));
		Pint::FuncLibrary* pFuncLib = pChar->get_ptr_wk<Pint::FuncLibrary>(int(BindIdx::FUNCLIB));

		if (is_mood_enabled()) {
			pChar->mood_update();
			pChar->mMood = nxCalc::saturate(char_mood_calc_pint(pChar));
			if (is_mood_visible() && pChar->mpObj) {
				float moodc = pChar->mMood;
				pChar->mpObj->set_base_color_scl(moodc, moodc, moodc);
			}
		}

		if (is_roamctrl_disabled()) return;
		pCtx->clear_vars();
		Pint::interp(pSrc, srcSize, pCtx, pFuncLib);
		pCtx->print_vars();
		Pint::Value* pNewActVal = pCtx->var_val("newActName");
		const char* pNewActName = pNewActVal ? pNewActVal->val.pStr : "";
		if (!nxCore::str_eq(pNewActName, "")) {
			ActPintProg* pProg = nullptr;
			int actId = find_pint_prog(pNewActName, &pProg);
			if (actId >= 0) {
				double newActDuration = pCtx->get_num_val("newActDuration", 0.0);
				pChar->change_act(actId, newActDuration);
			}
		}

		Pint::Value* pWTResetVal = pCtx->var_val("wallTouchReset");
		bool wallTouchReset = pWTResetVal ? bool(pWTResetVal->val.num) : false;
		if (wallTouchReset) {
			pChar->reset_wall_touch();
		}
	}
}

bool chr_exec_init_pint_func(ScnObj* pObj, void* pWkMem) {
	SmpChar* pChr = SmpCharSys::char_from_obj(pObj);
	if (pChr) {
		Pint::ExecContext* pCtx = nxCore::tMem<Pint::ExecContext>::alloc();
		pCtx->init(pChr);
		pChr->set_ptr_wk<Pint::ExecContext>(int(BindIdx::EXECONTEXT), pCtx);
		pChr->set_ptr_wk<Pint::FuncLibrary>(int(BindIdx::FUNCLIB), s_pintWk.mpFuncLib);
	}
	return true;
}

bool chr_exec_reset_pint_func(ScnObj* pObj, void* pWkMem) {
	SmpChar* pChr = SmpCharSys::char_from_obj(pObj);
	if (pChr) {
		Pint::ExecContext* pCtx = pChr->get_ptr_wk<Pint::ExecContext>(int(BindIdx::EXECONTEXT));
		nxCore::tMem<Pint::ExecContext>::free(pCtx);
	}
	return true;
}

void init_roam_pint() {
	s_pintWk.init();
}

void reset_roam_pint() {
	s_pintWk.reset();
}
