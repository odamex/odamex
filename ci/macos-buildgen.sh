#!/usr/bin/env zsh

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

# Install packages
brew install ninja sdl2 sdl2_mixer wxwidgets zstd

# Fetch DeuTex source if missing.
if [ ! -d "wad/deutex" ]; then
  curl -L -o deutex.tar.zst \
    "https://github.com/Doom-Utils/deutex/releases/download/v5.2.3/deutex-5.2.3.tar.zst"
  mkdir -p wad/deutex
  tar --use-compress-program=unzstd -xf deutex.tar.zst -C wad/deutex --strip-components=1
fi

# Generate build
mkdir -p build && cd build
cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1 \
    -DUSE_INTERNAL_ZLIB=1 -DUSE_INTERNAL_PNG=1 \
    -DUSE_INTERNAL_DEUTEX=1
