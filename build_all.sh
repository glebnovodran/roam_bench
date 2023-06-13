#!/bin/sh

WEB_CC=0
BUILD_WEB_MODE=""

if [ "$#" -gt 0 ]; then
	case $1 in
		clean)
			./build.sh clean
			exit 0
		;;
		web)
			WEB_CC=1
			BUILD_WEB_MODE="web"
			shift
			if [ "$1" = "wasm" ]; then
				BUILD_WEB_MODE="web wasm"
				shift
			fi
		;;
	esac
fi

DL_CMD=""

if [ -x "`command -v curl`" ]; then
        DL_MODE="CURL"
        DL_CMD="curl -o"
elif [ -x "`command -v wget`" ]; then
        DL_MODE="WGET"
        DL_CMD="wget -O"
fi

WRENCH_DIR=wrench/src
WRENCH_SRCS="wrench.h wrench.cpp"
WRENCH_SRC_URL=https://raw.githubusercontent.com/jingoro2112/wrench/main/src

NEED_WRENCH=0
if [ ! -d $WRENCH_DIR ]; then
	mkdir -p $WRENCH_DIR
	NEED_WRENCH=1
else
	for src in $WRENCH_SRCS; do
		if [ $NEED_WRENCH -ne 1 ]; then
			if [ ! -f $WRENCH_DIR/$src ]; then
				NEED_WRENCH=1
			fi
		fi
	done
fi

if [ $NEED_WRENCH -ne 0 ]; then
	printf "Downloading wrench sources...\n"
	for src in $WRENCH_SRCS; do
		if [ ! -f $WRENCH_DIR/$src ]; then
			printf "     $src\n"
			$DL_CMD $WRENCH_DIR/$src $WRENCH_SRC_URL/$src
		fi
	done
fi

if [ ! -f lua/lua.a ]; then
	printf "Building Lua...\n"
	cd lua
	if [ $WEB_CC -ne 0 ]; then
		CC=emcc AR=emar ./lua_build.sh -O3 -flto
	else
		./lua_build.sh -O3 -flto
	fi
	cd ..
fi

if [ ! -f qjs/quickjs.a ]; then
	printf "Building QuickJS...\n"
	cd qjs
	if [ $WEB_CC -ne 0 ]; then
		CC=emcc AR=emar ./qjs_build.sh -O3 -flto
	else
		./qjs_build.sh -O3 -flto
	fi
	cd ..
fi

ROAM_FLAGS="-DROAM_WRENCH=1 -DROAM_QJS=1 -DROAM_LUA=1"
./build.sh $BUILD_WEB_MODE -I wrench/src wrench/src/wrench.cpp $ROAM_FLAGS -I qjs/src qjs/quickjs.a -I lua/src lua/lua.a -O3 -flto=auto $*
