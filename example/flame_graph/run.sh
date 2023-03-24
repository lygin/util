#!/bin/bash
rm -rf perf.data* perf.folded perf.unfold out.perf
# 1.install perf: apt-get install linux-tools-common linux-tools-generic linux-tools-`uname -r`
FlameSRC=../../third_party/FlameGraph
# 2.clone FlameGraph
# 3.grep your proc name
PID=`ps -ef | grep ./perf_test | grep -v grep | awk '{print $2}'`
echo $PID
if [ ${#PID} -eq 0 ]
then
  echo "process not found"
  exit -1
fi

# -F 99: collect 99 times per sec
# -g: collect stack
# -a: collect all cpus
# -p: your proc pid
# -- sleep 60: capature 60s
perf record -F 999 -g -p $PID -- sleep 30
perf script  > out.perf
echo "perf done"
$FlameSRC/stackcollapse-perf.pl out.perf > perf.folded
$FlameSRC/flamegraph.pl perf.folded > perf.svg
echo "success"
rm -rf perf.data* perf.folded perf.unfold out.perf