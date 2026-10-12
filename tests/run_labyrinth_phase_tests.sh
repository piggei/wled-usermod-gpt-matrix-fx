#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
for bounding in 0 1; do
  for backend in 0 2; do
    "$CXX" -std=c++11 -O2 -Wall -Wextra -Wshadow \
      -DGPTFX_GPT18_PHASE_BOUNDING="$bounding" \
      -DGPTFX_GPT18_TRIG_BACKEND="$backend" \
      -I. -c compile_arduino_compat.cpp -o "/tmp/gptfx-lab-compat-$bounding-$backend.o"
    "$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow \
      -DGPTFX_GPT18_PHASE_BOUNDING="$bounding" \
      -DGPTFX_GPT18_TRIG_BACKEND="$backend" \
      -I. test_labyrinth_phase.cpp -o "/tmp/gptfx-lab-test-$bounding-$backend"
    "/tmp/gptfx-lab-test-$bounding-$backend"
  done
done
