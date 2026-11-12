#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p .pio/tests
c++ -std=c++17 -Wall -Wextra -Werror test/native/test_logic.cpp -o .pio/tests/logic
.pio/tests/logic
node --check data/app.js
