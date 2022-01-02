#!/usr/bin/env bash

#
# Copyright (C) 2021 Alex Mayfield, The Odamex Team
#
# This script builds flatpaks for Odamex and Odalaunch, as well as a
# statically compiled Odasrv.
#

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Build Client
function oda_preflight {
    rm -rf "$SCRIPT_DIR/.flatpak-builder/build" \
        "$SCRIPT_DIR/build_client" \
        "$SCRIPT_DIR/build_launcher"

    flatpak remote-add --user --if-not-exists flathub \
        https://flathub.org/repo/flathub.flatpakrepo
    flatpak install --user --assumeyes flathub \
        org.freedesktop.Platform//21.08 \
        org.freedesktop.Sdk//21.08
}

function oda_build_client {
    flatpak-builder -v build_client net.odamex.Odamex.yaml
}

function oda_build_launcher {
    flatpak-builder -v build_launcher net.odamex.Odalaunch.yaml
}

oda_preflight
#oda_build_client
#oda_build_launcher
