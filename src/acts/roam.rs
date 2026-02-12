#![no_std]
#![no_main]
#![allow(dead_code)]

use core::ffi::c_char;

fn sys_call_i32(no: usize) -> i32 {
	let res: i32;
	unsafe { core::arch::asm!(
		"ecall",
		in("a0") no,
		lateout("a0") res,
	); }
	res
}

fn sys_call_u32(no: usize) -> u32 {
	let res: u32;
	unsafe { core::arch::asm!(
		"ecall",
		in("a0") no,
		lateout("a0") res,
	); }
	res
}

fn sys_call_f32(no: usize) -> f32 {
	let res: f32;
	unsafe { core::arch::asm!(
		"ecall",
		in("a0") no,
		out("fa0") res,
	); }
	res
}

fn check_act_timeout() -> i32 { sys_call_i32(0) }
fn glb_rng_next() -> u32 { sys_call_u32(1) }
fn glb_rng_f01() -> f32 { sys_call_f32(2) }
fn wall_touch_duration_secs() -> f32 { sys_call_f32(3) }
fn obj_touch_duration_secs() -> f32 { sys_call_f32(4) }
fn get_mood_arg() -> f32 { sys_call_f32(5) }

#[no_mangle]
pub extern "C" fn char_mood_calc() -> f32 {
	let t = get_mood_arg();
	let x = t * core::f32::consts::PI * 2.0;
	let x2 = x*x;
	let x4 = x2*x2;
	let x6 = x4*x2;
	let x8 = x4*x4;
	let x10 = x8*x2;
	let x12 = x10*x2;
	let x14 = x12*x2;
	let x16 = x8*x8;
	1.0
	- 1.0/4.0 * x2
 	+ 1.0/48.0 * x4
	- 1.0/1440.0 * x6
	+ 1.0/80640.0 * x8
	- 1.0/7257600.0 * x10
	+ 1.0/958003200.0 * x12
	- 1.0/174356582400.0 * x14
	+ 1.0/41845579776000.0 * x16
}

fn lerp(a: f32, b: f32, t: f32) -> f32 { a + (b - a)*t }
fn fit(val: f32,
       old_min: f32, old_max: f32,
       new_min: f32, new_max: f32) -> f32 {
	let t = old_max - old_min;
	if t == 0.0 { return 0.0; }
	let rel = ((val - old_min) / t).clamp(0.0, 1.0);
	lerp(new_min, new_max, rel)
}

#[repr(C)]
pub struct ActInfo {
	pub new_act_name: *const c_char,
	pub new_act_duration: f32,
	pub wall_touch_reset: i32,
}

impl Default for ActInfo {
	fn default() -> Self { Self {
		new_act_name: core::ptr::null(),
		new_act_duration: 0.0,
		wall_touch_reset: 0,
	} }
}

static S_STAND: &[u8; 6] = b"STAND\0";
static S_WALK:  &[u8; 5] = b"WALK\0";
static S_RUN:   &[u8; 4] = b"RUN\0";
static S_RETREAT: &[u8; 8] = b"RETREAT\0";
static S_TURN_L: &[u8; 7] = b"TURN_L\0";
static S_TURN_R: &[u8; 7] = b"TURN_R\0";

#[no_mangle]
pub extern "C" fn STAND(p_info: *mut ActInfo) {
	let info = unsafe { &mut *p_info };
	*info = ActInfo::default();
	if check_act_timeout() != 0 {
		if (glb_rng_next() & 0x3F) < 0xF {
			info.new_act_name = S_RUN.as_ptr() as *const c_char;
			info.new_act_duration = 2.0;
		} else {
			info.new_act_name = S_WALK.as_ptr() as *const c_char;
			info.new_act_duration = 5.0;
			info.wall_touch_reset = 1;
		}
	}
}

#[no_mangle]
pub extern "C" fn WALK(p_info: *mut ActInfo) {
	let info = unsafe { &mut *p_info };
	let obj_touch_dt = obj_touch_duration_secs();
	let wall_touch_dt = wall_touch_duration_secs();
	*info = ActInfo::default();
	if check_act_timeout() != 0 || obj_touch_dt > 0.2 || wall_touch_dt > 0.25 {
		if obj_touch_dt > 0.0 && (glb_rng_next() & 0x3F) < 0x1F {
			info.new_act_name = S_RETREAT.as_ptr() as *const c_char;
			info.new_act_duration = 0.5;
		} else {
			let t = fit(glb_rng_f01(), 0.0, 1.0, 0.5, 2.0);
			info.new_act_name = if (glb_rng_next() & 1) != 0 {
				S_TURN_L.as_ptr() as *const c_char
			} else {
				S_TURN_R.as_ptr() as *const c_char
			};
			info.new_act_duration = t;
		}
	}
}

#[no_mangle]
pub extern "C" fn RUN(p_info: *mut ActInfo) {
	let info = unsafe { &mut *p_info };
	let obj_touch_dt = obj_touch_duration_secs();
	let wall_touch_dt = wall_touch_duration_secs();
	*info = ActInfo::default();
	if check_act_timeout() != 0 || wall_touch_dt > 0.1 || obj_touch_dt > 0.1 {
		info.new_act_name = S_STAND.as_ptr() as *const c_char;
		info.new_act_duration = 2.0;
	}
}

#[no_mangle]
pub extern "C" fn RETREAT(p_info: *mut ActInfo) {
	let info = unsafe { &mut *p_info };
	let wall_touch_dt = wall_touch_duration_secs();
	*info = ActInfo::default();
	if check_act_timeout() != 0 || wall_touch_dt > 0.1 {
		info.new_act_name = S_STAND.as_ptr() as *const c_char;
		info.new_act_duration = 2.0;
	}
}

#[no_mangle]
pub extern "C" fn TURN(p_info: *mut ActInfo) {
	let info = unsafe { &mut *p_info };
	*info = ActInfo::default();
	if check_act_timeout() != 0 {
		info.new_act_name = S_STAND.as_ptr() as *const c_char;
		info.new_act_duration = 1.0;
	}
}


#[panic_handler]
fn panic(_pi: &core::panic::PanicInfo) -> ! { loop{} }

#[no_mangle]
pub extern "C" fn BAD() -> i32 { 0xBAD }

#[no_mangle]
pub extern "C" fn _start() -> ! { loop{} }



