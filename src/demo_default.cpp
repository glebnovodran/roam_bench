#include "crosscore.hpp"
#include "oglsys.hpp"
#include "scene.hpp"
#include "demo.hpp"
#include "smprig.hpp"
#include "smpchar.hpp"
#include "pint.hpp"

#include "roam_ctrl.hpp"

DEMO_PROG_BEGIN

static cxStopWatch s_execStopWatch;
static float s_medianExecMillis = -1.0f;
static int s_exerep = 1;
static int s_dummyFPS = 0;
static int s_minimapMode = 0;
static float s_moodPeriod = -1.0f;
static bool s_moodVis = false;

static RoamProgKind s_roamProgKind = RoamProgKind::NATIVE;
static int s_mode = 0;
static bool s_roamctrlDisabled = false;
static uint64_t s_quitFrame = 0;
static bool s_draw2dDisable = false;

struct CtrlExecStats {
	double sum;
	double avg;
	int nchr;
};

static bool ctrldt_obj_func(ScnObj* pObj, void* pWkMem) {
	SmpChar* pChr = SmpCharSys::char_from_obj(pObj);
	if (pChr) {
		CtrlExecStats* pWk = reinterpret_cast<CtrlExecStats*>(pWkMem);
		if (pWk) {
			pWk->sum += pChr->mCtrlDt;
			pWk->nchr++;
		}
	}
	return true;
}

static CtrlExecStats calc_ctrldt() {
	CtrlExecStats wk;
	wk.nchr = 0;
	wk.sum = 0.0;
	Scene::for_each_obj(ctrldt_obj_func, &wk);
	wk.avg = nxCalc::div0(wk.sum, double(wk.nchr));
	return wk;
}

static double s_ctrldt_smps[30];
static uint32_t s_ctrldt_idx = 0;
static double s_ctrldtAvg = -1.0;
static double s_ctrldt_avg_smps[10];
static uint32_t s_ctrldt_avg_idx = 0;
static double s_ctrldtAvgAvg = -1.0;
static bool s_ctrdtInfoEnabled = true;

static void draw_2d_ctrl_stats() {
	if (!s_ctrdtInfoEnabled) return;
	if (s_ctrldt_idx >= XD_ARY_LEN(s_ctrldt_smps)) {
		double ctrldtAvg = 0.0;
		for (size_t i = 0; i < XD_ARY_LEN(s_ctrldt_smps); ++i) {
			ctrldtAvg += s_ctrldt_smps[i];
		}
		ctrldtAvg /= double(XD_ARY_LEN(s_ctrldt_smps));
		s_ctrldtAvg = ctrldtAvg;
		s_ctrldt_idx = 0;
		if (s_ctrldt_avg_idx < XD_ARY_LEN(s_ctrldt_avg_smps)) {
			s_ctrldt_avg_smps[s_ctrldt_avg_idx++] = ctrldtAvg;
		} else {
			double avgAvg = 0.0f;
			for (size_t i = 0; i < XD_ARY_LEN(s_ctrldt_avg_smps); ++i) {
				avgAvg += s_ctrldt_avg_smps[i];
			}
			avgAvg /= double(XD_ARY_LEN(s_ctrldt_avg_smps));
			s_ctrldtAvgAvg = avgAvg;
			s_ctrldt_avg_idx = 0;
		}
	}
	CtrlExecStats ctrldt = calc_ctrldt();
	s_ctrldt_smps[s_ctrldt_idx++] = ctrldt.sum;
	if (s_ctrldtAvg < 0.0) return;

	char str[512];
	float sx = 10.3f;
	float sy = 10.0f;
	xt_float2 bpos[4];
	xt_float2 btex[4];
	btex[0].set(0.0f, 0.0f);
	btex[1].set(1.0f, 0.0f);
	btex[2].set(1.0f, 1.0f);
	btex[3].set(0.0f, 1.0f);
	float bx = 4.0f;
	float by = 4.0f;
	float bw = 330.0f;
	float bh = 12.0f;
	cxColor bclr(0.0f, 0.0f, 0.1f, 0.15f);
	bpos[0].set(sx - bx, sy - by);
	bpos[1].set(sx + bw + bx, sy - by);
	bpos[2].set(sx + bw + bx, sy + bh + by);
	bpos[3].set(sx - bx, sy + bh + by);
	Scene::quad(bpos, btex, bclr);
	const char* pCtrlProgStr = "?";
	switch (s_roamProgKind) {
		case RoamProgKind::NATIVE: pCtrlProgStr = "Native"; break;
		case RoamProgKind::PINT: pCtrlProgStr = "Pint"; break;
		case RoamProgKind::QJS: pCtrlProgStr = "QuickJS"; break;
		case RoamProgKind::LUA: pCtrlProgStr = "Lua"; break;
		case RoamProgKind::WRENCH: pCtrlProgStr = "wrench"; break;
		default: break;
	}
	if (Scene::get_num_active_workers() > 0) {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s, %d chars, multithreaded mode", pCtrlProgStr, ctrldt.nchr);
	} else {
		if (s_ctrldtAvgAvg > 0.0) {
			XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s, %d chars: %.2f (%.2f) micros", pCtrlProgStr, ctrldt.nchr, s_ctrldtAvg, s_ctrldtAvgAvg);
		} else {
			XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s, %d chars: %.2f micros", pCtrlProgStr, ctrldt.nchr, s_ctrldtAvg);
		}
	}
	Scene::print(sx, sy, cxColor(0.75f, 0.4f, 0.1f, 1.0f), str);

	if (OGLSys::is_dummy()) {
		nxCore::dbg_msg("\n[\x1B[1m\x1B[46m\x1B[93m %s \x1B[0m]\x1B[F", str);
	}
}


