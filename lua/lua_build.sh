#!/bin/sh

TMP_DIR=tmp
SRC_DIR=src
OBJ_DIR=obj

LUA_OUT=lua.a

LUA_VER="5.4.4"
LUA_TAR="lua-$LUA_VER.tar.gz"
LUA_URL="http://www.lua.org/ftp/$LUA_TAR"
LUA_CFLAGS=""

DL_CMD=""
if [ -x "`command -v curl`" ]; then
	DL_CMD="curl -o"
elif [ -x "`command -v wget`" ]; then
	DL_CMD="wget -O"
fi

if [ -n "$DL_CMD" ]; then
	mkdir -p $TMP_DIR
	$DL_CMD $TMP_DIR/$LUA_TAR $LUA_URL
	if [ -f $TMP_DIR/$LUA_TAR ]; then
		tar -xzf $TMP_DIR/$LUA_TAR
	fi
	cp -fr lua-$LUA_VER/src .
	rm -rdf lua-$LUA_VER
fi

CC=${CC:-gcc}

echo Building $LUA_OUT...
rm -f $LUA_OUT

LUA_CORE="lapi lcode lctype ldebug ldo ldump lfunc lgc llex lmem lobject lopcodes lparser lstate lstring ltable ltm lundump lvm lzio"
LUA_LIB="lauxlib lbaselib lcorolib ldblib liolib lmathlib loadlib loslib lstrlib ltablib lutf8lib linit"
LUA_SRCS="$LUA_CORE $LUA_LIB"
mkdir -p $OBJ_DIR
for lua_src in $LUA_SRCS; do
	if [ -f "$SRC_DIR/$lua_src.c" ]; then
		echo Compiling $lua_src.c
		$CC -c $LUA_CFLAGS -I $SRC_DIR $SRC_DIR/$lua_src.c -o $OBJ_DIR/$lua_src.o $*
	fi
done

AR=${AR:-ar}
$AR -cqv $LUA_OUT obj/*.o
