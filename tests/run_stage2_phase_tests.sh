#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
for bounding in 0 1; do
  "$CXX" -std=c++11 -O2 -Wall -Wextra -Wshadow \
    -DGPTFX_STAGE2_PHASE_BOUNDING="$bounding" \
    -I. -c compile_arduino_compat.cpp -o "/tmp/gptfx-stage2-compat-$bounding.o"
  "$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow \
    -DGPTFX_STAGE2_PHASE_BOUNDING="$bounding" \
    -I. test_stage2_phase.cpp -o "/tmp/gptfx-stage2-phase-$bounding"
  "/tmp/gptfx-stage2-phase-$bounding" \
    "../PERFORMANCE_STAGE2_VISUAL_${bounding}.csv"
done
