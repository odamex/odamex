#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

function oda_build {
    flatpak-builder --repo="$SCRIPT_DIR/repo" --force-clean build_launcher net.odamex.Odalaunch.yaml
    flatpak --user remote-add --if-not-exists --no-gpg-verify odamex-repo ./repo
    flatpak --user install --reinstall --assumeyes odamex-repo net.odamex.Odalaunch net.odamex.Odalaunch.Debug
}

function oda_shell {
    flatpak run --devel --command=sh net.odamex.Odalaunch
}

function oda_run {
    flatpak run --devel net.odamex.Odalaunch
}

if [ $# -gt 0 ]; then
    SUBCMD="$1"
else
    SUBCMD="run"
fi

case "$SUBCMD" in
    rebuild)
        oda_build
        ;;
    shell)
        oda_shell
        ;;
    run)
        oda_run
        ;;
    *)
        echo "requires one of rebuild|shell|run"
        ;;
esac
