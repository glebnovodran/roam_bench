#!/bin/sh

PROG_NAME=roam_bench
BUILD_DATE="$(date)"
SYS_NAME="`uname -s`"
BUILD_PATH=$PWD

PROG_DIR=prog
CORE_DIR=core
DATA_DIR=data
PINT_DIR=pint

INC_DIR=inc
SRC_DIR=src
TMP_DIR=tmp


WEB_TMP_DIR=tmp/web

PROG_PATH=$PROG_DIR/$PROG_NAME
RUN_PATH=run.sh

if [ "$SYS_NAME" = "FreeBSD" ]; then
	NO_FMT=""
fi

FMT_BOLD=${NO_FMT-"\e[1m"}
FMT_UNDER=${NO_FMT-"\e[4m"}
FMT_BLACK=${NO_FMT-"\e[30m"}
FMT_BLACK_BG=${NO_FMT-"\e[40m"}
FMT_RED=${NO_FMT-"\e[31m"}
FMT_RED_BG=${NO_FMT-"\e[41m"}
FMT_GREEN=${NO_FMT-"\e[32m"}
FMT_GREEN_BG=${NO_FMT-"\e[42m"}
FMT_YELLOW=${NO_FMT-"\e[33m"}
FMT_YELLOW_BG=${NO_FMT-"\e[43m"}
FMT_BLUE=${NO_FMT-"\e[34m"}
FMT_BLUE_BG=${NO_FMT-"\e[44m"}
FMT_MAGENTA=${NO_FMT-"\e[35m"}
FMT_MAGENTA_BG=${NO_FMT-"\e[45m"}
FMT_CYAN=${NO_FMT-"\e[36m"}
FMT_CYAN_BG=${NO_FMT-"\e[46m"}
FMT_WHITE=${NO_FMT-"\e[37m"}
FMT_WHITE_BG=${NO_FMT-"\e[47m"}
FMT_GRAY=${NO_FMT-"\e[90m"}
FMT_GRAY_BG=${NO_FMT-"\e[100m"}
FMT_B_RED=${NO_FMT-"\e[91m"}
FMT_B_RED_BG=${NO_FMT-"\e[101m"}
FMT_B_GREEN=${NO_FMT-"\e[92m"}
FMT_B_GREEN_BG=${NO_FMT-"\e[102m"}
FMT_B_YELLOW=${NO_FMT-"\e[93m"}
FMT_B_YELLOW_BG=${NO_FMT-"\e[103m"}
FMT_B_BLUE=${NO_FMT-"\e[94m"}
FMT_B_BLUE_BG=${NO_FMT-"\e[104m"}
FMT_B_MAGENTA=${NO_FMT-"\e[95m"}
FMT_B_MAGENTA_BG=${NO_FMT-"\e[105m"}
FMT_B_CYAN=${NO_FMT-"\e[96m"}
FMT_B_CYAN_BG=${NO_FMT-"\e[106m"}
FMT_B_WHITE=${NO_FMT-"\e[97m"}
FMT_B_WHITE_BG=${NO_FMT-"\e[107m"}
FMT_OFF=${NO_FMT-"\e[0m"}

WEB_CXX=""
WEB_BUILD=0