struct AVG_SAMPLES {
	double* mpSmps;
	int mNum;
	int mCur;
	double mAvg;

	void init(const int num) {
		mCur = 0;
		mNum = num;
		mpSmps = nullptr;
		if (num > 1) {
			mpSmps = (double*)nxCore::mem_alloc(num * sizeof(double), "Avg:smps");
		}
		mAvg = -1.0f;
	}

	void reset() {
		if (mpSmps) {
			nxCore::mem_free(mpSmps);
			mpSmps = nullptr;
		}
	}

	void add_smp(const double val) {
		if (val <= 0.0) return;
		if (!mpSmps) return;
		if (mCur < mNum) {
			mpSmps[mCur++] = val;
			if (mCur >= mNum) {
				double sum = 0.0;
				for (int i = 0; i < mNum; ++i) {
					sum += mpSmps[i];
				}
				double s = nxCalc::rcp0(double(mNum));
				mAvg = sum * s;
				mCur = 0;
			}
		}
	}
};

static AVG_SAMPLES s_avgFPS = {};
static AVG_SAMPLES s_avgEXE = {};

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

static ScnObj* add_char(const SmpChar::Descr& descr, char charType, SmpChar::CtrlFunc& ctrlFunc);

static void init_single_char(SmpChar::CtrlFunc& ctrlFunc) {
	SmpChar::Descr descr;
	descr.reset();

	float x = -5.57f;
	ScnObj* pObj = add_char(descr, 'f', ctrlFunc);
	pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(x, 0.0f, 0.0f));
}

static void init_two_chars(SmpChar::CtrlFunc& ctrlFunc) {
	SmpChar::Descr descr;
	descr.reset();

	float x = -5.57f;
	ScnObj* pObj = add_char(descr, 'f', ctrlFunc);
	pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(x, 0.0f, 0.0f));
	x += 0.7f;
	pObj = add_char(descr, 'm', ctrlFunc);
	pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(x, 0.0f, 0.0f));
}

static ScnObj* add_char(const SmpChar::Descr& descr, char charType, SmpChar::CtrlFunc& ctrlFunc) {
	ScnObj* pObj = nullptr;
	if (charType == 'f') {
		pObj = SmpCharSys::add_f(descr, ctrlFunc);
	} else {
		pObj = SmpCharSys::add_m(descr, ctrlFunc);
	}
	return pObj;
}

