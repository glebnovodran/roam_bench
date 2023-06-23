#include "crosscore.hpp"
#include "scene.hpp"
#include "smprig.hpp"
#include "smpchar.hpp"

#include "roam_ctrl.hpp"

#if ROAM_WRENCH

#include "wrench.h"

static char* load_wrench_prog(const char* pName, size_t& srcSize) {
	return Scene::load_text_cstr(pName, &srcSize, "acts", "w");
}

static void wifc_glb_rng_next(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr);
static void wifc_glb_rng_f01(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr);
static void wifc_check_act_timeout(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr);
static void wifc_obj_touch_duration_secs(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr);
static void wifc_wall_touch_duration_secs(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr);
static void wifc_get_mood_arg(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr);

static struct ActWrenchProg {
	const char* pFuncName;
} s_actProgs[] {
	{ "STAND" }, { "TURN_L" }, { "TURN_R" }, { "WALK" }, { "RETREAT" }, { "RUN" }
};

static struct RoamWrenchWk {
	WRState* mpState;
	WRContext* mpCtx;
	sxLock* mpLock;
	ScnObj* mpActiveObj;

	unsigned char* mpByteCode;
	int mByteCodeLen;
	bool mExecFlg;


	void register_ifc_funcs() {
		if (mpState) {
			wr_registerFunction(mpState, "glb_rng_next", wifc_glb_rng_next);
			wr_registerFunction(mpState, "glb_rng_f01", wifc_glb_rng_f01);
			wr_registerFunction(mpState, "check_act_timeout", wifc_check_act_timeout);
			wr_registerFunction(mpState, "obj_touch_duration_secs", wifc_obj_touch_duration_secs);
			wr_registerFunction(mpState, "wall_touch_duration_secs", wifc_wall_touch_duration_secs);
			wr_registerFunction(mpState, "get_mood_arg", wifc_get_mood_arg);
		}
	}

	void init() {
		mpActiveObj = nullptr;
		mExecFlg = false;
		mpState = wr_newState();
		if (mpState) {
			char errMsg[512];
			mpLock = nxSys::lock_create();
			nxCore::dbg_msg("wrench state: %p\n", mpState);

			wr_loadMathLib(mpState);

			register_ifc_funcs();
			size_t srcSize = 0;
			char* pSrc = load_wrench_prog("acts", srcSize);
			if (pSrc) {
				int err = wr_compile(pSrc, srcSize, &mpByteCode, &mByteCodeLen, errMsg);
				if (!err) {
					mpCtx = wr_run(mpState, mpByteCode, mByteCodeLen);
					mExecFlg = true;
				} else {
					nxCore::dbg_msg("Error while compiling wrench source.\n%s", errMsg);
				}
				Scene::unload_text_cstr(pSrc);
			}
		}
	}

	void reset() {
		mExecFlg = false;
		if (mpCtx) {
			wr_destroyContext(mpCtx);
		}
		if (mpByteCode) {
			delete[] mpByteCode;
			mByteCodeLen = 0;
		}
		if (mpState) {
			wr_destroyState(mpState);
		}
		if (mpLock) {
			nxSys::lock_destroy(mpLock);
			mpLock = nullptr;
		}
	}

