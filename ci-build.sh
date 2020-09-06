#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

if [[ $(uname -s) == "Linux" ]]; then
    # Ubuntu
    sudo apt update
    if [[ -z ${USE_SDL12:-} ]]; then
        sudo apt install ninja-build libsdl2-dev libsdl2-mixer-dev \
            libwxgtk3.0-gtk3-dev deutex
    else
        sudo apt install ninja-build libsdl1.2-dev libsdl-mixer1.2-dev \
            libwxgtk3.0-gtk3-dev deutex
    fi
else
    # macOS
    brew update
    brew install ninja sdl2 sdl2_mixer wxmac
fi

mkdir -p build && cd build
if [[ -z ${USE_SDL12:-} ]]; then
    cmake .. -GNinja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_COLOR_DIAGNOSTICS=1
else
    cmake .. -GNinja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_COLOR_DIAGNOSTICS=1 -DUSE_SDL12=1
fi
