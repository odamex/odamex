#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

mkdir -p build/artifact && cd build

if [[ $(uname -s) == "Linux" ]]; then
    cp -aR \
        "client/odamex" "server/odasrv" "odalaunch/odalaunch" \
        "wad/odamex.wad" artifact/
else
    cp -pR \
        "client/odamex.app" "server/odasrv" "odalaunch/odalaunch.app" \
        "../wad/odamex.wad" artifact/
fi
