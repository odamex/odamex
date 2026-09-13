#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

mkdir -p build/artifact && cd build

cp -aR \
    "odamex/odamex" \
    "odamex/odasrv" \
    "odamex/odalaunch" \
    "odamex/odamex.wad" \
    artifact/
