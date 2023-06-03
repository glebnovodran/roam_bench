#include "crosscore.hpp"
#include "scene.hpp"
#include "smprig.hpp"
#include "smpchar.hpp"

#include "roam_ctrl.hpp"

#if ROAM_QJS
#include "quickjs.h"
#include "quickjs-libc.h"

static ScnObj* jsifc_get_active_obj();
static SmpChar* jsifc_get_active_char();

static JSValue jsifc_glb_rng_next(JSContext* pJsctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
	uint64_t rnd = Scene::glb_rng_next();
	rnd &= 0xffffffff;
	return JS_NewInt32(pJsctx, (uint32_t)rnd);
}

static JSValue jsifc_glb_rng_f01(JSContext* pJsctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
	float rnd = Scene::glb_rng_f01();
	return JS_NewFloat64(pJsctx, double(rnd));
}

static JSValue jsifc_check_act_timeout(JSContext* pJsctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
	bool res = false;
	SmpChar* pChar = jsifc_get_active_char();
	if (pChar) {
		res = pChar->check_act_time_out();
	}
	return JS_NewBool(pJsctx, res);
}

static JSValue jsifc_obj_touch_duration_secs(JSContext* pJsctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
	double res = 0.0;
	SmpChar* pChar = jsifc_get_active_char();
	if (pChar) {
		res = pChar->get_obj_touch_duration_secs();
	}
	return JS_NewFloat64(pJsctx, res);
}

static JSValue jsifc_wall_touch_duration_secs(JSContext* pJsctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
	double res = 0.0;
	SmpChar* pChar = jsifc_get_active_char();
	if (pChar) {
		res = pChar->get_wall_touch_duration_secs();
	}
	return JS_NewFloat64(pJsctx, res);
}

static JSValue jsifc_get_mood_arg(JSContext* pJsctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
	double res = 0.0;
	SmpChar* pChar = jsifc_get_active_char();
	if (pChar) {
		res = calc_mood_arg(pChar->mMoodTimer);
	}
	return JS_NewFloat64(pJsctx, res);
}

static JSCFunctionListEntry s_ifc_func_exps[] = {
	JS_CFUNC_DEF("glb_rng_next", 0, jsifc_glb_rng_next),
	JS_CFUNC_DEF("glb_rng_f01", 0, jsifc_glb_rng_f01),
	JS_CFUNC_DEF("check_act_timeout", 0, jsifc_check_act_timeout),
	JS_CFUNC_DEF("obj_touch_duration_secs", 0, jsifc_obj_touch_duration_secs),
	JS_CFUNC_DEF("wall_touch_duration_secs", 0, jsifc_wall_touch_duration_secs),
	JS_CFUNC_DEF("get_mood_arg", 0, jsifc_get_mood_arg)
};

static char* load_js_prog(const char* pName, size_t& srcSize) {
	return Scene::load_text_cstr(pName, &srcSize, "acts", "js");
/*
	char* pProg = nullptr;
	cxResourceManager* pRsrcMgr = Scene::get_rsrc_mgr();
	const char* pDataPath = pRsrcMgr ? pRsrcMgr->get_data_path() : nullptr;

	if (pName) {
		char path[256];
		char* pSrcPath = path;
		size_t bufSize = sizeof(path);
		size_t pathSize = (pDataPath ? nxCore::str_len(pDataPath) : 1) + 6 + nxCore::str_len(pName) + 5 + 1;
		if (bufSize < pathSize) {
			pSrcPath = reinterpret_cast<char*>(nxCore::mem_alloc(pathSize, "chr_jsprog_path"));
			bufSize = pathSize;
		}

		XD_SPRINTF(XD_SPRINTF_BUF(pSrcPath, bufSize), "%s/%s/%s.js", pDataPath ? pDataPath : ".", "acts", pName);
		srcSize = 0;
		pProg = reinterpret_cast<char*>(nxCore::bin_load(pSrcPath, &srcSize, false, true));
		if (pProg) {
			char* pProgCstr = (char*)nxCore::mem_alloc(srcSize + 1, "QJSProgSrc");
			if (pProgCstr) {
				nxCore::mem_copy(pProgCstr, pProg, srcSize);
				pProgCstr[srcSize] = 0;
				nxCore::bin_unload(pProg);
				pProg = pProgCstr;
			}
		}
		if (pSrcPath != path) {
			nxCore::mem_free(pSrcPath);
		}
	}
	return pProg;
*/
}

