#include <stddef.h>
#include <stdint.h>

typedef struct _ACT_INFO {
	const char* pNewActName;
	float newActDuration;
	int wallTouchReset;
} ACT_INFO;

#define SYS_FN __attribute__((naked))
#define SYS_CALL(_no) __asm volatile ("li a0, " #_no "\necall\nret")

SYS_FN int check_act_timeout()          { SYS_CALL(0); }
SYS_FN uint32_t glb_rng_next()          { SYS_CALL(1); }
SYS_FN float glb_rng_f01()              { SYS_CALL(2); }
SYS_FN float wall_touch_duration_secs() { SYS_CALL(3); }
SYS_FN float obj_touch_duration_secs()  { SYS_CALL(4); }
SYS_FN float get_mood_arg()             { SYS_CALL(5); }

float lerp(float a, float b, float t) {
	return a + (b - a)*t;
}

float fit(float val, float oldMin, float oldMax, float newMin, float newMax) {
	float rel;
	float t = oldMax - oldMin;
	if (t == 0.0) return 0.0f;
	rel = (val - oldMin) / t;
	if (rel < 0.0f) rel = 0.0f;
	if (rel > 1.0f) rel = 1.0f;
	return lerp(newMin, newMax, rel);
}

float char_mood_calc() {
	float t = get_mood_arg();
	float x = t * 3.14159274f * 2.0f;
	float x2 = x*x;
	float x4 = x2*x2;
	float x6 = x4*x2;
	float x8 = x4*x4;
	float x10 = x8*x2;
	float x12 = x10*x2;
	float x14 = x12*x2;
	float x16 = x8*x8;
	float y = 1.0f
	         - 1.0f/4.0f*x2
	         + 1.0f/48.0f*x4
	         - 1.0f/1440.0f*x6
	         + 1.0f/80640.0f*x8
	         - 1.0f/7257600.0f*x10
	         + 1.0f/958003200.0f*x12
	         - 1.0f/174356582400.0f*x14
	         + 1.0f/41845579776000.0f*x16;
	return y;
}

void STAND(ACT_INFO* pInfo) {
	pInfo->pNewActName = NULL;
	pInfo->newActDuration = 0.0f;
	pInfo->wallTouchReset = 0;

	if (check_act_timeout()) {
		if ((glb_rng_next() & 0x3F) < 0xF) {
			pInfo->pNewActName = "RUN";
			pInfo->newActDuration = 2.0f;
		} else {
			pInfo->pNewActName = "WALK";
			pInfo->newActDuration = 5.0f;
			pInfo->wallTouchReset = 1;
		}
	}
}

void WALK(ACT_INFO* pInfo) {
	float objTouchDT = obj_touch_duration_secs();
	float wallTouchDT = wall_touch_duration_secs();
	pInfo->pNewActName = NULL;
	pInfo->newActDuration = 0.0f;
	pInfo->wallTouchReset = 0;

	if (check_act_timeout() || objTouchDT > 0.2f || wallTouchDT > 0.25f) {
		if (objTouchDT > 0 && ((glb_rng_next() & 0x3F) < 0x1F)) {
			pInfo->pNewActName = "RETREAT";
			pInfo->newActDuration = 0.5f;
		} else {
			float t = fit(glb_rng_f01(), 0.0f, 1.0f, 0.5f, 2.0f);
			if (glb_rng_next() & 1) {
				pInfo->pNewActName = "TURN_L";
			} else {
				pInfo->pNewActName = "TURN_R";
			}
			pInfo->newActDuration = t;
		}
	}
}

void RUN(ACT_INFO* pInfo) {
	float objTouchDT = obj_touch_duration_secs();
	float wallTouchDT = wall_touch_duration_secs();
	pInfo->pNewActName = NULL;
	pInfo->newActDuration = 0.0f;
	pInfo->wallTouchReset = 0;

	if (check_act_timeout() || wallTouchDT > 0.1 || objTouchDT > 0.1) {
		pInfo->pNewActName = "STAND";
		pInfo->newActDuration = 2.0f;
	}
}

void RETREAT(ACT_INFO* pInfo) {
	float objTouchDT = obj_touch_duration_secs();
	float wallTouchDT = wall_touch_duration_secs();
	pInfo->pNewActName = NULL;
	pInfo->newActDuration = 0.0f;
	pInfo->wallTouchReset = 0;

	if (check_act_timeout() || wallTouchDT > 0.1) {
		pInfo->pNewActName = "STAND";
		pInfo->newActDuration = 2.0f;
	}
}

void TURN(ACT_INFO* pInfo) {
	if (check_act_timeout()) {
		pInfo->pNewActName = "STAND";
		pInfo->newActDuration = 1.0f;
	}
}
