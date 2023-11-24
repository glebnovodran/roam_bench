#include "crosscore.hpp"
#include "scene.hpp"
#include "smprig.hpp"
#include "smpchar.hpp"

#include "roam_ctrl.hpp"

#define ROAM_MINION 1

#if ROAM_MINION
#include <minion.h>

static struct ActMiProg {
	const char* pFuncName;
	int ifn;
} s_actMiProgs[] = {
	{"STAND", -1},    // ACT_STAND
	{"TURN", -1},     // ACT_TURN_L
	{"TURN", -1},     // ACT_TURN_R
	{"WALK", -1},     // ACT_WALK
	{"RETREAT", -1},  // ACT_RETREAT
	{"RUN", -1},      // ACT_RUN
};

static void exec_from_pc(MINION* pMi) {
	while (1) {
		uint32_t instr = minion_fetch_pc_instr(pMi);
		minion_instr(pMi, instr, MINION_IMODE_EXEC);
		if (pMi->pcStatus & MINION_PCSTATUS_NATIVE) {
			break;
		}
	}
}

static float get_mood_arg(MINION* pMi) {
	float argVal = 0.0f;
	if (pMi) {
		SmpChar* pChar = reinterpret_cast<SmpChar*>(pMi->pUser);
		if (pChar) {
			double nowTime = pChar->mMoodTimer;
			argVal = calc_mood_arg(nowTime);
		}
	}
	return argVal;
}

void roam_ecall(MINION* pMi) {
	int fn = minion_get_a0(pMi);
	switch (fn) {
		case 5:
			minion_set_fa0_s(pMi, get_mood_arg(pMi));
			break;
	}
}

static struct RoamMinionWk {
	MINION_BIN* mpMiBin;
	ActMiProg* mpActProgs;
	size_t mNumActProgs;
	int mMoodFn;

	void init() {
		mpMiBin = nxCore::tMem<MINION_BIN>::alloc();
		if (!mpMiBin) return;

		memset(mpMiBin, 0, sizeof(MINION_BIN));

		size_t roamBinSz = 0;
		void* pRoamBin = Scene::load_bin_file("roam", &roamBinSz, "acts", "minion");
		if (!pRoamBin) {
			nxCore::tMem<MINION_BIN>::free(mpMiBin);
			mpMiBin = nullptr;
			return;
		}

		minion_bin_from_mem(mpMiBin, pRoamBin, roamBinSz);

		MINION mi;
		memset(&mi, 0, sizeof(mi));
		minion_init(&mi, mpMiBin);
		mMoodFn = minion_find_func(&mi, "char_mood_calc");
		minion_release(&mi);
	}

	void reset() {
		minion_bin_free(mpMiBin);
		nxCore::tMem<MINION_BIN>::free(mpMiBin);
	}

	void init(SmpChar* pChar) {
		if (pChar) {
			MINION* pMi = nxCore::tMem<MINION>::alloc();
			if (pMi) {
				memset(pMi, 0, sizeof(MINION));
				minion_init(pMi, mpMiBin);
				pMi->ecall_fn = roam_ecall;
				pMi->pUser = pChar;

				pChar->set_ptr_wk<MINION>(0, pMi);
			}
		}
	}
	void reset(SmpChar* pChar) {
		if (pChar) {
			MINION* pMi = pChar->get_ptr_wk<MINION>(0);
			if (pMi) {
				minion_release(pMi);
				nxCore::tMem<MINION>::free(pMi);
			}
		}
	}

} s_minion = {
	nullptr, s_actMiProgs, XD_ARY_LEN(s_actMiProgs), -1
};

#endif

bool chr_exec_init_minion_func(ScnObj* pObj, void* pWkMem) {
#if ROAM_MINION
	SmpChar* pChar = SmpCharSys::char_from_obj(pObj);
	if (pChar) {
		s_minion.init(pChar);
	}
#endif
	return true;
}

bool chr_exec_reset_minion_func(ScnObj* pObj, void* pWkMem) {
#if ROAM_MINION
	SmpChar* pChar = SmpCharSys::char_from_obj(pObj);
	if (pChar) {
		s_minion.reset(pChar);
	}
#endif
	return true;
}

void init_roam_minion() {
#if ROAM_MINION
	s_minion.init();
#endif
}

void reset_roam_minion() {
#if ROAM_MINION
	s_minion.reset();
#endif
}

static float char_mood_calc_minion(SmpChar* pChar) {
	float mood = 0.0f;
	if (pChar) {
		MINION* pMi = pChar->get_ptr_wk<MINION>(0);
		if (pMi) {
			if (s_minion.mMoodFn >= 0) {
				minion_set_pc_to_func_idx(pMi, s_minion.mMoodFn);
				exec_from_pc(pMi);
				mood = minion_get_fa0_s(pMi);
				minion_msg(pMi, "Mood : %.3f\n", mood);
			}
		}
	}

	return mood;
}

void roam_ctrl_minion(SmpChar* pChar) {
#if ROAM_MINION
	if (!pChar) return;

	if (is_mood_enabled()) {
		pChar->mood_update();
		pChar->mMood = nxCalc::saturate(char_mood_calc_minion(pChar));
		if (is_mood_visible() && pChar->mpObj) {
			float moodc = pChar->mMood;
			pChar->mpObj->set_base_color_scl(moodc, moodc, moodc);
		}
	}

	if (is_roamctrl_disabled()) return;


#endif
}