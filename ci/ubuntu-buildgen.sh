#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

# Install packages
sudo apt update
if [[ -z ${USE_SDL12:-} ]]; then
    sudo apt install ninja-build libsdl2-dev libsdl2-mixer-dev \
        libcurl4-openssl-dev libpng-dev libwxgtk3.2-dev deutex
else
    sudo apt install ninja-build libsdl1.2-dev libsdl-mixer1.2-dev \
        libcurl4-openssl-dev libpng-dev libwxgtk3.2-dev deutex
fi

# Generate build
mkdir -p build && cd build
if [[ -z ${USE_SDL12:-} ]]; then
    cmake .. -GNinja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
        -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1
else
    cmake .. -GNinja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_SDL12=1 \
        -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
        -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1
fi
