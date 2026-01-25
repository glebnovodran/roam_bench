inline fn sysCall(no: usize) void {
	asm volatile (
		"ecall"
		:
		: [number] "{a0}" (no)
	);
}

fn _check_act_timeout() void { sysCall(0); }
fn _glb_rng_next() void { sysCall(1); }
fn _glb_rng_f01() void { sysCall(2); }
fn _wall_touch_duration_secs() void { sysCall(3); }
fn _obj_touch_duration_secs() void { sysCall(4); }
fn _get_mood_arg() void { sysCall(5); }

const check_act_timeout: *const fn() i32 = @ptrCast(&_check_act_timeout);
const glb_rng_next: *const fn() u32 = @ptrCast(&_glb_rng_next);
const glb_rng_f01: *const fn() f32 = @ptrCast(&_glb_rng_f01);
const wall_touch_duration_secs: *const fn() f32 = @ptrCast(&_wall_touch_duration_secs);
const obj_touch_duration_secs: *const fn() f32 = @ptrCast(&_obj_touch_duration_secs);
const get_mood_arg: *const fn() f32 = @ptrCast(&_get_mood_arg);

export fn char_mood_calc() f32 {
	const t = get_mood_arg();
	const x = t * 3.14159274 * 2.0;
	const x2 = x*x;
	const x4 = x2*x2;
	const x6 = x4*x2;
	const x8 = x4*x4;
	const x10 = x8*x2;
	const x12 = x10*x2;
	const x14 = x12*x2;
	const x16 = x8*x8;
	return 1.0
	       - 1.0/4.0*x2
	       + 1.0/48.0*x4
	       - 1.0/1440.0*x6
	       + 1.0/80640.0*x8
	       - 1.0/7257600.0*x10
	       + 1.0/958003200.0*x12
	       - 1.0/174356582400.0*x14
	       + 1.0/41845579776000.0*x16;
}

inline fn lerp(a: f32, b: f32, t: f32) f32 {
	return a + (b - a)*t;
}

fn fit(val: f32, oldMin: f32, oldMax: f32, newMin: f32, newMax: f32) f32 {
	const t = oldMax - oldMin;
	if (t == 0.0) return 0.0;
	var rel = (val - oldMin) / t;
	if (rel < 0.0) rel = 0.0;
	if (rel > 1.0) rel = 1.0;
	return lerp(newMin, newMax, rel);
}


const ActInfo = struct {
	pNewActName: ?*const u8,
	newActDuration: f32,
	wallTouchReset: i32,
};

export fn STAND(pInfo: *ActInfo) void {
	pInfo.* = ActInfo {
		.pNewActName = null,
		.newActDuration = 0.0,
		.wallTouchReset = 0
	};

	if (check_act_timeout() != 0) {
		if ((glb_rng_next() & 0x3F) < 0xF) {
			pInfo.pNewActName = @ptrCast("RUN");
			pInfo.newActDuration = 2.0;
		} else {
			pInfo.pNewActName = @ptrCast("WALK");
			pInfo.newActDuration = 5.0;
			pInfo.wallTouchReset = 1;
		}
	}
}

export fn WALK(pInfo: *ActInfo) void {
	const objTouchDT = obj_touch_duration_secs();
	const wallTouchDT = wall_touch_duration_secs();
	pInfo.* = ActInfo {
		.pNewActName = null,
		.newActDuration = 0.0,
		.wallTouchReset = 0
	};

	if (check_act_timeout() != 0 or objTouchDT > 0.2 or wallTouchDT > 0.25) {
		if (objTouchDT > 0.0 and ((glb_rng_next() & 0x3F) < 0x1F)) {
			pInfo.pNewActName = @ptrCast("RETREAT");
			pInfo.newActDuration = 0.5;
		} else {
			const t = fit(glb_rng_f01(), 0.0, 1.0, 0.5, 2.0);
			if ((glb_rng_next() & 1) != 0) {
				pInfo.pNewActName = @ptrCast("TURN_L");
			} else {
				pInfo.pNewActName = @ptrCast("TURN_R");
			}
			pInfo.newActDuration = t;
		}
	}
}

export fn RUN(pInfo: *ActInfo) void {
	const objTouchDT = obj_touch_duration_secs();
	const wallTouchDT = wall_touch_duration_secs();
	pInfo.* = ActInfo {
		.pNewActName = null,
		.newActDuration = 0.0,
		.wallTouchReset = 0
	};

	if (check_act_timeout() != 0 or wallTouchDT > 0.1 or objTouchDT > 0.1) {
		pInfo.pNewActName = @ptrCast("STAND");
		pInfo.newActDuration = 2.0;
	}
}

export fn RETREAT(pInfo: *ActInfo) void {
	const wallTouchDT = wall_touch_duration_secs();
	pInfo.* = ActInfo {
		.pNewActName = null,
		.newActDuration = 0.0,
		.wallTouchReset = 0
	};

	if (check_act_timeout() != 0 or wallTouchDT > 0.1) {
		pInfo.pNewActName = @ptrCast("STAND");
		pInfo.newActDuration = 2.0;
	}
}

export fn TURN(pInfo: *ActInfo) void {
	pInfo.* = ActInfo {
		.pNewActName = null,
		.newActDuration = 0.0,
		.wallTouchReset = 0
	};

	if (check_act_timeout() != 0) {
		pInfo.pNewActName = @ptrCast("STAND");
		pInfo.newActDuration = 1.0;
	}
}



export fn _start() void {
}