if [ "$#" -gt 0 ]; then
	case $1 in
		clean)
			printf "$FMT_RED$FMT_BOLD""Removing temporary files!""$FMT_OFF\n"
			for tdir in core inc prog tmp $SHADERS_TGT_DIR; do
				if [ -d $tdir ]; then
					printf " * ""$FMT_BOLD""$tdir""$FMT_OFF\n"
					rm -rf $tdir/*
					rmdir $tdir
				fi
			done
			if [ -f $RUN_PATH ]; then
				printf " - ""$FMT_BOLD""$RUN_PATH""$FMT_OFF\n"
				rm -f $RUN_PATH
			fi
			CMK_FILE="CMakeLists.txt"
			if [ -f $CMK_FILE ]; then
				rm -f $CMK_FILE
			fi
			exit 0
		;;
		web)
			WEB_BUILD=1
			if [ -n "$EMSDK" ]; then
				shift
				WEB_CXX="em++"
			fi
		;;
	esac
fi

printf "$FMT_MAGENTA$FMT_BOLD""*-----------------------------------------* ""$FMT_OFF\n"
printf " ""$FMT_BLUE_BG$FMT_B_YELLOW"" .: Building roam_bench project :. ""$FMT_OFF\n"
printf "$FMT_MAGENTA$FMT_BOLD""*-----------------------------------------* ""$FMT_OFF\n"

if [ $WEB_BUILD -ne 0 ]; then
	if [ -z "$WEB_CXX" ]; then
		printf "$FMT_RED$FMT_BOLD""web-build: can't find emscripten""$FMT_OFF\n"
		exit 1
	fi
fi

if [ ! -d $PROG_DIR ]; then
	mkdir -p $PROG_DIR
fi

PINT_SRC_URL="https://raw.githubusercontent.com/glebnovodran/proto-plop/main/pint/src"
PINT_SRCS="pint.hpp pint.cpp"


CORE_SRC_URL="https://raw.githubusercontent.com/schaban/crosscore_dev/main/src"
CORE_SRCS="crosscore.hpp crosscore.cpp demo.hpp demo.cpp draw.hpp oglsys.hpp oglsys.cpp oglsys.inc scene.hpp scene.cpp smprig.hpp smprig.cpp smpchar.hpp smpchar.cpp draw_ogl.cpp main.cpp"
CORE_OGL_SRCS="gpu_defs.h progs.inc shaders.inc"

CORE_MAC_SRCS=""
if [ "$SYS_NAME" = "Darwin" ]; then
	CORE_MAC_SRCS="mac_main.m mac_ifc.h"
fi

DL_MODE="NONE"
DL_CMD=""

if [ -x "`command -v curl`" ]; then
	DL_MODE="CURL"
	DL_CMD="curl -o"
elif [ -x "`command -v wget`" ]; then
	DL_MODE="WGET"
	DL_CMD="wget -O"
fi

NEED_PINT=0
if [ ! -d $PINT_DIR ]; then
	mkdir -p $PINT_DIR
	NEED_PINT=1
else
	for src in $PINT_SRCS; do
		if [ $NEED_PINT -ne 1 ]; then
			printf $PINT_DIR/$src
			if [ ! -f $PINT_DIR/$src ]; then
				NEED_PINT=1
			fi
		fi
	done
fi

NEED_CORE=0
if [ ! -d $CORE_DIR ]; then
	mkdir -p $CORE_DIR
	mkdir -p $CORE_DIR/ogl
	NEED_CORE=1
else
	for src in $CORE_SRCS; do
		if [ $NEED_CORE -ne 1 ]; then
			if [ ! -f $CORE_DIR/$src ]; then
				NEED_CORE=1
			fi
		fi
	done
	if [ ! -d $CORE_DIR/ogl ]; then
		mkdir -p $CORE_DIR/ogl
		NEED_CORE=1
	fi
	if [ $NEED_CORE -ne 1 ]; then
		for src in $CORE_OGL_SRCS; do
			if [ ! -f $CORE_DIR/ogl/$src ]; then
				NEED_CORE=1
			fi
		done
	fi
	if [ $NEED_CORE -ne 1 ]; then
		if [ -n "$CORE_MAC_SRCS" ]; then
			for src in $CORE_MAC_SRCS; do
				if [ ! -f $CORE_DIR/mac/$src ]; then
					NEED_CORE=1
				fi
			done
		fi
	fi
fi

if [ $NEED_PINT -ne 0 ]; then
	printf "$FMT_B_RED""-> Downloading pint sources...""$FMT_OFF\n"
	for src in $PINT_SRCS; do
		if [ ! -f $PINT_DIR/$src ]; then
			printf "$FMT_B_BLUE""     $src""$FMT_OFF\n"
			$DL_CMD $PINT_DIR/$src $PINT_SRC_URL/$src
		fi
	done
fi

if [ $NEED_CORE -ne 0 ]; then
	printf "$FMT_B_RED""-> Downloading crosscore sources...""$FMT_OFF\n"
	for src in $CORE_SRCS; do
		if [ ! -f $CORE_DIR/$src ]; then
			printf "$FMT_B_BLUE""     $src""$FMT_OFF\n"
			$DL_CMD $CORE_DIR/$src $CORE_SRC_URL/$src
		fi
	done
	for src in $CORE_OGL_SRCS; do
		if [ ! -f $CORE_DIR/ogl/$src ]; then
			printf "$FMT_CYAN""     $src""$FMT_OFF\n"
			$DL_CMD $CORE_DIR/ogl/$src $CORE_SRC_URL/ogl/$src
		fi
	done
	if [ -n "$CORE_MAC_SRCS" ]; then
		mkdir -p $CORE_DIR/mac
		for src in $CORE_MAC_SRCS; do
			if [ ! -f $CORE_DIR/mac/$src ]; then
				printf "$FMT_CYAN""     $src""$FMT_OFF\n"
				$DL_CMD $CORE_DIR/mac/$src $CORE_SRC_URL/mac/$src
			fi
		done
	fi
fi

NEED_OGL_INC=0
if [ $WEB_BUILD -eq 0 ]; then
	case $SYS_NAME in
		Linux | SunOS)
			NEED_OGL_INC=1
		;;
	esac
fi

if [ $NEED_OGL_INC -ne 0 ]; then
	KHR_REG_URL=https://registry.khronos.org
	INC_OGL=$INC_DIR/GL
	if [ ! -d $INC_OGL ]; then
		mkdir -p $INC_OGL
	fi
	OGL_INC_DL=0
	for kh in glext.h glcorearb.h; do
		if [ ! -f $INC_OGL/$kh ]; then
			if [ $OGL_INC_DL -ne 1 ]; then
				printf "$FMT_OFF""-> Downloading OpenGL headers...\n"
				OGL_INC_DL=1
			fi
			printf "$FMT_B_GREEN""     $kh""$FMT_OFF\n"
			$DL_CMD $INC_OGL/$kh $KHR_REG_URL/OpenGL/api/GL/$kh
		fi
	done
	INC_KHR=$INC_DIR/KHR
	if [ ! -d $INC_KHR ]; then
		mkdir -p $INC_KHR
	fi
	for kh in khrplatform.h; do
		if [ ! -f $INC_KHR/$kh ]; then
			if [ $OGL_INC_DL -ne 1 ]; then
				printf "$FMT_OFF""-> Downloading OpenGL headers...\n"
				OGL_INC_DL=1
			fi
			printf "$FMT_B_GREEN""     $kh""$FMT_OFF\n"
			$DL_CMD $INC_KHR/$kh $KHR_REG_URL/EGL/api/KHR/$kh
		fi
	done
fi

INCS="-I $CORE_DIR -I $INC_DIR -I $PINT_DIR"
SRCS="`ls $SRC_DIR/*.cpp` `ls $CORE_DIR/*.cpp` `ls $PINT_DIR/*.cpp`"

DEFS="-DX11"
LIBS="-lX11"

DEF_CXX="g++"
case $SYS_NAME in
	OpenBSD)
		INCS="$INCS -I/usr/X11R6/include"
		LIBS="$LIBS -L/usr/X11R6/lib"
		DEF_CXX="clang++"
	;;
	FreeBSD)
		INCS="$INCS -I/usr/local/include"
		LIBS="$LIBS -lpthread -L/usr/local/lib"
		DEF_CXX="clang++"
	;;
	Darwin)
		DEFS=""
		LIBS="-framework Foundation -framework Cocoa -framework OpenGL -Xlinker -w"
		DEF_CXX="clang++"
	;;
	Linux)
		LIBS="$LIBS -ldl -lpthread"
	;;
esac
CXX=${CXX:-$DEF_CXX}

if [ "$#" -gt 0 ]; then
	case $1 in
		srcs)
			shift
			SRCS_OUT=
			printf "$FMT_GREEN""-> Dumping sources list...""$FMT_OFF\n"
			if [ "$#" -gt 0 ]; then
				SRCS_OUT=$1
				shift
			fi
			SCRCS_FLG=0
			if [ "$#" -gt 0 ]; then
				if [ "$1" = "append" ]; then
					SCRCS_FLG=1
				fi
				shift
			fi
			for src in $SRCS; do
				if [ -n "$SRCS_OUT" ]; then
					if [ $SCRCS_FLG -ne 0 ]; then
						echo $src >> $SRCS_OUT
					else
						echo $src > $SRCS_OUT
					fi
				else
					echo $src
				fi
				SCRCS_FLG=1
			done
			printf "Done.\n"
			exit 0
		;;
	esac
fi


if [ $WEB_BUILD -ne 0 ]; then
	PROG_HTML=$PROG_PATH.html
	if [ ! -d "$WEB_TMP_DIR" ]; then
		mkdir -p $WEB_TMP_DIR
	fi
	XROM_PATH="$BUILD_PATH/$WEB_TMP_DIR/xrom.cpp"
	MKROM_SRC="$WEB_TMP_DIR/mkrom.cpp"
	if [ ! -f "$MKROM_SRC" ]; then
		printf "$FMT_B_BLUE""Downloading mkrom...""$FMT_OFF\n"
		$DL_CMD $MKROM_SRC $CORE_SRC_URL/etc/rom/mkrom.cpp
	fi
	MKROM_EXE="$BUILD_PATH/$WEB_TMP_DIR/mkrom"
	if [ ! -f "$MKROM_EXE" ]; then
		printf "$FMT_B_BLUE""Compiling mkrom...""$FMT_OFF\n"
		MKROM_CXX_FLGS="-O2 -flto -pthread -I $CORE_DIR"
		MKROM_CXX_SRCS="$CORE_DIR/crosscore.cpp $MKROM_SRC"
		$CXX $MKROM_CXX_FLGS $MKROM_CXX_SRCS -o $MKROM_EXE
	fi
	printf "Making ROM file:\n"
	cd $DATA_DIR
	ls -1d */* | $MKROM_EXE -nobin:1 -txt:$XROM_PATH
	cd $BUILD_PATH
	WEB_OPTS="-std=c++11 -O3"
	WEB_OPTS="$WEB_OPTS -s WASM=0 -s SINGLE_FILE"
	WEB_OPTS="$WEB_OPTS -s SINGLE_FILE -s USE_SDL=2 -s ASSERTIONS=1"
	WEB_OPTS="$WEB_OPTS -s INITIAL_MEMORY=50MB -s ALLOW_MEMORY_GROWTH=1"
	WEB_OPTS="$WEB_OPTS -DXD_THREADFUNCS_ENABLED=0 -DXD_FILEFUNCS_ENABLED=0"
	WEB_OPTS="$WEB_OPTS -DOGLSYS_WEB -DUSE_XROM_ARCHIVE"
	printf "Compiling \"$FMT_BOLD$FMT_B_MAGENTA$PROG_HTML$FMT_OFF\" with $FMT_BOLD$WEB_CXX$FMT_OFF.\n"
	$WEB_CXX $WEB_OPTS $INCS $SRCS $XROM_PATH --pre-js src/web/opt.js --shell-file src/web/shell.html -o $PROG_HTML -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -s EXPORTED_FUNCTIONS='["_main","_wi_set_key_state"]' $*
	if [ -f "$PROG_HTML" ]; then
		sed -i 's/antialias:!1/antialias:1/g' $PROG_HTML
		DATE_EXPR="s/Build date:.../Build date: $BUILD_DATE | JS\/ROM ver./g"
		sed -i "$DATE_EXPR" $PROG_HTML
		printf "web-build: ""$FMT_B_GREEN""Success""$FMT_OFF""$FMT_BOLD""!!""$FMT_OFF\n"
	else
		printf "web-build: ""$FMT_B_RED""Failure""$FMT_OFF...\n"
	fi
	exit
