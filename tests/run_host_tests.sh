#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
"$CXX" -std=c++11 -O2 -Wall -Wextra -Wshadow -I. -c compile_arduino_compat.cpp -o /tmp/gpt-matrix-fx-arduino-compat.o
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow \
  -I. test_host.cpp -o /tmp/gpt-matrix-fx-host-test
/tmp/gpt-matrix-fx-host-test
