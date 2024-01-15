#include "crosscore.hpp"
#include "scene.hpp"
#include "smprig.hpp"
#include "smpchar.hpp"

#include "roam_ctrl.hpp"

#if ROAM_MINION
#include <minion.h>

struct ActInfo {
        uint32_t vptrNewActName;
        float newActDuration;
        int wallTouchReset;
};

struct MinionMachine {
  MINION minion;
  ActInfo actInfo;
  uint32_t vptrActInfo;
};

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
	while (true) {
		uint32_t instr = minion_fetch_pc_instr(pMi);
		minion_instr(pMi, instr, MINION_IMODE_EXEC);
		if (pMi->pcStatus & MINION_PCSTATUS_NATIVE) {
			break;
		}
	}
}

static int mifc_check_act_timeout(MINION* pMi) {
	int timeout = 0;
	if (pMi) {
		SmpChar* pChar = reinterpret_cast<SmpChar*>(pMi->pUser);
		timeout = pChar->check_act_time_out();
	}
	return timeout;
}

static uint32_t mifc_glb_rng_next(MINION* pMi) {
	uint64_t rnd = Scene::glb_rng_next();
	rnd &= 0xffffffff;
	return uint32_t(rnd);
}

static float mifc_glb_rng_f01(MINION* pMi) {
	float rnd = Scene::glb_rng_f01();
	return rnd;
}

static float mifc_wall_touch_duration_secs(MINION* pMi) {
	float res = 0.0f;
	if (pMi) {
		SmpChar* pChar = reinterpret_cast<SmpChar*>(pMi->pUser);
		res = float(pChar->get_wall_touch_duration_secs());
	}
	return res;
}

static float mifc_obj_touch_duration_secs(MINION* pMi) {
	float res = 0.0f;
	if (pMi) {
		SmpChar* pChar = reinterpret_cast<SmpChar*>(pMi->pUser);
		res = float(pChar->get_obj_touch_duration_secs());
	}
	return res;
}

static float mifc_get_mood_arg(MINION* pMi) {
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
		case 0:
			minion_set_a0(pMi, mifc_check_act_timeout(pMi));
			break;
		case 1:
			minion_set_a0(pMi, mifc_glb_rng_next(pMi));
			break;
		case 2:
			minion_set_fa0_s(pMi, mifc_glb_rng_f01(pMi));
			break;
		case 3:
			minion_set_fa0_s(pMi, mifc_wall_touch_duration_secs(pMi));
			break;
		case 4:
			minion_set_fa0_s(pMi, mifc_obj_touch_duration_secs(pMi));
			break;
		case 5:
			minion_set_fa0_s(pMi, mifc_get_mood_arg(pMi));
			break;
	}
}

static struct RoamMinionWk {
	MINION_BIN* mpMiBin = { nullptr };
	MinionMachine* mpMachines = { nullptr };
	ActMiProg* mpActProgs { s_actMiProgs };
	size_t mNumActProgs { XD_ARY_LEN(s_actMiProgs) };
	int mMoodFn { -1 };

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
		Scene::unload_bin_file(pRoamBin);

		int numMachines = Scene::get_num_per_worker_blocks();
		mpMachines = nxCore::tMem<MinionMachine>::alloc(numMachines);

		if (mpMachines) {
			for (int i = 0; i < numMachines; ++i) {
				MinionMachine* pMachine = &mpMachines[i];
				MINION* pMi = &pMachine->minion;
				nxCore::mem_zero(pMachine, sizeof(MinionMachine));
				minion_init(pMi, mpMiBin);
				pMi->ecall_fn = roam_ecall;
				pMachine->vptrActInfo = minion_mem_map(pMi, &pMachine->actInfo, sizeof(ActInfo));
			}
		}
		mMoodFn = minion_bin_find_func(mpMiBin, "char_mood_calc");
		for (int i = 0; i < mNumActProgs; ++i) {
			mpActProgs[i].ifn = minion_bin_find_func(mpMiBin, mpActProgs[i].pFuncName);
		}
	}

	void reset() {
		if (mpMachines) {
			int numMachines = Scene::get_num_per_worker_blocks();
			for (int i = 0; i < numMachines; ++i) {
				MinionMachine* pMachine = &mpMachines[i];
				MINION* pMi = &pMachine->minion;
				minion_mem_unmap(pMi, pMachine->vptrActInfo);
				minion_release(pMi);
			}
			nxCore::tMem<MinionMachine>::free(mpMachines, numMachines);
		}
		if (mpMiBin) {
			minion_bin_free(mpMiBin);
			nxCore::tMem<MINION_BIN>::free(mpMiBin);
		}
	}

	MinionMachine* get_machine(const SmpChar* pChar) {
		int wkid = pChar->get_worker_id();
		return wkid < 0 ? &mpMachines[0] : &mpMachines[wkid];
	}

} s_minion;

#endif

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
#if ROAM_MINION
	if (pChar) {
		MinionMachine* pMachine = s_minion.get_machine(pChar);
		MINION* pMi = &pMachine->minion;
		if (pMi) {
			if (s_minion.mMoodFn >= 0) {
				minion_set_pc_to_func_idx(pMi, s_minion.mMoodFn);
				exec_from_pc(pMi);
				mood = minion_get_fa0_s(pMi);
			}
		}
	}
#endif

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

	MinionMachine* pMachine = s_minion.get_machine(pChar);
	if (pMachine) {
		MINION* pMi = &pMachine->minion;
		pMi->pUser = pChar;
		size_t numActs = s_minion.mNumActProgs;
		int32_t nowAct = pChar->mAction;
		if ((nowAct >= 0) && (nowAct < numActs)) {
			int32_t ifn = s_actMiProgs[nowAct].ifn;
			minion_set_a0(pMi, pMachine->vptrActInfo);
			minion_set_pc_to_func_idx(pMi, ifn);

			exec_from_pc(pMi);
			ActInfo* pActInfo = &pMachine->actInfo;
			char* pNewActName = reinterpret_cast<char*>(minion_resolve_vptr(pMi, pActInfo->vptrNewActName));
			if (pNewActName) {
				int newActId = SmpCharSys::act_id_from_name(pNewActName);
				if (newActId >= 0) {
					pChar->change_act(newActId, pActInfo->newActDuration);
				}
			}

			if (pActInfo->wallTouchReset) {
				pChar->reset_wall_touch();
			}
		}
	}
#endif
}