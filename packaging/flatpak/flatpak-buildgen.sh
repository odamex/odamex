#!/bin/bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

# Exit if any command fails
set -e

# Echo all commands for debug purposes
set -x

# Untar deutex to wad directory
tar --zstd -xvf deutex-5.2.2.tar.zst -C wad/

# Untar wxWidgets to libraries directory
tar -xvf wxWidgets-3.0.5.tar.bz2 -C libraries/

# Generate build
if [[ -z ${USE_SDL12:-} ]]; then
    cmake . -GNinja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
        -DBUILD_MASTER=0 -DBUILD_LAUNCHER=1 -DUSE_INTERNAL_LIBS=1 \
        -DUSE_INTERNAL_WXWIDGETS=1
else
    cmake . -GNinja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_SDL12=1 \
        -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
        -DBUILD_MASTER=0 -DBUILD_LAUNCHER=1 -DUSE_INTERNAL_LIBS=1 \
        -DUSE_INTERNAL_WXWIDGETS=1
fi

ninja odamex
ninja odasrv

# Copy wxWidgets dependencies into lib
# Before running wxrc
mkdir -p /app/lib
cp -r libraries/wxWidgets-3.0.5/lib/*.so /app/lib/
cp -r libraries/wxWidgets-3.0.5/lib/*.0 /app/lib/

ninja odalaunch

# Assemble Flatpak assets

# Client app
projectName=Odamex.Client
projectId=net.odamex.Odamex.Client
executableName=odamex

# Copy the client app to the Flatpak-based location.
mkdir -p /app/$projectName
cp -r client/$executableName /app/$projectName/
chmod +x /app/$projectName/$executableName
mkdir -p /app/bin
cp -r wad/odamex.wad /app/$projectName/
cp -r wad/odamex.wad /app/bin/
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

# Server app
projectName=Odamex.Server
projectId=net.odamex.Odamex.Server
executableName=odasrv

# Copy the server app to the Flatpak-based location.
mkdir -p /app/$projectName
cp -r server/$executableName /app/$projectName/
chmod +x /app/$projectName/$executableName
mkdir -p /app/bin
cp -r wad/odamex.wad /app/$projectName/
cp -r wad/odamex.wad /app/bin/
ln -s /app/$projectName/$executableName /app/bin/$executableName

# Install the icon.
iconDir=/app/share/icons/hicolor/512x512/apps
mkdir -p $iconDir
cp -r ./media/icon_odasrv_512.png $iconDir/$projectId.png

# Install the desktop file.
desktopFileDir=/app/share/applications
mkdir -p $desktopFileDir
cp -r packaging/linux/$projectId.desktop $desktopFileDir/

# Install the AppStream metadata file.
metadataDir=/app/share/metainfo
mkdir -p $metadataDir
cp -r packaging/linux/$projectId.metainfo.xml $metadataDir/

# Launcher app
projectName=Odamex.Launcher
projectId=net.odamex.Odamex.Launcher
executableName=odalaunch

# Copy the launcher app to the Flatpak-based location.
mkdir -p /app/$projectName
cp -r odalaunch/$executableName /app/$projectName/
chmod +x /app/$projectName/$executableName
mkdir -p /app/bin
ln -s /app/$projectName/$executableName /app/bin/$executableName

# Install the icon.
iconDir=/app/share/icons/hicolor/512x512/apps
mkdir -p $iconDir
cp -r ./media/icon_odalaunch_512.png $iconDir/$projectId.png

# Install the desktop file.
desktopFileDir=/app/share/applications
mkdir -p $desktopFileDir
cp -r packaging/linux/$projectId.desktop $desktopFileDir/

# Install the AppStream metadata file.
metadataDir=/app/share/metainfo
mkdir -p $metadataDir
cp -r packaging/linux/$projectId.metainfo.xml $metadataDir/