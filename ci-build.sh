#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

if [[ $(uname -s) == "Linux" ]]; then
    # Ubuntu
    sudo apt install ninja-build libsdl2-dev libsdl2-mixer-dev
else
    # macOS
    brew install ninja sdl2 sdl2_mixer
fi

mkdir -p build && cd build
cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_COLOR_DIAGNOSTICS=1
