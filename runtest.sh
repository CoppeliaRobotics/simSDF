#!/bin/sh
inst_dir="../../vrep.app/Contents/MacOS"
scene_file="$(cd "$(dirname "$0")"; pwd)/runtest.ttt"
timeout=500
make debug && cp libv_repExtSDF.1.0.0.dylib "$inst_dir" && cd "$inst_dir" && if [[ $DEBUG -gt 0 ]]; then lldb --batch --one-line run ./vrep -- -s$timeout -q -g"$1" "$scene_file"; else ./vrep -s$timeout -q -g"$1" "$scene_file"; fi
