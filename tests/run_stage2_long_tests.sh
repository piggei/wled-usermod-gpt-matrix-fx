#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
for bounding in 0 1; do
  "$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow \
    -DGPTFX_STAGE2_PHASE_BOUNDING="$bounding" -I. test_stage2_phase_long.cpp \
    -o "/tmp/gptfx-stage2-long-$bounding"
  "/tmp/gptfx-stage2-long-$bounding" \
    "../PERFORMANCE_STAGE2_LONG_${bounding}.csv" \
    2> "../PERFORMANCE_STAGE2_LONG_${bounding}.log"
done
