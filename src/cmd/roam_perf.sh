#!/bin/sh

PROG_ID="$(ps -a | grep roam_bench | awk '{print $1}')"
if [ -n "$PROG_ID" ]; then
	echo "found prog @ PID = $PROG_ID"
	perf record --freq=MAX --pid=$PROG_ID --user-callchains -- sleep 10
	perf report -n --stdio
else
	echo "prog not found"
fi