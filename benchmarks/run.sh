#!/bin/bash

ROOT_DIR=$(git rev-parse --show-toplevel)
mkdir -p $ROOT_DIR/benchmarks/build
BUILD_DIR=$ROOT_DIR/benchmarks/build
BUILDIT_DIR=$ROOT_DIR/buildit

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ROOT_DIR/benchmarks/re2/obj/so

g++ -O3 performance.cpp -o $BUILD_DIR/perf -I$ROOT_DIR/generated_code -Ihyperscan -Ire2 -I$BUILDIT_DIR/include -I$BUILDIT_DIR/build/gen_headers -I$ROOT_DIR/include -Lhyperscan/build/lib/ -lhs -Lre2/obj/so -lre2 -L$BUILDIT_DIR/build -lbuildit -L$ROOT_DIR/build -lbuildit_regex -ldl

exec $BUILD_DIR/perf
