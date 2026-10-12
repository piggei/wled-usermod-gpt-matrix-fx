#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
for bounding in 0 1; do
  "$CXX" -std=c++11 -O2 -Wall -Wextra -Wshadow \
    -DGPTFX_STAGE1_PHASE_BOUNDING="$bounding" \
    -I. -c compile_arduino_compat.cpp -o "/tmp/gptfx-stage1-compat-$bounding.o"
  "$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow \
    -DGPTFX_STAGE1_PHASE_BOUNDING="$bounding" \
    -I. test_stage1_phase.cpp -o "/tmp/gptfx-stage1-phase-$bounding"
  "/tmp/gptfx-stage1-phase-$bounding" \
    "../PERFORMANCE_STAGE1_VISUAL_${bounding}.csv"
  "$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow \
    -DGPTFX_STAGE1_PHASE_BOUNDING="$bounding" \
    -I. test_stage1_audio.cpp -o "/tmp/gptfx-stage1-audio-$bounding"
  "/tmp/gptfx-stage1-audio-$bounding"
done
