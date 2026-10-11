#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
for backend in 0 2; do
  "$CXX" -std=c++17 -O3 -Wall -Wextra -Wshadow -DGPTFX_EXTRA_TRIG_BACKEND="$backend" -I. \
    benchmark_multi_host.cpp -o "/tmp/gptfx-multi-bench-$backend"
  "/tmp/gptfx-multi-bench-$backend"
done
