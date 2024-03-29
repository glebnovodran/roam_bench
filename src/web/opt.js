const pageParams = new URLSearchParams(window.location.search);

Module.arguments = [
	"-demo:default",
	"-draw:ogl",
	"-w:800", "-h:500",
	"-nwrk:0",
	"-adapt:1",
	"-data:bin/data",
	"-glsl_echo:1",
	"-mood_period:10",
	"-avg_smps:10"
];

if (pageParams.get("pint") !== null) {
	Module.arguments.push("-roamprog:pint");
}

if (pageParams.get("wrench") !== null) {
	Module.arguments.push("-roamprog:wrench");
}

if (pageParams.get("lua") !== null) {
	Module.arguments.push("-roamprog:lua");
}

if (pageParams.get("qjs") !== null) {
	Module.arguments.push("-roamprog:qjs");
}

if (pageParams.get("minion") !== null) {
	Module.arguments.push("-roamprog:minion");
}

if (pageParams.get("small") !== null) {
	Module.arguments.push("-w:480");
	Module.arguments.push("-h:320");
}

if (pageParams.get("lowq") !== null) {
	Module.arguments.push("-lowq:1");
	Module.arguments.push("-smap:512");
	Module.arguments.push("-bump:0");
	Module.arguments.push("-spec:0");
	Module.arguments.push("-vl:1");
} else {
	Module.arguments.push("-lowq:0");
	Module.arguments.push("-smap:2048");
	Module.arguments.push("-bump:1");
	Module.arguments.push("-spec:1");
	Module.arguments.push("-msaa:4");
}

if (pageParams.get("exerep") !== null) {
	Module.arguments.push("-exerep:5");
	Module.arguments.push("-speed:0.2");
}

modeVal = pageParams.get("mode");
if (modeVal !== null) {
	Module.arguments.push("-mode:" + modeVal);
}

if (pageParams.get("noctrl") !== null) {
	Module.arguments.push("-roamctrl_disable:1");
}

if (pageParams.get("nomood") !== null) {
	Module.arguments.push("-mood_period:-1");
}

if (pageParams.get("moodvis") !== null) {
	Module.arguments.push("-mood_vis:1");
}