	void exec() {
		if (!mExecFlg) return;
		if (!mpActiveObj) return;
		if (!mpState) return;
		if (!mpCtx) return;

		SmpChar* pChar = get_active_char();
		if (!pChar) return;

		if (is_mood_enabled()) {
			pChar->mood_update();
			pChar->mMood = nxCalc::saturate(char_mood_calc(pChar));
			if (is_mood_visible() && pChar->mpObj) {
				float moodc = pChar->mMood;
				pChar->mpObj->set_base_color_scl(moodc, moodc, moodc);
			}
		}

		if (is_roamctrl_disabled()) return;

		int32_t nowAct = pChar->mAction;
		if (nowAct >= int32_t(XD_ARY_LEN(s_actProgs)) || nowAct < 0) {
			nxCore::dbg_msg("wrench: unknown act %d\n", nowAct);
			return;
		}

		int32_t newAct = -1;
		double newActDuration = 0.0;
		bool wallTouchReset = false;

		WRFunction* pActFunc = wr_getFunction(mpCtx, s_actProgs[nowAct].pFuncName);
		if (pActFunc) {
			int err = wr_callFunction(mpState, mpCtx, pActFunc, nullptr, 0);
			if (err == WR_ERR_None) {
				WRValue* pVal = wr_returnValueFromLastCall(mpState);
				if (pVal) {
					uint32_t aryLen = 0;
					char aryType = 0;
					WRValue* pAryVals = reinterpret_cast<WRValue*>(pVal->array(&aryLen, &aryType));
					const char* pNewActName = pAryVals[0].c_str();
					if (pNewActName) {
						newAct = SmpCharSys::act_id_from_name(pNewActName);
					}
					newActDuration = pAryVals[1].asFloat();
					wallTouchReset = pAryVals[2].asInt();

					if (newAct >=0) {
						pChar->change_act(newAct, newActDuration);
					}

					if (wallTouchReset) {
						pChar->reset_wall_touch();
					}
				}
			}
		}
	}

	ScnObj* get_active_obj() {
		return mpActiveObj;
	}
	SmpChar* get_active_char() {
		return SmpCharSys::char_from_obj(get_active_obj());
	}

	float char_mood_calc(SmpChar* pChar) {
		float mood = 0.0f;
		WRFunction* pMoodFunc = wr_getFunction(mpCtx, "char_mood_calc");
		if (pMoodFunc) {
			int err = wr_callFunction(mpState, mpCtx, pMoodFunc, nullptr, 0);
			if (err == WR_ERR_None) {
				WRValue* pMoodVal = wr_returnValueFromLastCall(mpState);
				if (pMoodVal) {
					mood = pMoodVal->asFloat();
				}
			} else {
				nxCore::dbg_msg("WRENCH: error calling char_mood_calc: %d\n", err);
			}
		}
		return mood;
	}

} s_wrench;

static void wifc_glb_rng_next(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr) {
	uint64_t rnd = Scene::glb_rng_next();
	rnd &= 0xffffffff;
	wr_makeInt(&retVal, rnd);
}

static void wifc_glb_rng_f01(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr) {
	float rnd = Scene::glb_rng_f01();
	wr_makeFloat(&retVal, rnd);
}

static void wifc_check_act_timeout(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr) {
	SmpChar* pChar = s_wrench.get_active_char();
	bool res = pChar->check_act_time_out();
	wr_makeInt(&retVal, (int) res);
}

static void wifc_obj_touch_duration_secs(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr) {
	SmpChar* pChar = s_wrench.get_active_char();
	float duration = pChar->get_obj_touch_duration_secs();
	wr_makeFloat(&retVal, duration);
}

static void wifc_wall_touch_duration_secs(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr) {
	SmpChar* pChar = s_wrench.get_active_char();
	float duration = pChar->get_wall_touch_duration_secs();
	wr_makeFloat(&retVal, duration);
}

static void wifc_get_mood_arg(WRState* w, const WRValue* argv, const int argn, WRValue& retVal, void* usr) {
	float res = 0.0f;
	SmpChar* pChar = s_wrench.get_active_char();
	if (pChar) {
		res = calc_mood_arg(pChar->mMoodTimer);
	}
	wr_makeFloat(&retVal, res);
}


#endif

void init_roam_wrench() {
#if ROAM_WRENCH
	s_wrench.init();
#endif
}

void reset_roam_wrench() {
#if ROAM_WRENCH
	s_wrench.reset();
#endif
}

void roam_ctrl_wrench(SmpChar* pChar) {
#if ROAM_WRENCH
	if (!pChar) return;

	if (s_wrench.mpLock) {
		nxSys::lock_acquire(s_wrench.mpLock);
	}

	s_wrench.mpActiveObj = pChar->mpObj;
	s_wrench.exec();
	s_wrench.mpActiveObj = nullptr;

	if (s_wrench.mpLock) {
		nxSys::lock_release(s_wrench.mpLock);
	}
#endif
}

