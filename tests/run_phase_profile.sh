#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow -I. profile_phase_uptime.cpp -o /tmp/gptfx-phase-profile
/tmp/gptfx-phase-profile
