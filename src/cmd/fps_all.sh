#!/bin/sh

RES_FILE=roam_fps.txt
TIME_CMD=/usr/bin/time
FRAMES=${FRAMES-3000}
CMD_OPTS="$*"

echo "# Roam Scripting Benchmark FPS results for $FRAMES frames" > $RES_FILE
echo "`uname -m`" >> $RES_FILE
cat /proc/cpuinfo | grep "model name" | head -n 1 >> $RES_FILE
cat /proc/cpuinfo | grep "cpu MHz" | head -n 1 >> $RES_FILE
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_governor >> $RES_FILE
if [ -n "$CMD_OPTS" ]; then
	echo "cmd: $CMD_OPTS" >> $RES_FILE
fi
echo "" >> $RES_FILE

for prog in native lua wrench qjs pint minion; do
	echo Benchmarking $prog...
	./run.sh -draw_2d_disable:1 -quit_frame:$FRAMES -roamprog:$prog $CMD_OPTS >> $RES_FILE
	echo "" >> $RES_FILE
done

grep "prog kind\|avgFPS" $RES_FILE
