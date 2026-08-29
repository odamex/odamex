#!/bin/bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

# Exit if any command fails
set -e

# Echo all commands for debug purposes
set -x

# Assemble Flatpak assets

# Install timidity config
install -Dm644 packaging/flatpak/timidity.cfg /app/etc/timidity.cfg

# Install helper script
install -Dm755 packaging/flatpak/select-exe.sh /app/bin/select-exe

# Install shell integration
install -Dm644 packaging/flatpak/posix_aliases.sh /app/share/odamex/shell/posix_aliases.sh
