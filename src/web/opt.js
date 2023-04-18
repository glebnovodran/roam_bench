const pageParams = new URLSearchParams(window.location.search);

Module.arguments = [
	"-demo:default",
	"-draw:ogl",
	"-w:800", "-h:500",
	"-nwrk:0",
	"-adapt:1",
	"-data:bin/data",
	"-glsl_echo:1"
];

if (pageParams.get("pint") !== null) {
	Module.arguments.push("-roamprog:pint");
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


