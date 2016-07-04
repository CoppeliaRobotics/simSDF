#!/bin/sh
inst_dir="../../vrep.app/Contents/MacOS"
scene_file="$(cd "$(dirname "$0")"; pwd)/runtest.ttt"
test_file="$(cd "$(dirname "$1")"; pwd)/$(basename "$1")"
timeout=500
make debug && cp -v libv_repExtSDF.1.0.0.dylib "$inst_dir" && cd "$inst_dir" && if [[ $DEBUG -gt 0 ]]; then lldb --batch --one-line run ./vrep -- -s$timeout -q -g"$test_file" "$scene_file"; elif [[ "x$ONLYCOPY" = "x" ]]; then ./vrep -s$timeout -q -g"$test_file" "$scene_file"; fi
