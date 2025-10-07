#!/bin/bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

# Exit if any command fails
set -e

# Echo all commands for debug purposes
set -x

# Assemble Flatpak assets

# Install timidity config
mkdir -p /app/etc
cp -r packaging/flatpak/timidity.cfg /app/etc/timidity.cfg

# Install the AppStream metadata file.
projectId=net.odamex.Odamex
metadataDir=/app/share/metainfo
mkdir -p $metadataDir
cp -r packaging/linux/$projectId.metainfo.xml $metadataDir/

# Install the odd demo filetype
iconDir=/app/share/icons/hicolor/512x512/mimetypes
mkdir -p $iconDir
cp -r media/icon_odademo_512.png $iconDir/$projectId-application-odamex-demo.png
mimeDir=/app/share/mime/packages/
mkdir -p $mimeDir
cp -r packaging/linux/$projectId-mime.xml $mimeDir/$projectId-mime.xml

# Install the icons and .desktop files for the executables
oda_install() {
    local projectIdExt=$1
    local executableName=$2
    local buildFolder=$3

    # Install the icon.
    iconDir=/app/share/icons/hicolor/512x512/apps
    mkdir -p $iconDir
    cp -r media/icon_${executableName}_512.png $iconDir/$projectId.$projectIdExt.png

    # Install the desktop file.
    desktopFileDir=/app/share/applications
    mkdir -p $desktopFileDir
    cp -r packaging/linux/$projectId.$projectIdExt.desktop $desktopFileDir/
}

# Client app
oda_install Client odamex client

# Server app
oda_install Server odasrv server

# Launcher app
oda_install Launcher odalaunch odalaunch

# Install helper script
install -c packaging/flatpak/select-exe.sh /app/bin/select-exe