static struct JSFunc {
	const char* pName;
	JSValue funcVal;
} s_actFuncs[] = {
	{ "STAND", JS_NULL },
	{ "TURN_L", JS_NULL },
	{ "TURN_R", JS_NULL },
	{ "WALK", JS_NULL },
	{ "RETREAT", JS_NULL },
	{ "RUN", JS_NULL }
};

static JSFunc s_moodFunc = { "char_mood_calc", JS_NULL };

static struct ROAM_QJS_WK {
	JSRuntime* mpRT;
	JSContext* mpCtx;
	sxLock* mpLock;
	ScnObj* mpActiveObj;
	char* mpProgSrc;
	size_t mProgSize;
	bool mExecFlg;

	void init() {
		mpRT = JS_NewRuntime();
		mpCtx = nullptr;
		mpActiveObj = nullptr;
		mpProgSrc = nullptr;
		mProgSize = 0;
		mExecFlg = false;

		if (mpRT) {
			JS_SetMemoryLimit(mpRT, 1024*1024);
			JS_SetMaxStackSize(mpRT, 16 * 1024);
			mpCtx = JS_NewContext(mpRT);
			nxCore::dbg_msg("QJS: Runtime: %p, Context: %p\n", mpRT, mpCtx);
		}

		if (mpCtx) {
			JS_SetPropertyFunctionList(mpCtx, JS_GetGlobalObject(mpCtx), s_ifc_func_exps, XD_ARY_LEN(s_ifc_func_exps));
		}

		mpLock = nxSys::lock_create();
		if (mpCtx) {
			mpProgSrc = load_js_prog("acts", mProgSize);
		}

		if (mpProgSrc) {
			nxCore::dbg_msg("QJS: prog loaded - %d bytes\n", mProgSize);
			JSValue res = JS_Eval(mpCtx, mpProgSrc, mProgSize, "", JS_EVAL_TYPE_GLOBAL);

			if (JS_IsException(res)) {
				nxCore::dbg_msg("QJS: src eval error\n");
				js_std_dump_error(mpCtx);
			} else {
				nxCore::dbg_msg("QJS: src OK\n");
				JSValue glbObj = JS_GetGlobalObject(mpCtx);
				for (size_t i = 0; i < XD_ARY_LEN(s_actFuncs); ++i) {
					const char* pFuncName = s_actFuncs[i].pName;
					JSValue func = JS_GetPropertyStr(mpCtx, glbObj, pFuncName);
					if (JS_IsFunction(mpCtx, func)) {
						nxCore::dbg_msg("QJS: func %s OK\n", pFuncName);
						s_actFuncs[i].funcVal = func;
					} else {
						nxCore::dbg_msg("QJS: func %s not found\n", pFuncName);
						s_actFuncs[i].funcVal = JS_NULL;
					}
				}
				JSValue func = JS_GetPropertyStr(mpCtx, glbObj, s_moodFunc.pName);
				if (JS_IsFunction(mpCtx, func)) {
					nxCore::dbg_msg("QJS: func %s OK\n", s_moodFunc.pName);
					s_moodFunc.funcVal = func;
				} else {
					nxCore::dbg_msg("QJS: func %s not found\n", s_moodFunc.pName);
					s_moodFunc.funcVal = JS_NULL;
				}
				mExecFlg = true;
			}
		}
	}

	void reset() {
		if (mpRT) {
			JS_RunGC(mpRT);
		}

		if (mpCtx) {
			JS_FreeContext(mpCtx);
			mpCtx = nullptr;
		}

		if (mpRT) {
			// commented to avoid assertion fired by QJS
			// JS_FreeRuntime(mpRT);
			// mpRT = nullptr;
		}

		Scene::unload_text_cstr(mpProgSrc);
		mpProgSrc = nullptr;
		mProgSize = 0;

		if (mpLock) {
			nxSys::lock_destroy(mpLock);
			mpLock = nullptr;
		}
	}

