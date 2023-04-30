#include "crosscore.hpp"
#include "scene.hpp"
#include "smprig.hpp"
#include "smpchar.hpp"

#include "roam_ctrl.hpp"

void roam_ctrl_native(SmpChar* pChar) {
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
