#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

rm -rf "$SCRIPT_DIR/.flatpak-builder/build" "$SCRIPT_DIR/build_client"

flatpak-builder --user --install build_launcher net.odamex.Odamex.yaml
