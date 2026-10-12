#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
"$CXX" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I. test_host.cpp -o /tmp/gpt-matrix-fx-asan
ASAN_OPTIONS=detect_leaks=0 /tmp/gpt-matrix-fx-asan
