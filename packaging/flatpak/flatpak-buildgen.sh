#!/bin/bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

# Exit if any command fails
set -e

# Echo all commands for debug purposes
set -x

# Generate build
if [[ -z ${USE_SDL12:-} ]]; then
    cmake . -GNinja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
        -DBUILD_MASTER=0 -DBUILD_LAUNCHER=0 -DUSE_INTERNAL_LIBS=1
else
    cmake . -GNinja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_SDL12=1 \
        -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
        -DBUILD_MASTER=0 -DBUILD_LAUNCHER=0 -DUSE_INTERNAL_LIBS=1
fi

# No spaces in project name.
projectName=Odamex.Client
projectId=net.odamex.Odamex.Client
executableName=odamex

# Assemble Flatpak assets
# Copy the portable app to the Flatpak-based location.
cp -r $projectName /app/
chmod +x /app/$projectName/$executableName
mkdir -p /app/bin
ln -s /app/$projectName/$executableName /app/bin/$executableName

# Install the icon.
iconDir=/app/share/icons/hicolor/512x512/apps
mkdir -p $iconDir
cp -r ./media/icon_odamex_512.png $iconDir/$projectId.png

# Install the desktop file.
desktopFileDir=/app/share/applications
mkdir -p $desktopFileDir
cp -r packaging/linux/$projectId.desktop $desktopFileDir/

# Install the AppStream metadata file.
metadataDir=/app/share/metainfo
mkdir -p $metadataDir
cp -r packaging/linux/$projectId.metainfo.xml $metadataDir/