fi

printf "Compiling \"$FMT_BOLD$FMT_B_MAGENTA$PROG_PATH$FMT_OFF\" with $FMT_BOLD$CXX$FMT_OFF.\n"
rm -f $PROG_PATH
rm -f $RUN_PATH
if [ "$SYS_NAME" = "Darwin" ]; then
	if [ ! -d $TMP_DIR ]; then
		mkdir -p $TMP_DIR
	fi
	MAC_MAIN_SRC=$CORE_DIR/mac/mac_main.m
	MAC_MAIN_OBJ=$TMP_DIR/mac_main.o
	rm -f $MAC_MAIN_OBJ
	clang -Wno-deprecated-declarations $MAC_MAIN_SRC -Og -c -o $MAC_MAIN_OBJ
	if [ ! -f $MAC_MAIN_OBJ ]; then
		exit 1
	fi
	SRCS="$SRCS $MAC_MAIN_OBJ"
	INCS="$INCS -I $CORE_DIR/mac"
fi

$CXX -std=c++11 -ggdb $DEFS $INCS $SRCS -o $PROG_PATH $LIBS $*

if [ -f "$PROG_PATH" ]; then
	printf "#!/bin/sh\n\n" > $RUN_PATH
	printf "./$PROG_PATH -nwrk:0 \$*\n" >> $RUN_PATH
	chmod +x $RUN_PATH
	printf "$FMT_B_GREEN""Success""$FMT_OFF""$FMT_BOLD""!!""$FMT_OFF\n"
else
	printf "$FMT_B_RED""Failure""$FMT_OFF...\n"
fi

