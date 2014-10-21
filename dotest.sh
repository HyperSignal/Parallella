#!/bin/bash

python testgen.py
rm output.mat
START=$(($(sed < /proc/timer_list -ne '/^now at/{s/now at \([0-9]*\) ns.*$/\1/;H};/offset/{s/^.*: *\([0-9]*\) ns.*$/\1/;G;s/\n\+/+/;p;q}')))
#./run.sh -czt -w 384 -h 384 -A 64 -B 64 -C 320 -D 320 > /dev/null
cat testpacket.bin | ./run.sh -ktz > /tmp/output.mat
cp -fr /tmp/output.mat .
END=$(($(sed < /proc/timer_list -ne '/^now at/{s/now at \([0-9]*\) ns.*$/\1/;H};/offset/{s/^.*: *\([0-9]*\) ns.*$/\1/;G;s/\n\+/+/;p;q}')))

DELTA=`expr $END - $START - 23000000`
DELTA=`expr $DELTA / 1000`
echo "Delta: $DELTA microseconds"
