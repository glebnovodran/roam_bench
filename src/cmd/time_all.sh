#!/bin/sh

RES_FILE=roam_times.txt
TIME_CMD=/usr/bin/time
FRAMES=${FRAMES-3000}
CMD_OPTS="$*"

echo "# Roam Scripting Benchmark results for $FRAMES frames" > $RES_FILE
echo "`uname -m`" >> $RES_FILE
cat /proc/cpuinfo | grep "model name" | head -n 1 >> $RES_FILE
cat /proc/cpuinfo | grep "cpu MHz" | head -n 1 >> $RES_FILE
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_governor >> $RES_FILE
if [ -n "$CMD_OPTS" ]; then
	echo "cmd: $CMD_OPTS" >> $RES_FILE
fi
echo "" >> $RES_FILE

for prog in native lua wrench qjs pint; do
	$TIME_CMD -f "$prog\t: %E" -a -o $RES_FILE ./run.sh -quit_frame:$FRAMES -roamprog:$prog $CMD_OPTS
done
