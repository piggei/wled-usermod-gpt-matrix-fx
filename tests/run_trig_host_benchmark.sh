#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
for backend in 0 1 2; do
  "$CXX" -std=c++17 -O3 -DGPTFX_GPT24_TRIG_BACKEND="$backend" -I. \
    benchmark_lut_host.cpp -o "/tmp/gptfx-trig-bench-$backend"
  "/tmp/gptfx-trig-bench-$backend"
done
