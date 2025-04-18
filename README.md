# ~ Character scripting benchmark ~

The project goal is to measure relative performance of different character scripting methods in a game-type scenario.

## 'Built-in' control methods

These are based on my own code.

- **Native** is a compiled [C++ code](https://github.com/glebnovodran/roam_bench/blob/main/src/roam_ctrl_native.cpp). Its goal is to provide the fastest possible timing for character control execution.

- **pint** is a simple executable S-expression form. [**pint**](https://github.com/glebnovodran/proto-plop/tree/main/pint) is very slow (though still useful for my project needs) and its goal is to provide worst case timing.

## 'External' control methods

These are implemented with external libraries.

- [**QuickJS**](https://bellard.org/quickjs/) is a Javascript engine. It is easily embeddable and relatively lightweight and, as such, is a good way to test JS implemented via bytecode interpreter in the aforementioned scenario.

- [**Lua**](http://www.lua.org/) is a scripting language of choice for many game projects. Its goal to provide a performance standard to achieve.

- [**wrench**](https://github.com/jingoro2112/wrench) is a recently developed embeddable scripting language. Its main purpose is to be used on embedded platforms with very limited resources. For such scenarios it seems to be on par or faster than Lua. So, it is interesting to see it in the game-programming context on different platforms.

**WRENCH SUPPORT TEMPORARILY DISABLED**

## 'RISC-y' control method

Character control routines are defined in [this C source](https://github.com/glebnovodran/roam_bench/blob/main/src/acts/roam.c). They are compiled to RISC-V machine code which is then executed with [**Minion**](https://github.com/schaban/rv_minion) RV32g instruction set simulator.

## Building the project. ##

Clone the project with
<br>```git clone https://github.com/glebnovodran/roam_bench.git --depth 1```

Execute ```./build_all.sh ``` . It will build the project and generate run.sh launcher script.

For Windows and Visual Studio see [**here**](./doc/roam_windows.md).

Executing ```run.sh``` will display something like:

![info](https://glebnovodran.github.io/roam/roam_info.jpg)

By default, if launched with ```./run.sh``` , it will use 'Native' control method. To use another control method, for example Lua, use ```./run.sh -roamprog:lua```

## How to interpret the benchmark information. ###
Here is how to interpret the information on the screen:

1. Control type;
<br>Select with ```-roamprog:{lua|qjs|wrench|pint|minion|native}```, default is ```native```.

2. Number of character in scene;
<br>This can be changed with ```-mode:{0|1|-1|-2}``` option.
<br> 0 - 20 characters; 1 - 40 characters; -1 - single character; -2 - two characters.

3. Average time to execute control programs for all characters in the scene;
<br>The value in brackets is an average over the certain number of frames.

4. Frames per second;
<br>This is limited by V-Sync by defailt, use ```-swap:0``` to redraw as fast as possible.

5. Time spent updating the entire scene (but not drawing it!);
<br>Control program is only a fraction of the character update routine, which also includes animation and collision-detection execution.


Example results for all currently supported control methods:

![info](https://glebnovodran.github.io/roam/roam_all_res.jpg)

Exact values, for various platforms differ, but the ratio is more or less the same.

The project can be built without graphics output: ```./build_all.sh dummygl```
In that case character positions will be displayed as an 'overhead view' in the terminal. The statistics displayed is the same as described for the gfx mode above:

![dummygl](https://glebnovodran.github.io/roam/dummygl_disp.png)
<br><br>

You can run the graphics version of the benchmark inside web-browser here:<br>
https://glebnovodran.github.io/roam/rb_web.html

This project uses [-=crosscore=-](https://github.com/schaban/crosscore_dev) library.
