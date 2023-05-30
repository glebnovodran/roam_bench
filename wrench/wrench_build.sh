#!/bin/sh

TMP_DIR=tmp
SRC_DIR=src
OBJ_DIR=obj

WRENCH_OUT=wrench.a

WRENCH_CFLAGS=""

DL_CMD=""
if [ -x "`command -v curl`" ]; then
	DL_CMD="curl -o"
elif [ -x "`command -v wget`" ]; then
	DL_CMD="wget -O"
fi

WRENCH_SRC_URL="https://raw.githubusercontent.com/jingoro2112/wrench/main/src"
WRENCH_SRCS="wrench.h wrench.cpp"

CC=${CC:-gcc}

printf "Downloading wrench sources...\n"
mkdir -p $SRC_DIR
for src in $WRENCH_SRCS; do
	if [ ! -f $SRC_DIR/$src ]; then
		printf "     $WRENCH_SRC_URL/$src\n"
		$DL_CMD $SRC_DIR/$src $WRENCH_SRC_URL/$src
	fi
done


echo Building $WRENCH_OUT...
rm -f $WRENCH_OUT

mkdir -p $OBJ_DIR

echo Compiling wrench.cpp
$CC -c $WRENCH_CFLAGS -I ./$SRC_DIR $SRC_DIR/wrench.cpp -o $OBJ_DIR/wrench.o $*

AR=${AR:-ar}
$AR -cqv $WRENCH_OUT obj/*.o