	double char_mood_calc_js(SmpChar* pChar) {
		double mood = 0.0;
		JSValue func = s_moodFunc.funcVal;
		if (JS_IsFunction(mpCtx, func)) {
			JSValue glbObj = JS_GetGlobalObject(mpCtx);
			JSValue res = JS_Call(mpCtx, func, glbObj, 0, nullptr);
			if (JS_IsNumber(res)) {
				mood = JS_VALUE_GET_FLOAT64(res);
			} else {
				nxCore::dbg_msg("QJS: invalid func res for %s\n", s_moodFunc.pName);
				if (JS_IsException(res)) {
					js_std_dump_error(mpCtx);
				}
			}

		} else {
			nxCore::dbg_msg("QJS: no func for %s\n", s_moodFunc.pName);
		}

		return mood;
	}

	void exec() {
		if (!mpCtx) return;
		if (!mExecFlg) return;
		if (!mpActiveObj) return;

		SmpChar* pChar = SmpCharSys::char_from_obj(mpActiveObj);
		if (!pChar) return;

		if (is_mood_enabled()) {
			pChar->mood_update();
			pChar->mMood = nxCalc::saturate(char_mood_calc_js(pChar));
			if (is_mood_visible() && pChar->mpObj) {
				float moodc = pChar->mMood;
				pChar->mpObj->set_base_color_scl(moodc, moodc, moodc);
			}
		}

		int32_t nowAct = pChar->mAction;
		if (nowAct < 0 || nowAct >= XD_ARY_LEN(s_actFuncs)) {
			nxCore::dbg_msg("QJS: unknown act %d\n", nowAct);
			return;
		}

		JSValue func = s_actFuncs[nowAct].funcVal;
		if (!JS_IsFunction(mpCtx, func)) {
			nxCore::dbg_msg("QJS: no func for act %d\n", nowAct);
			return;
		}
		JSValue glbObj = JS_GetGlobalObject(mpCtx);
		JSValue res = JS_Call(mpCtx, func, glbObj, 0, nullptr);
		if (!JS_IsObject(res)) {
			nxCore::dbg_msg("QJS: invalid func res for act %d\n", nowAct);
			if (JS_IsException(res)) {
				js_std_dump_error(mpCtx);
			}
			return;
		}

		int32_t newAct = -1;
		double newActDuration = 0.0;
		bool wallTouchReset = false;
		JSValue newActNameVal = JS_GetPropertyStr(mpCtx, res, "newActName");
		if (JS_IsString(newActNameVal)) {
			size_t lenNewActName = 0;
			const char* pNewActName = JS_ToCStringLen(mpCtx, &lenNewActName, newActNameVal);
			if (pNewActName && lenNewActName > 0) {
				newAct = SmpCharSys::act_id_from_name(pNewActName);
				JS_FreeCString(mpCtx, pNewActName);
				newActDuration = 1.0; // default
				JSValue newActDurVal = JS_GetPropertyStr(mpCtx, res, "newActDuration");
				if (JS_IsNumber(newActDurVal)) {
					double duration = 0.0;
					if (JS_ToFloat64(mpCtx, &duration, newActDurVal) == 0) {
						newActDuration = duration;
					}
				}
			}
		}

		JSValue wallResetVal = JS_GetPropertyStr(mpCtx, res, "wallTouchReset");
		if (JS_IsBool(wallResetVal)) {
			if (JS_ToBool(mpCtx, wallResetVal) == 1) {
				wallTouchReset = true;
			}
		}

		if (newAct >= 0) {
			pChar->change_act(newAct, newActDuration);
		}
		if (wallTouchReset) {
			pChar->reset_wall_touch();
		}

		JS_FreeValue(mpCtx, res);
	}

} s_js = {};

static ScnObj* jsifc_get_active_obj() {
	return s_js.mpActiveObj;
}

static SmpChar* jsifc_get_active_char() {
	return SmpCharSys::char_from_obj(jsifc_get_active_obj());
}

#endif /* ROAM_QJS */

void init_roam_qjs() {
#if ROAM_QJS
	s_js.init();
#endif
}

void reset_roam_qjs() {
#if ROAM_QJS
	s_js.reset();
#endif
}

void roam_ctrl_qjs(SmpChar* pChar) {
#if ROAM_QJS
	if (!pChar) return;

	if (s_js.mpLock) {
		nxSys::lock_acquire(s_js.mpLock);
	}

	s_js.mpActiveObj = pChar->mpObj;
	s_js.exec();
	s_js.mpActiveObj = nullptr;

	if (s_js.mpLock) {
		nxSys::lock_release(s_js.mpLock);
	}
#endif
}
