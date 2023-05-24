#include "crosscore.hpp"
#include "scene.hpp"
#include "smprig.hpp"
#include "smpchar.hpp"

#include "roam_ctrl.hpp"


#if ROAM_LUA

#include "lua.hpp"
#include "lauxlib.h"

static char* load_lua_prog(const char* pName, size_t& srcSize) {
	char* pProg = nullptr;
	cxResourceManager* pRsrcMgr = Scene::get_rsrc_mgr();
	const char* pDataPath = pRsrcMgr ? pRsrcMgr->get_data_path() : nullptr;

	if (pName) {
		char path[256];
		char* pSrcPath = path;
		size_t bufSize = sizeof(path);
		size_t pathSize = (pDataPath ? nxCore::str_len(pDataPath) : 1) + 6 + nxCore::str_len(pName) + 5 + 1;
		if (bufSize < pathSize) {
			pSrcPath = reinterpret_cast<char*>(nxCore::mem_alloc(pathSize, "chr_luaprog_path"));
			bufSize = pathSize;
		}

		XD_SPRINTF(XD_SPRINTF_BUF(pSrcPath, bufSize), "%s/%s/%s.lua", pDataPath ? pDataPath : ".", "acts", pName);
		srcSize = 0;
		pProg = reinterpret_cast<char*>(nxCore::bin_load(pSrcPath, &srcSize, false, true));
		if (pProg && srcSize > 0) {
			char* pProgCstr = (char*)nxCore::mem_alloc(srcSize + 1, "LuaProgSrc");
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
}

static ScnObj* luaifc_get_active_obj();
static SmpChar* luaifc_get_active_char();

static int luaifc_glb_rng_next(lua_State* pLua);
static int luaifc_glb_rng_f01(lua_State* pLua);
static int luaifc_check_act_timeout(lua_State* pLua);
static int luaifc_obj_touch_duration_secs(lua_State* pLua);
static int luaifc_wall_touch_duration_secs(lua_State* pLua);
static int luaifc_get_mood_arg(lua_State* pLua);

static struct ActLuaProg {
	const char* pFuncName;
} s_actProgs[] {
	"STAND", "TURN_L", "TURN_R", "WALK", "RETREAT", "RUN"
};

static struct RoamLuaWk {
	lua_State* mpLua;
	//sxLock* mpLock;
	ScnObj* mpActiveObj;
	char* mpProgSrc;
	bool mExecFlg;

	void register_ifc_funcs() {
		if (mpLua) {
			lua_register(mpLua, "glb_rng_next", luaifc_glb_rng_next);
			lua_register(mpLua, "glb_rng_f01", luaifc_glb_rng_f01);
			lua_register(mpLua, "check_act_timeout", luaifc_check_act_timeout);
			lua_register(mpLua, "obj_touch_duration_secs", luaifc_obj_touch_duration_secs);
			lua_register(mpLua, "wall_touch_duration_secs", luaifc_wall_touch_duration_secs);
			lua_register(mpLua, "get_mood_arg", luaifc_get_mood_arg);
		}
	}

	void init() {
		mpLua = luaL_newstate();
		mpActiveObj = nullptr;
		mpProgSrc = nullptr;
		mExecFlg = false;

		if (mpLua) {
			nxCore::dbg_msg("Lua state: %p\n", mpLua);
			register_ifc_funcs();

			size_t srcSize = 0;
			mpProgSrc = load_lua_prog("acts", srcSize);
			int luaErr = luaL_dostring(mpLua, mpProgSrc);
			if (luaErr == LUA_OK) {
				nxCore::dbg_msg("Lua prog run successfully\n");
				mExecFlg = true;
			} else {
				nxCore::dbg_msg("Lua error: %d\n", luaErr);
			}
		} else {
			nxCore::dbg_msg("Failed to init Lua\n");
		}
	}

	void reset() {
		if (mpLua) {
			lua_close(mpLua);
			mpLua = nullptr;
		}
		if (mpProgSrc) {
			nxCore::bin_unload(mpProgSrc);
		}
	}

	double char_mood_calc_lua(SmpChar* pChar) {
		double mood = 0.0;
		lua_getglobal(mpLua, "mood");
		lua_call(mpLua, 0, 1);
		mood = lua_tonumber(mpLua, -1);
		lua_pop(mpLua, 1);
		return mood;
	}

	void exec() {
		if (!mExecFlg) return;
		if (!mpActiveObj) return;
		if (!mpLua) return;

		SmpChar* pChar = SmpCharSys::char_from_obj(mpActiveObj);
		if (!pChar) return;

		if (is_mood_enabled()) {
			pChar->mood_update();
			pChar->mMood = nxCalc::saturate(char_mood_calc_lua(pChar));
			if (is_mood_visible() && pChar->mpObj) {
				float moodc = pChar->mMood;
				pChar->mpObj->set_base_color_scl(moodc, moodc, moodc);
			}
		}

		int32_t nowAct = pChar->mAction;
		if (nowAct >= XD_ARY_LEN(s_actProgs) || nowAct < 0) {
			nxCore::dbg_msg("Lua: unknown act %d\n", nowAct);
			return;
		}

		lua_getglobal(mpLua, s_actProgs[nowAct].pFuncName);
		lua_call(mpLua, 0, 3);

		int32_t newAct = -1;
		double newActDuration = 0.0;
		bool wallTouchReset = false;

		size_t nameSz = 0;
		const char* pNewActName = lua_tolstring(mpLua, 1, &nameSz);
		newActDuration = lua_tonumber(mpLua, 2);
		wallTouchReset = lua_toboolean(mpLua, 3);

		lua_pop(mpLua, 3);

		if (nameSz > 0 && pNewActName) {
			newAct = SmpCharSys::act_id_from_name(pNewActName);
		}

		if (newAct >= 0) {
			pChar->change_act(newAct, newActDuration);
		}

		if (wallTouchReset) {
			pChar->reset_wall_touch();
		}
	}
} s_lua;

static ScnObj* luaifc_get_active_obj() {
	return s_lua.mpActiveObj;
}

static SmpChar* luaifc_get_active_char() {
	return SmpCharSys::char_from_obj(luaifc_get_active_obj());
}

static int luaifc_glb_rng_next(lua_State* pLua) {
	uint64_t rnd = Scene::glb_rng_next();
	rnd &= 0xffffffff;
	
	lua_pushinteger(pLua, rnd);
	return 1;
}

static int luaifc_glb_rng_f01(lua_State* pLua) {
	float rnd = Scene::glb_rng_f01();

	lua_pushnumber(pLua, rnd);
	return 1;
}

static int luaifc_check_act_timeout(lua_State* pLua) {
	SmpChar* pChar = luaifc_get_active_char();
	bool res = pChar->check_act_time_out();

	lua_pushboolean(pLua, res);
	return 1;
}

static int luaifc_obj_touch_duration_secs(lua_State* pLua) {
	SmpChar* pChar = luaifc_get_active_char();
	double res = pChar->get_obj_touch_duration_secs();

	lua_pushnumber(pLua, res);
	return 1;
}

static int luaifc_wall_touch_duration_secs(lua_State* pLua) {
	SmpChar* pChar = luaifc_get_active_char();
	double res = pChar->get_wall_touch_duration_secs();

	lua_pushnumber(pLua, res);
	return 1;
}

static int luaifc_get_mood_arg(lua_State* pLua) {
	float res = 0.0f;
	SmpChar* pChar = luaifc_get_active_char();
	if (pChar) {
		res = calc_mood_arg(pChar->mMoodTimer);
	}
	lua_pushnumber(pLua, res);
	return 1;
}

#endif

void init_roam_lua() {
#if ROAM_LUA
	s_lua.init();
#endif
}

void reset_roam_lua() {
#if ROAM_LUA
	s_lua.reset();
#endif
}

void roam_ctrl_lua(SmpChar* pChar) {
#if ROAM_LUA
	s_lua.mpActiveObj = pChar->mpObj;
	s_lua.exec();
	s_lua.mpActiveObj = nullptr;
#endif
}