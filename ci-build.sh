#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

echo "which gcc = $(which gcc)"
echo "which clang = $(which clang)"
echo "which ninja = $(which ninja)"

mkdir -p build && cd build
cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=Debug -DUSE_COLOR_DIAGNOSTICS=1 \
    -DCMAKE_MAKE_PROGRAM="$(which ninja)"
