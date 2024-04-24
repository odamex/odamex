#!/usr/bin/env zsh

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

# Install packages
brew install ninja sdl2 sdl2_mixer wxwidgets

# Generate build
mkdir -p build && cd build
cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1
