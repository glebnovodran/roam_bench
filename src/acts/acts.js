function lerp(a, b, t) {
	return a + (b - a)*t;
}

function fit(val, oldMin, oldMax, newMin, newMax) {
	let t = oldMax - oldMin;
	if (t == 0.0) return 0.0;
	rel = (val - oldMin) / t;
	if (rel < 0.0) rel = 0.0;
	if (rel > 1.0) rel = 1.0;
	return lerp(newMin, newMax, rel);
}

function STAND() {
	let newAct = "";
	let newDuration = 0.0;
	let wallReset = false;
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
	return { newActName: newAct, newActDuration: newDuration, wallTouchReset: wallReset };
}

function WALK() {
	let newAct = "";
	let newDuration = 0.0;
	let wallReset = false;
	let objTouchDT = obj_touch_duration_secs();
	let wallTouchDT = wall_touch_duration_secs();
	if (check_act_timeout() || objTouchDT > 0.2 || wallTouchDT > 0.25) {
		if (objTouchDT > 0 && ((glb_rng_next() & 0x3F) < 0x1F)) {
			newAct = "RETREAT";
			newDuration = 0.5;
		} else {
			let t = fit(glb_rng_f01(), 0.0, 1.0, 0.5, 2.0);
			if (glb_rng_next() & 1) {
				newAct = "TURN_L";
			} else {
				newAct = "TURN_R";
			}
			newDuration = t;
		}
	}
	return { newActName: newAct, newActDuration: newDuration, wallTouchReset: wallReset };
}

function RUN() {
	let newAct = "";
	let newDuration = 0.0;
	let wallReset = false;
	let objTouchDT = obj_touch_duration_secs();
	let wallTouchDT = wall_touch_duration_secs();
	if (check_act_timeout() || objTouchDT > 0.1 || wallTouchDT > 0.1) {
		newAct = "STAND";
		newDuration = 2.0;
	}
	return { newActName: newAct, newActDuration: newDuration, wallTouchReset: wallReset };
}

function RETREAT() {
	let newAct = "";
	let newDuration = 0.0;
	let wallReset = false;
	let wallTouchDT = wall_touch_duration_secs();
	if (check_act_timeout() || wallTouchDT > 0.1) {
		newAct = "STAND";
		newDuration = 2.0;
	}
	return { newActName: newAct, newActDuration: newDuration, wallTouchReset: wallReset };
}

function TURN_L() {
	let newAct = "";
	let newDuration = 0.0;
	let wallReset = false;
	if (check_act_timeout()) {
		newAct = "STAND";
		newDuration = 1.0;
	}
	return { newActName: newAct, newActDuration: newDuration, wallTouchReset: wallReset };
}

function TURN_R() {
	let newAct = "";
	let newDuration = 0.0;
	let wallReset = false;
	if (check_act_timeout()) {
		newAct = "STAND";
		newDuration = 1.0;
	}
	return { newActName: newAct, newActDuration: newDuration, wallTouchReset: wallReset };
}


