#!/bin/bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

# Exit if any command fails
set -e

# Echo all commands for debug purposes
set -x

# Assemble Flatpak assets

# Install timidity config
install -Dm644 packaging/flatpak/timidity.cfg /app/etc/timidity.cfg

# Install the AppStream metadata file.
projectId=net.odamex.Odamex
metadataDir=/app/share/metainfo
install -Dm644 packaging/linux/$projectId.metainfo.xml -t $metadataDir/
install -Dm644 packaging/linux/$projectId.releases.xml -t $metadataDir/releases/

# Install the odd demo filetype
iconDir=/app/share/icons/hicolor/512x512/mimetypes
install -Dm644 media/icon_odademo_512.png $iconDir/$projectId-application-odamex-demo.png
mimeDir=/app/share/mime/packages/
install -Dm644 packaging/linux/$projectId-mime.xml $mimeDir/$projectId-mime.xml

# Install the icons and .desktop files for the executables
oda_install() {
    local projectIdExt=$1
    local executableName=$2

    # Install the icon.
    iconDir=/app/share/icons/hicolor/512x512/apps
    install -Dm644 media/icon_${executableName}_512.png $iconDir/$projectId.$projectIdExt.png

    # Install the desktop file.
    local desktopFileDir=/app/share/applications
    install -Dm644 packaging/linux/$projectId.$projectIdExt.desktop -t $desktopFileDir/
}

# Client app
oda_install Client odamex

# Server app
oda_install Server odasrv

# Launcher app
oda_install Launcher odalaunch

# Install helper script
install -Dm755 packaging/flatpak/select-exe.sh /app/bin/select-exe

# Install shell integration
install -Dm644 packaging/flatpak/posix_aliases.sh /app/share/odamex/shell/posix_aliases.sh
