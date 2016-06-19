#!/bin/sh
inst_dir="../../vrep.app/Contents/MacOS"
scene_file="$(cd "$(dirname "$0")"; pwd)/runtest.ttt"
make debug && cp libv_repExtSDF.1.0.0.dylib "$inst_dir" && cd "$inst_dir" && ./vrep -s -q -g"$1" "$scene_file"
