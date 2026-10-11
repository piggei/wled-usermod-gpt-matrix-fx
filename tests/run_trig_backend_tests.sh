#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
for backend in 0 1 2; do
  "$CXX" -std=c++11 -O2 -Wall -Wextra -Wshadow -DGPTFX_GPT24_TRIG_BACKEND="$backend" -I. \
    -c compile_arduino_compat.cpp -o "/tmp/gptfx-trig-compat-$backend.o"
  "$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow -DGPTFX_GPT24_TRIG_BACKEND="$backend" -I. \
    test_host.cpp -o "/tmp/gptfx-trig-test-$backend"
  "/tmp/gptfx-trig-test-$backend" > "/tmp/gptfx-trig-test-$backend.log"
  grep -E '^(TRIG_LUT_NUMERIC_PASS|GPT24_LUT_DEV9_FRAME_PASS|HOST_TEST_PASS)' \
    "/tmp/gptfx-trig-test-$backend.log"
done

# Independent extra-effect backend A/B (GPT24 stays on default 2).
for backend in 0 2; do
  "$CXX" -std=c++11 -O2 -Wall -Wextra -Wshadow -DGPTFX_EXTRA_TRIG_BACKEND="$backend" -I. \
    -c compile_arduino_compat.cpp -o "/tmp/gptfx-extra-compat-$backend.o"
  "$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow -DGPTFX_EXTRA_TRIG_BACKEND="$backend" -I. \
    test_host.cpp -o "/tmp/gptfx-extra-test-$backend"
  "/tmp/gptfx-extra-test-$backend" > "/tmp/gptfx-extra-test-$backend.log"
  grep -E '^(EXTRA_TRIG_FRAME_PASS|EXTRA_TRIG_TELEMETRY_PASS|HOST_TEST_PASS)' \
    "/tmp/gptfx-extra-test-$backend.log"
done

# GPT18 remains native by default. Verify its independent experimental LUT.
"$CXX" -std=c++11 -O2 -Wall -Wextra -Wshadow -DGPTFX_GPT18_TRIG_BACKEND=2 -I. \
  -c compile_arduino_compat.cpp -o /tmp/gptfx-gpt18-compat-2.o
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow -DGPTFX_GPT18_TRIG_BACKEND=2 -I. \
  test_host.cpp -o /tmp/gptfx-gpt18-test-2
/tmp/gptfx-gpt18-test-2 > /tmp/gptfx-gpt18-test-2.log
grep -E '^(EXTRA_TRIG_FRAME_PASS GPT18|EXTRA_LARGE_FRAME_PASS GPT18|HOST_TEST_PASS)' /tmp/gptfx-gpt18-test-2.log
