#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

# Install packages (retry to handle transient mirror churn)
sudo apt-get update -y -o Acquire::Retries=3
if ! sudo apt-get install -y --no-install-recommends \
    ninja-build libsdl2-dev libsdl2-mixer-dev \
    libcurl4-openssl-dev libpng-dev libwxgtk3.2-dev deutex; then
  sudo apt-get update -y -o Acquire::Retries=3
  sudo apt-get install -y --no-install-recommends \
    ninja-build libsdl2-dev libsdl2-mixer-dev \
    libcurl4-openssl-dev libpng-dev libwxgtk3.2-dev deutex
fi

# Generate build
mkdir -p build && cd build
cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1
