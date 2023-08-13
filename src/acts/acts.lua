
function lerp(a, b, t)
	return a + (b - a)*t
end

function fit(val, oldMin, oldMax, newMin, newMax)
	local t = oldMax - oldMin
	if t == 0.0 then
		return 0.0
	end
	local rel = (val - oldMin) / t
	if rel < 0.0 then
		rel = 0.0
	end
	if rel > 1.0 then
		rel = 1.0
	end
	return lerp(newMin, newMax, rel)
end

function mood()
	local t = get_mood_arg()
	local x = t * 6.2831853072
	local x2 = x*x
	local x4 = x2*x2
	local x6 = x4*x2
	local x8 = x4*x4
	local x10 = x8*x2
	local x12 = x10*x2
	local x14 = x12*x2
	local x16 = x8*x8

	local y = 1.0 - 0.25*x2 + 0.02083333*x4 - 0.00069444*x6 + 0.0000124007936508*x8 - 0.000000137787*x10 + 0.000000001044*x12 - 0.00000000000574*x14 + 0.0000000000000239*x16

	return y
end

function STAND()
	local newAct = ""
	local newDuration = 0.0
	local wallReset = false
	if check_act_timeout() then
		if (glb_rng_next() & 0x3F) < 0xF then
			newAct = "RUN"
			newDuration = 2.0
		else
			newAct = "WALK"
			newDuration = 5.0
			wallReset = true
		end
	end

	return newAct, newDuration, wallReset
end

function WALK()
	local newAct = ""
	local newDuration = 0.0
	local wallReset = false	
	local objTouchDT = obj_touch_duration_secs()
	local wallTouchDT = wall_touch_duration_secs()
	if check_act_timeout() or objTouchDT > 0.2 or wallTouchDT > 0.25 then
		if (objTouchDT > 0) and ((glb_rng_next() & 0x3F) < 0x1F) then
			newAct = "RETREAT"
			newDuration = 0.5
		else
			local t = fit(glb_rng_f01(), 0.0, 1.0, 0.5, 2.0)
			if glb_rng_next() & 1 then
				newAct = "TURN_L"
			else
				newAct = "TURN_R"
			end
			newDuration = t
		end
	end

	return newAct, newDuration, wallReset
end

function RUN()
	local newAct = ""
	local newDuration = 0.0
	local wallReset = false
	local objTouchDT = obj_touch_duration_secs()
	local wallTouchDT = wall_touch_duration_secs()
	if check_act_timeout() or (objTouchDT > 0.1) or (wallTouchDT > 0.1) then
		newAct = "STAND"
		newDuration = 2.0
	end

	return newAct, newDuration, wallReset
end

function RETREAT()
	local newAct = ""
	local newDuration = 0.0
	local wallReset = false
	local wallTouchDT = wall_touch_duration_secs()
	if check_act_timeout() or wallTouchDT > 0.1 then
		newAct = "STAND"
		newDuration = 2.0
	end

	return newAct, newDuration, wallReset
end

function TURN_L()
	local newAct = ""
	local newDuration = 0.0
	local wallReset = false
	if check_act_timeout() then
		newAct = "STAND"
		newDuration = 1.0
	end

	return newAct, newDuration, wallReset
end

function TURN_R()
	local newAct = ""
	local newDuration = 0.0
	local wallReset = false
	if check_act_timeout() then
		newAct = "STAND"
		newDuration = 1.0
	end

	return newAct, newDuration, wallReset
end
