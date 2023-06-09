@echo off

setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

set PROG_NAME=roam_bench

set SRC_DIR=src
set TMP_DIR=tmp
set INC_DIR=inc
set PROG_DIR=prog
set XCORE_DIR=core
set PINT_DIR=pint
set DATA_DIR=data
set ACTS_DIR=acts

set PROG_PATH=%PROG_DIR%\%PROG_NAME%.exe
set RUN_PATH=run.bat

set PDB_PATH="%PROG_DIR%\%PROG_NAME%.pdb"
set ILK_PATH="%PROG_DIR%\%PROG_NAME%.ilk"
set LST_PATH="%PROG_DIR%\%PROG_NAME%.lst"

if not exist %PROG_DIR% mkdir %PROG_DIR%

echo   ~ Compiling Roam-Benchmark for Windows ~

goto :start

:mk_hget
	echo /* WSH HTTP downloader */ > %_HGET_%
	echo var req = new ActiveXObject("MSXML2.XMLHTTP.6.0"); >> %_HGET_%
	echo var url = WScript.Arguments(0); var res = null; >> %_HGET_%
	echo WScript.StdErr.WriteLine("Downloading " + url + "..."); >> %_HGET_%
	echo try{req.Open("GET",url,false);req.Send();if(req.Status==200)res=req.ResponseText; >> %_HGET_%
	echo } catch (e) { WScript.StdErr.WriteLine(e.description); } >> %_HGET_%
	echo if (res) { WScript.StdErr.WriteLine("OK^!"); WScript.Echo(res); } >> %_HGET_%
	echo else { WScript.StdErr.WriteLine("Failed."); } >> %_HGET_%
goto :EOF


:start
set TDM_DIR=%~dp0etc\tools\TDM-GCC
if not x%TDM_HOME%==x (
	set TDM_DIR=%TDM_HOME%
)
echo [Setting up compiler tools...]
if not exist %TDM_DIR%\bin (
	if x%VC_CFG%==x (
		echo ^^! Please install TDM-GCC tools:
		echo ^^! https://jmeubank.github.io/tdm-gcc/download/
		echo ^^! To build:
		echo ^^! cmd /c "set TDM_HOME=^<path^>&&build.bat"
		exit /B -1
	)
)
set TDM_BIN=%TDM_DIR%\bin
set TDM_GCC=%TDM_BIN%\gcc.exe
set TDM_GPP=%TDM_BIN%\g++.exe
set TDM_MAK=%TDM_BIN%\mingw32-make.exe
set TDM_WRS=%TDM_BIN%\windres.exe
if x%VC_CFG%==x (
	echo [C++ compiler path: %TDM_GPP%]
) else (
	echo Configuring MSVC env...
	call %VC_CFG%
	if x!VC_CXX!==x (
		echo Path to MSVC not specified
		exit /B -1
	)
	if not exist "!VC_CXX!" (
		echo ^^! MSVC tools not found ^^!
		exit /B -1
	)
	echo [C++ compiler path: !VC_CXX!]
)


set _CSCR_=cscript.exe /nologo

if not exist %TMP_DIR% mkdir %TMP_DIR%
set _HGET_=%TMP_DIR%\hget.js
set _BNDGET_=%TMP_DIR%\bndget.js

echo [Checking OpenGL headers...]
if not exist %_HGET_% call :mk_hget
if not exist %INC_DIR%\GL mkdir %INC_DIR%\GL
if not exist %INC_DIR%\KHR mkdir %INC_DIR%\KHR
set KHR_REG=https://registry.khronos.org
for %%i in (GL\glcorearb.h GL\glext.h GL\wglext.h KHR\khrplatform.h) do (
	if not exist %INC_DIR%\%%i (
		set rel=%%i
		set rel=!rel:GL\=OpenGL/api/GL/!
		set rel=!rel:KHR\=EGL/api/KHR/!
		%_CSCR_% %_HGET_% %KHR_REG%/!rel! > %INC_DIR%\%%i
	)
)

rem goto :xskip
echo [Checking crosscore sources...]
if not exist %XCORE_DIR% mkdir %XCORE_DIR%
if not exist %XCORE_DIR%\ogl mkdir %XCORE_DIR%\ogl
set XCORE_BASE=https://raw.githubusercontent.com/schaban/crosscore_dev/main/src
for %%i in (crosscore.hpp crosscore.cpp demo.hpp demo.cpp draw.hpp oglsys.hpp oglsys.cpp oglsys.inc scene.hpp scene.cpp smprig.hpp smprig.cpp smpchar.cpp smpchar.hpp draw_ogl.cpp main.cpp ogl\gpu_defs.h ogl\progs.inc ogl\shaders.inc) do (
	if not exist %XCORE_DIR%\%%i (
		set rel=%%i
		%_CSCR_% %_HGET_% %XCORE_BASE%/!rel:\=/! > %XCORE_DIR%\%%i
	)
)
:xskip

