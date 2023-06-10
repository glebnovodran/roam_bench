#!/bin/sh

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
	./lua_build.sh -O3 -flto
	cd ..
fi

if [ ! -f qjs/quickjs.a ]; then
	printf "Building QuickJS...\n"
	cd qjs
	./qjs_build.sh -O3 -flto
	cd ..
fi

./build.sh -I wrench/src wrench/src/wrench.cpp -DROAM_WRENCH=1 -DROAM_QJS=1 -DROAM_LUA=1 -I qjs/src qjs/quickjs.a -I lua/src lua/lua.a -O3 -flto=auto $*
