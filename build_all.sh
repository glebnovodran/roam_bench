#!/bin/sh

WEB_CC=0
BUILD_WEB_MODE=""
DUMMYGL_MODE=0
CMN_OPTS=${CMN_OPTS-""}

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
		dummygl)
			DUMMYGL_MODE=1
			shift
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


# ---- get wrench

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


# ---- get minion

MINION_DIR=minion
MINION_SRC_URL=https://raw.githubusercontent.com/schaban/rv_minion/main
MINION_SRCS="minion.c minion.h minion_instrs.c minion_regs.c"

NEED_MINION=0
if [ ! -d $MINION_DIR ]; then
	mkdir -p $MINION_DIR
	NEED_MINION=1
else
	for src in $MINION_SRCS; do
		sname=$src
		if [ $sname = minion.c ]; then
			sname=minion.cpp
		fi
		if [ $NEED_MINION -ne 1 ]; then
			if [ ! -f $MINION_DIR/$sname ]; then
				NEED_MINION=1
			fi
		fi
	done
fi

if [ $NEED_MINION -ne 0 ]; then
	printf "Downloading minion sources...\n"
	for src in $MINION_SRCS; do
		sname=$src
		if [ $sname = minion.c ]; then
			sname=minion.cpp
		fi
		if [ ! -f $MINION_DIR/$sname ]; then
			printf "     $src - $sname\n"
			$DL_CMD $MINION_DIR/$sname $MINION_SRC_URL/$src
		fi
	done
fi


# ---- build lua

if [ ! -f lua/lua.a ]; then
	printf "Building Lua...\n"
	cd lua
	if [ $WEB_CC -ne 0 ]; then
		CC=emcc AR=emar ./lua_build.sh -O3 -flto $CMN_OPTS
	else
		./lua_build.sh -O3 -flto $CMN_OPTS
	fi
	cd ..
fi


# ---- build qjs

if [ ! -f qjs/quickjs.a ]; then
	printf "Building QuickJS...\n"
	cd qjs
	if [ $WEB_CC -ne 0 ]; then
		CC=emcc AR=emar ./qjs_build.sh -O3 -flto $CMN_OPTS
	else
		./qjs_build.sh -O3 -flto $CMN_OPTS
	fi
	cd ..
fi


ROAM_FLAGS="-DROAM_WRENCH=1 -DROAM_QJS=1 -DROAM_LUA=1 -DROAM_MINION=1"
DEP_WRENCH="-I $WRENCH_DIR $WRENCH_DIR/wrench.cpp"
DEP_MINION="-I $MINION_DIR $MINION_DIR/minion.cpp"
DEP_OPTS="$DEP_WRENCH $DEP_MINION $ROAM_FLAGS -I qjs/src qjs/quickjs.a -I lua/src lua/lua.a"
OPTI_OPTS="-O3 -flto=auto"
if [ $DUMMYGL_MODE -ne 0 ]; then
	ALT_LIBS="" ALT_DEFS="-DDUMMY_GL" ./build.sh $DEP_OPTS $OPTI_OPTS $CMN_OPTS $*
else
	./build.sh $BUILD_WEB_MODE $DEP_OPTS $OPTI_OPTS $CMN_OPTS $*
fi
