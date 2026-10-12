#!/bin/sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
for cache in 0 1; do
  for packet in 0 1; do
    echo "TEST cache=$cache packet=$packet"
    "$CXX" -std=c++11 -O2 -Wall -Wextra -Wshadow \
      -DGPTFX_GPT10_CELL_CACHE="$cache" -DGPTFX_GPT10_PACKET_BOUNDING="$packet" \
      -I. -c compile_arduino_compat.cpp -o "/tmp/gptfx-gpt10-compat-$cache-$packet.o"
    "$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow \
      -DGPTFX_GPT10_CELL_CACHE="$cache" -DGPTFX_GPT10_PACKET_BOUNDING="$packet" \
      -I. test_gpt10_remediation.cpp -o "/tmp/gptfx-gpt10-test-$cache-$packet"
    "/tmp/gptfx-gpt10-test-$cache-$packet" "../PERFORMANCE_GPT10_VISUAL_${cache}_${packet}.csv"
  done
done
