#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

# Install packages
sudo apt update
sudo apt install ninja-build libsdl2-dev libsdl2-mixer-dev \
    libcurl4-openssl-dev libpng-dev libwxgtk3.2-dev deutex

# Generate build
mkdir -p build && cd build
cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1
