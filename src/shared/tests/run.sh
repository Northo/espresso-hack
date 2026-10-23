#!/bin/sh
set -eu
cd "$(dirname "$0")"
bin=$(mktemp "${TMPDIR:-/tmp}/espresso-tests.XXXXXX")
trap 'rm -f "$bin"' EXIT
"${CXX:-c++}" -std=c++11 -Wall -Wextra -Werror -Wno-sign-compare -DBOARD_DEVKITC1 \
  -I stubs -I .. -I ../../firmware/src \
  test_brew.cpp ../../firmware/src/controller.cpp ../../firmware/src/brew_timer.cpp -o "$bin"
"$bin"