static void init_chars(SmpChar::CtrlFunc& ctrlFunc) {
	SmpChar::Descr descr;
	descr.reset();
	bool disableSl = nxApp::get_bool_opt("scl_off", false);

	if (s_mode == -1) {
		init_single_char(ctrlFunc);
		return;
	} else if (s_mode == -2) {
		init_two_chars(ctrlFunc);
		return;
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

		ScnObj* pObj = add_char(descr, 'f', ctrlFunc);
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

		ScnObj* pObj = add_char(descr, 'm', ctrlFunc);
		if (pObj) {
			pObj->set_world_quat_pos(q, pos);
		}
		pos += add;
	}

	if (s_mode == 1) {
		cxVec pos(-8.0f, 0.0f, -2.0f);
		cxVec add(0.75f, 0.0f, 0.0f);
		cxQuat q = nxQuat::identity();
		for (int i = 0; i < 20; ++i) {
			ScnObj* pObj = nullptr;
			descr.scale = 1.0f;
			descr.variation = 9 - (i >> 1);
			if (i & 1) {
				pObj = add_char(descr, 'f', ctrlFunc);
			} else {
				pObj = add_char(descr, 'm', ctrlFunc);
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

static void init() {
	s_roamProgKind = RoamProgKind::NATIVE;
	const char* pRoamProgOpt = nxApp::get_opt("roamprog");
	if (pRoamProgOpt) {
		if (nxCore::str_eq(pRoamProgOpt, "pint")) {
			s_roamProgKind = RoamProgKind::PINT;
		} else if (nxCore::str_eq(pRoamProgOpt, "plop")) {
			s_roamProgKind = RoamProgKind::PLOP;
		} else if (nxCore::str_eq(pRoamProgOpt, "qjs")) {
			s_roamProgKind = RoamProgKind::QJS;
		} else if (nxCore::str_eq(pRoamProgOpt, "lua")) {
			s_roamProgKind = RoamProgKind::LUA;
		} else if (nxCore::str_eq(pRoamProgOpt, "wrench")) {
			s_roamProgKind = RoamProgKind::WRENCH;
		} else if (nxCore::str_eq(pRoamProgOpt, "minion")) {
			s_roamProgKind = RoamProgKind::MINION;
		}
	}

	s_mode = nxApp::get_int_opt("mode", 1);
	s_roamctrlDisabled = nxApp::get_bool_opt("roamctrl_disable", false);
	//Scene::alloc_global_heap(1024 * 1024 * 2);
	Scene::alloc_local_heaps(1024 * 1024 * 2);
	SmpCharSys::init();

	SmpChar::CtrlFunc ctrlFunc = roam_ctrl_native;
	if (s_roamProgKind == RoamProgKind::PINT) {
		ctrlFunc = roam_ctrl_pint;
		init_roam_pint();
	} else if (s_roamProgKind == RoamProgKind::QJS) {
		ctrlFunc = roam_ctrl_qjs;
		init_roam_qjs();
	} else if (s_roamProgKind == RoamProgKind::LUA) {
		ctrlFunc = roam_ctrl_lua;
		init_roam_lua();
	} else if (s_roamProgKind == RoamProgKind::WRENCH) {
		ctrlFunc = roam_ctrl_wrench;
		init_roam_wrench();
	} else if (s_roamProgKind == RoamProgKind::MINION) {
		ctrlFunc = roam_ctrl_minion;
		init_roam_minion();
	}

	init_chars(ctrlFunc);
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
	} else if (s_roamProgKind == RoamProgKind::PLOP) {
		nxCore::dbg_msg("PLOP");
	} else if (s_roamProgKind == RoamProgKind::QJS) {
		nxCore::dbg_msg("QJS");
	} else if (s_roamProgKind == RoamProgKind::LUA) {
		nxCore::dbg_msg("LUA");
	} else if (s_roamProgKind == RoamProgKind::WRENCH) {
		nxCore::dbg_msg("WRENCH");
	} else if (s_roamProgKind == RoamProgKind::MINION) {
		nxCore::dbg_msg("MINION");
	} else {
		nxCore::dbg_msg("NATIVE");
	}
	nxCore::dbg_msg("\n");

	s_dummyFPS = nxApp::get_int_opt("dummyfps", 0);
	s_minimapMode = nxApp::get_int_opt("minimap_mode", 0);

	int numAvgSmps = nxApp::get_int_opt("avg_smps", 0);

	s_avgFPS.init(numAvgSmps);
	s_avgEXE.init(numAvgSmps);

	s_moodPeriod = nxApp::get_float_opt("mood_period", -1.0f);
	nxCore::dbg_msg("mood period: %f\n", s_moodPeriod);
	s_moodVis = nxApp::get_bool_opt("mood_vis", false);

	s_ctrdtInfoEnabled = nxApp::get_bool_opt("ctrldt_info", true);

	s_quitFrame = nxApp::get_int_opt("quit_frame", 0);
	s_draw2dDisable = nxApp::get_bool_opt("draw_2d_disable", false);
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

#define D_MINIMAP_W 24
#define D_MINIMAP_H 18

static char s_minimap[(D_MINIMAP_W + 1)*D_MINIMAP_H + 1];
static bool s_minimapFlg = false;

struct MINIMAP_WK {
	cxAABB bbox;
	char* pMap;
	int w;
	int h;
};

static bool minimap_obj_func(ScnObj* pObj, void* pWkMem) {
	SmpChar* pChr = SmpCharSys::char_from_obj(pObj);
	if (pChr) {
		MINIMAP_WK* pWk = (MINIMAP_WK*)pWkMem;
		if (pWk) {
			cxVec wpos = pObj->get_world_pos();
			cxVec rel = wpos - pWk->bbox.get_min_pos();
			cxVec sv = pWk->bbox.get_size_vec();
			cxVec isv = nxVec::rcp0(sv);
			float x = nxCalc::saturate(rel.x * isv.x);
			float z = nxCalc::saturate(rel.z * isv.z);
			int ix = (int)(mth_roundf(x * float(D_MINIMAP_W - 1)));
			int iy = (int)(mth_roundf(z * float(D_MINIMAP_H - 1)));
			if (pWk->pMap) {
				int idx = iy*(D_MINIMAP_W + 1) + ix;
				char sym = '*';
				if (s_minimapMode != 0) {
					sym = ' ';
					if (SmpCharSys::obj_is_f(pObj)) {
						sym = 'f';
					} else if (SmpCharSys::obj_is_m(pObj)) {
						sym = 'm';
					}
				}
				pWk->pMap[idx] = sym;
			}
		}
	}
	return true;
}

static void print_minimap() {
	sxCollisionData* pCol = SmpCharSys::get_collision();
	if (!pCol) return;
	MINIMAP_WK wk;
	wk.bbox = pCol->mBBox;
	wk.w = D_MINIMAP_W;
	wk.h = D_MINIMAP_H;
	wk.pMap = s_minimap;
	nxCore::mem_zero(s_minimap, sizeof(s_minimap));
	char* pDst = wk.pMap;
	for (int y = 0; y < D_MINIMAP_H; ++y) {
		for (int x = 0; x < D_MINIMAP_W; ++x) {
			*pDst++ = '.';
		}
		*pDst++ = '\n';
	}
	Scene::for_each_obj(minimap_obj_func, &wk);
	if (s_minimapFlg) {
		nxCore::dbg_msg("\x1B[%dA\x1B[G", D_MINIMAP_H);
	}
	nxCore::dbg_msg("%s", s_minimap);
	s_minimapFlg = true;
}

static void draw_2d() {
	if (s_draw2dDisable) return;

	char str[512];
	const char* fpsStr = "FPS: ";
	const char* exeStr = "EXE: ";
	float refSizeX = 550.0f;
	float refSizeY = 350.0f;
	Scene::set_ref_scr_size(refSizeX, refSizeY);
	if (Scene::get_view_mode_width() < Scene::get_view_mode_height()) {
		Scene::set_ref_scr_size(refSizeY, refSizeX);
	}

	double avgFPS = s_avgFPS.mAvg;
	double avgExe = s_avgEXE.mAvg;

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
	float bw = 280.0f + (avgExe > 0.0 ? 78.0f : 0.0f);
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
		if (avgFPS > 0.0) {
			XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s%.2f (%.1f)", fpsStr, fps, avgFPS);
		} else {
			XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s%.2f", fpsStr, fps);
		}
	}
	Scene::print(sx, sy, cxColor(0.1f, 0.75f, 0.1f, 1.0f), str);
	sx += 120.0f;
	if (avgFPS > 0.0) {
		sx += 40.0f;
	}

	if (OGLSys::is_dummy()) {
		print_minimap();
		nxCore::dbg_msg("\x1B[G[\x1B[1m\x1B[42m\x1B[93m %s", str);
	}

	float exe = s_medianExecMillis;
	if (exe <= 0.0f) {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s--", exeStr);
	} else {
		if (avgExe > 0.0) {
			XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s%.3f (%.2f) millis", exeStr, exe, avgExe);
		} else {
			XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s%.4f millis", exeStr, exe);
		}
	}
	Scene::print(sx, sy, cxColor(0.5f, 0.4f, 0.1f, 1.0f), str);

	if (OGLSys::is_dummy()) {
		nxCore::dbg_msg("  %s \x1B[0m]   ", str);
	}
	draw_2d_ctrl_stats();
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
		s_avgEXE.add_smp(millis);
		s_avgFPS.add_smp(SmpCharSys::get_fps());
		Scene::thermal_info();
		Scene::battery_info();
	}
}

static void chr_exec_init() {
	if (s_roamProgKind == RoamProgKind::PINT) {
		Scene::for_each_obj(chr_exec_init_pint_func, nullptr);
	} else 	if (s_roamProgKind == RoamProgKind::MINION) {
		Scene::for_each_obj(chr_exec_init_minion_func, nullptr);
	}
}

static void chr_exec_reset() {
	if (s_roamProgKind == RoamProgKind::PINT) {
		Scene::for_each_obj(chr_exec_reset_pint_func, nullptr);
	} else 	if (s_roamProgKind == RoamProgKind::MINION) {
		Scene::for_each_obj(chr_exec_reset_minion_func, nullptr);
	}
}

static void scn_exec() {
	for (int i = 0; i < s_exerep; ++i) {
		chr_exec_init();
		Scene::exec();
		chr_exec_reset();
	}
}

static double s_dummyglTStamp = 0.0;

static void dummygl_begin() {
	if (OGLSys::is_dummy() && s_dummyFPS > 0) {
		s_dummyglTStamp = nxSys::time_micros();
	}
}

static void dummygl_end() {
	if (OGLSys::is_dummy() && s_dummyFPS > 0) {
		double usStart = s_dummyglTStamp;
		double usEnd = nxSys::time_micros();
		double refMillis = 1000.0 / double(s_dummyFPS);
		double frameMillis = (usEnd - usStart) / 1000.0;
		if (frameMillis < refMillis) {
			double sleepMillis = ::mth_round(refMillis - frameMillis);
			if (sleepMillis > 0.0) {
				nxSys::sleep_millis((uint32_t)sleepMillis);
			}
		}
	}
}

static void loop(void* pLoopCtx) {
	if (s_quitFrame != 0 && OGLSys::get_frame_count() >= s_quitFrame) {
		OGLSys::quit();
		return;
	}

	dummygl_begin();
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
	dummygl_end();
}

static void reset() {
	SmpCharSys::reset();
	s_execStopWatch.free();
	s_avgFPS.reset();
	s_avgEXE.reset();

	if (s_roamProgKind == RoamProgKind::PINT) {
		reset_roam_pint();
	} else if (s_roamProgKind == RoamProgKind::QJS) {
		reset_roam_qjs();
	} else if (s_roamProgKind == RoamProgKind::LUA) {
		reset_roam_lua();
	} else if (s_roamProgKind == RoamProgKind::WRENCH) {
		reset_roam_wrench();
	} else if (s_roamProgKind == RoamProgKind::MINION) {
		reset_roam_minion();
	}
}

DEMO_REGISTER(default);

DEMO_PROG_END

bool is_mood_enabled() {
	return s_moodPeriod > 0.0f;
}

bool is_mood_visible() {
	return s_moodVis;
}

bool is_roamctrl_disabled() {
	return s_roamctrlDisabled;
}

double calc_mood_arg(double nowTime) {
	double s = nowTime / 1000.0;
	double p = s_moodPeriod;
	return p = nxCalc::saturate(nxCalc::div0(mth_fmod(s, p) , p));
}
