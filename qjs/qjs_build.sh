#!/bin/sh

TMP_DIR=tmp
SRC_DIR=src
OBJ_DIR=obj

QJS_OUT=quickjs.a

QJS_VER="2021-03-27"
QJS_TAR="quickjs-$QJS_VER.tar.xz"
QJS_URL="https://bellard.org/quickjs/$QJS_TAR"
QJS_CFLAGS="-D_GNU_SOURCE -DCONFIG_VERSION=\"$QJS_VER\""

DL_CMD=""
if [ -x "`command -v curl`" ]; then
	DL_CMD="curl -o"
elif [ -x "`command -v wget`" ]; then
	DL_CMD="wget -O"
fi

if [ -n $DL_CMD ]; then
	mkdir -p $TMP_DIR
	$DL_CMD $TMP_DIR/$QJS_TAR $QJS_URL
	if [ -f $TMP_DIR/$QJS_TAR ]; then
		tar -xvJf $TMP_DIR/$QJS_TAR
	fi
	mkdir -p $SRC_DIR
	if [ -d "quickjs-$QJS_VER" ]; then
		for qjs_src in cutils list libunicode libregexp quickjs ; do
			cp -f quickjs-$QJS_VER/$qjs_src*.* $SRC_DIR
		done
	fi
	echo "Patching for multithreading..."
	sed -i 's/#define CONFIG_STACK_CHECK/#define _NO_CONFIG_STACK_CHECK/g' $SRC_DIR/quickjs.c
	rm -rdf quickjs-$QJS_VER
fi

CC=${CC:-gcc}

echo Building $QJS_OUT...
rm -f $QJS_OUT

QJS_SRCS="cutils libunicode libregexp quickjs quickjs-libc"
mkdir -p $OBJ_DIR
for qjs_src in $QJS_SRCS; do
	if [ -f "$SRC_DIR/$qjs_src.c" ]; then
		echo Compiling $qjs_src.c
		$CC -c $QJS_CFLAGS -I $SRC_DIR $SRC_DIR/$qjs_src.c -o $OBJ_DIR/$qjs_src.o $*
	fi
done

ar -cqv $QJS_OUT obj/*.o
