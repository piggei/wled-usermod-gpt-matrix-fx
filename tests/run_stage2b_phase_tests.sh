#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
for bounding in 0 1; do
  "$CXX" -std=c++11 -O2 -Wall -Wextra -Wshadow -DGPTFX_STAGE2B_PHASE_BOUNDING="$bounding" -I. -c compile_arduino_compat.cpp -o "/tmp/gptfx-stage2b-compat-$bounding.o"
  "$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow -DGPTFX_STAGE2B_PHASE_BOUNDING="$bounding" -I. test_stage2b_phase.cpp -o "/tmp/gptfx-stage2b-phase-$bounding"
  "/tmp/gptfx-stage2b-phase-$bounding" "../PERFORMANCE_STAGE2B_VISUAL_${bounding}.csv"
done
