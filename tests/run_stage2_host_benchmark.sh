#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wshadow -I. benchmark_stage2_host.cpp -o /tmp/gptfx-stage2-host-bench
/tmp/gptfx-stage2-host-bench > ../PERFORMANCE_STAGE2_HOST_DEV14.csv
