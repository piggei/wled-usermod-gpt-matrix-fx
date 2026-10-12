#!/bin/sh
set -eu
cd "$(dirname "$0")"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Wshadow -I. benchmark_gpt10_host.cpp -o /tmp/gptfx-gpt10-bench
/tmp/gptfx-gpt10-bench > ../PERFORMANCE_GPT10_HOST_DEV14A.csv