echo [Checking pint sources...]
if not exist %PINT_DIR% mkdir %PINT_DIR%
set PINT_BASE=https://raw.githubusercontent.com/glebnovodran/proto-plop/main/pint/src
for %%i in (pint.hpp pint.cpp) do (
	if not exist %PINT_DIR%\%%i (
		set rel=%%i
		%_CSCR_% %_HGET_% %PINT_BASE%/!rel:\=/! > %PINT_DIR%\%%i
	)
)

echo [Copying scripts...]
if not exist %DATA_DIR%\%ACTS_DIR% mkdir %DATA_DIR%\%ACTS_DIR%
copy /BY %SRC_DIR%\%ACTS_DIR% %DATA_DIR%\%ACTS_DIR%


if exist %PROG_PATH% (
	echo [Removing previously compiled executable %PROG_PATH%...]
	del %PROG_PATH%
)

if exist %RUN_PATH% del %RUN_PATH%

if exist %PDB_PATH% (
	echo Cleaning %PDB_PATH%.
	del %PDB_PATH%
)

if exist %ILK_PATH% (
	echo Cleaning %ILK_PATH%.
	del %ILK_PATH%
)

if exist %LST_PATH% (
	echo Cleaning %LST_PATH%.
	del %LST_PATH%
)


set SRC_FILES=
set OBJ_FILES=
for /f %%i in ('dir /b %SRC_DIR%\*.cpp') do (
	set SRC=%%i
	set SRC_FILES=!SRC_FILES! %SRC_DIR%/!SRC!
	set OBJ_FILES=!OBJ_FILES! !SRC:~0,-3!obj
)
for /f %%i in ('dir /b %XCORE_DIR%\*.cpp') do (
	set SRC=%%i
	set SRC_FILES=!SRC_FILES! %XCORE_DIR%/!SRC!
	set OBJ_FILES=!OBJ_FILES! !SRC:~0,-3!obj
)
for /f %%i in ('dir /b %PINT_DIR%\*.cpp') do (
	set SRC=%%i
	set SRC_FILES=!SRC_FILES! %PINT_DIR%/!SRC!
	set OBJ_FILES=!OBJ_FILES! !SRC:~0,-3!obj
)

set BUILD_OPTS=%*
set CPP_OFLGS=-ffast-math -ftree-vectorize
rem -O3 -flto
set XCORE_FLAGS=-DOGLSYS_ES=0 -DOGLSYS_CL=0 -DXD_TSK_NATIVE=1 -DSCN_CMN_PKG_NAME=\"common\"
set CPP_OPTS=%CPP_OFLGS% -std=c++11 -ggdb -Wno-psabi -Wno-deprecated-declarations
set LNK_OPTS=-l gdi32 -l ole32 -l windowscodecs
set INC_PATHS=-I %INC_DIR% -I %XCORE_DIR% -I %PINT_DIR% -I %SRC_DIR%
echo [Compiling %PROG_PATH%...]
if x!VC_CXX!==x (
	%TDM_GPP% %CPP_OPTS% %XCORE_FLAGS% %INC_PATHS% %SRC_FILES% -o %PROG_PATH% %LNK_OPTS% %BUILD_OPTS%
	rem %TDM_BIN%\objdump.exe -M intel -d -C %PROG_PATH% > %PROG_DIR%\%PROG_NAME%.txt
	rem %TDM_BIN%\strip.exe %PROG_PATH%
) else (
	set VCXX_OPTS=/O2 /arch:SSE2 /GL /Gy /Zi /Gm- /Oi /Oy /Zc:inline  /Zc:forScope /MT /EHsc /GS- /fp:fast /DNDEBUG /D_CONSOLE  /D_UNICODE /DUNICODE
	set VCLNK_OPTS=/DYNAMICBASE:NO "kernel32.lib" "user32.lib" "gdi32.lib" "ole32.lib" "windowscodecs.lib"
	!VC_CXX! !VCXX_OPTS! %XCORE_FLAGS% %INC_PATHS% %SRC_FILES% %BUILD_OPTS% /Fe%PROG_PATH% /Fd%PDB_PATH% !VCLNK_OPTS!
	rem !VC_DIS! %PROG_PATH% > %PROG_DIR%\%PROG_NAME%.txt
	del /Q %OBJ_FILES%
)

if exist %PROG_PATH% (
	echo rem roam-benchmark > %RUN_PATH%
	echo %PROG_PATH% -nwrk:0 -mood_period:10 %%* >> %RUN_PATH%
	echo Success^^!
) else (
	echo Failure :(
)
