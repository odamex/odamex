#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

mkdir -p build/artifact && cd build

cp -aR \
    "client/odamex" \
    "client/odamex.dbg" \
    "server/odasrv" \
    "server/odasrv.dbg" \
    "odalaunch/odalaunch" \
    "odalaunch/odalaunch.dbg" \
    "wad/odamex.wad" \
    artifact/
