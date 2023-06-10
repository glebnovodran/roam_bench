# ~ Character control benchmark ~

The project goal is to measure relative performance of different character scripting methods in a game type scenario.

## 'Built-in' control methods.

These are based on my own code.

- **Native** is a compiled [C++ code](https://github.com/glebnovodran/roam_bench/blob/main/src/roam_ctrl_native.cpp). Its goal is to provide the fastest possible timing for character control execution.

- **pint** is a simple executable S-expression form. [**pint**](https://github.com/glebnovodran/proto-plop/tree/main/pint) is very slow (though still useful) and its goal is to provide worst case timing.

## 'External' control methods.

These are implemented with external libraries.

- JavaScript on **QuickJS**. [QuickJS](https://github.com/bellard/quickjs/) is a Javascript engine. It is easily embeddable and relatively lightwieght and, as such, is a good way to test JS implemented via bytecode interpreter in an aforementioned scenario.

- **Lua** is a [scripting language](http://www.lua.org/) of choice for many game projects. Its goal to provide a performance standard to achieve.

- **wrench** is a recently developed embeddable scripting [language](https://www.northarc.com/wrench/www/). Its main purpose is to be used on embedded platforms with very limited resources. For such scenarios it seems to be on par or faster than Lua. So, it is interesting to see it in the game-programming context on different platforms.

## Quick compilation on Linux. ##
Execute ```./build_all.sh ``` . It will build the project and generate run.sh.

Executing run.sh will display something like:

![info](https://glebnovodran.github.io/roam/roam_info.jpg)

Here is how to interpret the information on the screen:

1. Control type;
<br>Select with ```-roamprog:{lua|qjs|wrench|pint|native}```, default is native.

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