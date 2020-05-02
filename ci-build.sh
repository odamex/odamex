#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

mkdir -p build && cd build
cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=Debug -DUSE_COLOR_DIAGNOSTICS=1
