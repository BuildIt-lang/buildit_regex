#!/bin/bash

ROOT_DIR=$(git rev-parse --show-toplevel)
mkdir -p $ROOT_DIR/benchmarks/build
BUILD_DIR=$ROOT_DIR/benchmarks/build

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ROOT_DIR/benchmarks/re2/obj/so

g++ -O3 performance.cpp -o $BUILD_DIR/perf -Ihyperscan -Ire2 -Lhyperscan/build/lib/ -lhs -Lre2/obj/so -lre2

exec $BUILD_DIR/perf
