function lerp(a, b, t) {
	return a + (b - a)*t;
}

function fit(val, oldMin, oldMax, newMin, newMax) {
	t = oldMax - oldMin;
	if (t == 0.0) {
		return 0.0;
	}
	rel = (val - oldMin) / t;
	if (rel < 0.0) {
		rel = 0.0;
	}
	if (rel > 1.0) {
		rel = 1.0;
	}
	return lerp(newMin, newMax, rel);
}

function char_mood_calc() {
	t = get_mood_arg();
	x = t * 6.2831853072;
	x2 = x*x;
	x4 = x2*x2;
	x6 = x4*x2;
	x8 = x4*x4;
	x10 = x8*x2;
	x12 = x10*x2;
	x14 = x12*x2;
	x16 = x8*x8;

	y = 1.0 - 0.25*x2 + 0.02083333*x4 - 0.00069444*x6 + 0.0000124007936508*x8 - 0.000000137787*x10 + 0.000000001044*x12 - 0.00000000000574*x14 + 0.0000000000000239*x16;
	return y;
}

function STAND() {
	newAct = "";
	newDuration = 0.0;
	wallReset = false;
	if (check_act_timeout()) {
		if ((glb_rng_next() & 0x3F) < 0xF) {
			newAct = "RUN";
			newDuration = 2.0;
		} else {
			newAct = "WALK";
			newDuration = 5.0;
			wallReset = true;
		}
	}
	newState[] = {newAct, newDuration, wallReset};
	return newState;
}

function WALK() {
	newAct = "";
	newDuration = 0.0;
	wallReset = false;

	objTouchDT = obj_touch_duration_secs();
	wallTouchDT = wall_touch_duration_secs();
	if (check_act_timeout() || objTouchDT > 0.2 || wallTouchDT > 0.25) {
		if (objTouchDT > 0 && ((glb_rng_next() & 0x3F) < 0x1F)) {
			newAct = "RETREAT";
			newDuration = 0.5;
		} else {
			t = fit(glb_rng_f01(), 0.0, 1.0, 0.5, 2.0);
			if (glb_rng_next() & 1) {
				newAct = "TURN_L";
			} else {
				newAct = "TURN_R";
			}
			newDuration = t;
		}
	}
	newState[] = {newAct, newDuration, wallReset};
	return newState;
}

function RUN() {
	newAct = "";
	newDuration = 0.0;
	wallReset = false;

	objTouchDT = obj_touch_duration_secs();
	wallTouchDT = wall_touch_duration_secs();

	if (check_act_timeout() || objTouchDT > 0.1 || wallTouchDT > 0.1) {
		newAct = "STAND";
		newDuration = 2.0;
	}
	newState[] = {newAct, newDuration, wallReset};
	return newState;
}

function RETREAT() {
	newAct = "";
	newDuration = 0.0;
	wallReset = false;
	wallTouchDT = wall_touch_duration_secs();
	if (check_act_timeout() || wallTouchDT > 0.1) {
		newAct = "STAND";
		newDuration = 2.0;
	}
	newState[] = {newAct, newDuration, wallReset};
	return newState;
}

function TURN_L() {
	newAct = "";
	newDuration = 0.0;
	wallReset = false;
	if (check_act_timeout()) {
		newAct = "STAND";
		newDuration = 1.0;
	}
	newState[] = {newAct, newDuration, wallReset};
	return newState;
}

function TURN_R() {
	newAct = "";
	newDuration = 0.0;
	wallReset = false;
	if (check_act_timeout()) {
		newAct = "STAND";
		newDuration = 1.0;
	}
	newState[] = {newAct, newDuration, wallReset};
	return newState;
}
