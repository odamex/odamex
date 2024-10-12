#
# Copyright (C) 2020 Alex Mayfield.
#
# If you double-clicked on the script expecting to run it, you need to right
# click Run With Powershell instead.
#

#
# These parameters can and should be changed for new versions.
# 

Set-Variable -Name "CurrentDir" -Value (Get-Location) # cd to the base odamex git path before executing

if ($env:new_version.length -gt 0)
{
    Set-Variable -Name "OdamexVersion" -Value "${env:new_version}"
}
else
{
    Set-Variable -Name "OdamexVersion" -Value "10.2.0"
}

if ($env:build_number.length -gt 0)
{
    Set-Variable -Name "OdamexTestSuffix" -Value "-build_${env:build_number}" # "-build_112"
}
else
{
    Set-Variable -Name "OdamexTestSuffix" -Value ""
}

#
# The actual script follows.
#

Set-Variable -Name "CommonDir" -Value "${CurrentDir}\OutCommon"
Set-Variable -Name "X64Dir" -Value "${CurrentDir}\OutX64"
Set-Variable -Name "OutputDir" -Value "${CurrentDir}\Output"
Set-Variable -Name "PdbDir" -Value "${OutputDir}\pdb"
Set-Variable -Name "ZipDir" -Value "${OutputDir}\zip"

function BuildX64 {
    if (Test-Path "${CurrentDir}\BuildX64")
    {
        Remove-Item -Recurse -Path "${CurrentDir}\BuildX64"
    }
    New-Item  -Force -ItemType "directory" -Path "${CurrentDir}\BuildX64"
    Set-Location -Path "${CurrentDir}\BuildX64"
    
    cmake.exe -G "Visual Studio 17 2022" -A "x64" "${CurrentDir}" `
        -DBUILD_OR_FAIL=1 `
        -DBUILD_CLIENT=1 -DBUILD_SERVER=1 `
        -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1
    cmake.exe --build . --config RelWithDebInfo

    Set-Location -Path "${CurrentDir}"
}

function CopyFilesX64 {
    if (Test-Path "${CommonDir}")
    {
        Remove-Item -Force -Recurse -Path "${CommonDir}"
    }

    if (Test-Path "${X64Dir}")
    {
        Remove-Item -Force -Recurse -Path "${X64Dir}"
    }

    New-Item -Force -ItemType "directory" -Path "${CommonDir}"
    New-Item -Force -ItemType "directory" -Path "${CommonDir}\config-samples"
    New-Item -Force -ItemType "directory" -Path "${CommonDir}\licenses"

    Copy-Item -Force -Path "${CurrentDir}\3RD-PARTY-LICENSES" `
        -Destination "${CommonDir}\3RD-PARTY-LICENSES.txt"
    Copy-Item -Force -Path "${CurrentDir}\CHANGELOG" `
        -Destination "${CommonDir}\CHANGELOG.txt"
    Copy-Item -Force -Path "${CurrentDir}\odamex-installed.txt" `
        -Destination "${CommonDir}"
    Copy-Item -Force -Path "${CurrentDir}\config-samples\*" `
        -Destination "${CommonDir}\config-samples"
    Copy-Item -Force -Path "${CurrentDir}\libraries\curl\COPYING" `
        -Destination "${CommonDir}\licenses\COPYING.curl.txt"
    Copy-Item -Force -Path "${CurrentDir}\libraries\miniupnp\LICENSE" `
        -Destination "${CommonDir}\licenses\LICENSE.miniupnp.txt"
    Copy-Item -Force -Path "${CurrentDir}\libraries\libpng\LICENSE" `
        -Destination "${CommonDir}\licenses\LICENSE.libpng.txt"
    Copy-Item -Force -Path "${CurrentDir}\libraries\portmidi\license.txt" `
        -Destination "${CommonDir}\licenses\license.portmidi.txt"
    Copy-Item -Force -Path "${CurrentDir}\LICENSE" `
        -Destination "${CommonDir}\LICENSE.txt"
    Copy-Item -Force -Path "${CurrentDir}\MAINTAINERS" `
        -Destination "${CommonDir}\MAINTAINERS.txt"
    Copy-Item -Force -Path "${CurrentDir}\README" `
        -Destination "${CommonDir}\README.txt"
    Copy-Item -Force -Path "${CurrentDir}\BuildX64\wad\odamex.wad" `
        -Destination "${CommonDir}"
    Copy-Item -Force -Path "${CurrentDir}\BuildX64\libraries\SDL2_mixer-2.0.4\COPYING.txt" `
        -Destination "${CommonDir}\licenses\COPYING.SDL2_mixer.txt"
    Copy-Item -Force -Path "${CurrentDir}\BuildX64\libraries\SDL2_mixer-2.0.4\lib\x64\LICENSE.FLAC.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX64\libraries\SDL2_mixer-2.0.4\lib\x64\LICENSE.modplug.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX64\libraries\SDL2_mixer-2.0.4\lib\x64\LICENSE.mpg123.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX64\libraries\SDL2_mixer-2.0.4\lib\x64\LICENSE.ogg-vorbis.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX64\libraries\SDL2_mixer-2.0.4\lib\x64\LICENSE.opus.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX64\libraries\SDL2_mixer-2.0.4\lib\x64\LICENSE.opusfile.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX64\libraries\SDL2-2.0.20\COPYING.txt" `
        -Destination "${CommonDir}\licenses\COPYING.SDL2.txt"

    ########################################
    ## 64-BIT FILES
    ########################################

    New-Item -Force -ItemType "directory" -Path "${X64Dir}"
    New-Item -Force -ItemType "directory" -Path "${X64Dir}\redist"

    Copy-Item -Force -Path `
        "${CurrentDir}\BuildX64\client\RelWithDebInfo\libFLAC-8.dll", `
        "${CurrentDir}\BuildX64\client\RelWithDebInfo\libmodplug-1.dll", `
        "${CurrentDir}\BuildX64\client\RelWithDebInfo\libmpg123-0.dll", `
        "${CurrentDir}\BuildX64\client\RelWithDebInfo\libogg-0.dll", `
        "${CurrentDir}\BuildX64\client\RelWithDebInfo\libopus-0.dll", `
        "${CurrentDir}\BuildX64\client\RelWithDebInfo\libvorbis-0.dll", `
        "${CurrentDir}\BuildX64\client\RelWithDebInfo\libvorbisfile-3.dll", `
        "${CurrentDir}\BuildX64\client\RelWithDebInfo\odamex.exe", `
        "${CurrentDir}\BuildX64\client\RelWithDebInfo\SDL2_mixer.dll", `
        "${CurrentDir}\BuildX64\client\RelWithDebInfo\SDL2.dll", `
        "${CurrentDir}\BuildX64\odalaunch\RelWithDebInfo\odalaunch.exe", `
        "${CurrentDir}\BuildX64\odalaunch\RelWithDebInfo\wxbase315u_net_vc14x_x64.dll", `
        "${CurrentDir}\BuildX64\odalaunch\RelWithDebInfo\wxbase315u_vc14x_x64.dll", `
        "${CurrentDir}\BuildX64\odalaunch\RelWithDebInfo\wxbase315u_xml_vc14x_x64.dll", `
        "${CurrentDir}\BuildX64\odalaunch\RelWithDebInfo\wxmsw315u_core_vc14x_x64.dll", `
        "${CurrentDir}\BuildX64\odalaunch\RelWithDebInfo\wxmsw315u_html_vc14x_x64.dll", `
        "${CurrentDir}\BuildX64\odalaunch\RelWithDebInfo\wxmsw315u_xrc_vc14x_x64.dll", `
        "${CurrentDir}\BuildX64\server\RelWithDebInfo\odasrv.exe" `
        -Destination "${X64Dir}\"

    # Get VC++ Redist
    Invoke-WebRequest -Uri "https://aka.ms/vs/17/release/vc_redist.x64.exe" -OutFile "${X64Dir}\redist\vc_redist.x64.exe"
}

function OutputsX64 {
    if (Test-Path "${OutputDir}")
    {
        Remove-Item -Force -Recurse -Path "${OutputDir}"
    }

    New-Item  -Force -ItemType "directory" -Path "${OutputDir}"
    New-Item  -Force -ItemType "directory" -Path "${ZipDir}"

    # Generate archives
    7z.exe a `
        "${ZipDir}\odamex-win64-${OdamexVersion}${OdamexTestSuffix}.zip" `
        "${CommonDir}\*" "${X64Dir}\*" `
        "-x!${CommonDir}\odamex-installed.txt"
}

function ZipDebugX64 {
    if (Test-Path "${PdbDir}")
    {
        Remove-Item -Force -Recurse -Path "${PdbDir}"
    }

    New-Item  -Force -ItemType "directory" -Path "${PdbDir}"

    # Copy pdb files into zip.  DO NOT THROW THESE AWAY!
    Copy-Item -Force -Path `
        "${CurrentDir}\BuildX64\client\RelWithDebInfo\odamex.pdb" `
        -Destination "${OutputDir}\odamex-x64-${OdamexVersion}.pdb"
    Copy-Item -Force -Path `
        "${CurrentDir}\BuildX64\server\RelWithDebInfo\odasrv.pdb" `
        -Destination "${OutputDir}\odasrv-x64-${OdamexVersion}.pdb"
    Copy-Item -Force -Path `
        "${CurrentDir}\BuildX64\odalaunch\RelWithDebInfo\odalaunch.pdb" `
        -Destination "${OutputDir}\odalaunch-x64-${OdamexVersion}.pdb"

    7z.exe a `
        "${PdbDir}\odamex-debug-pdb-${OdamexVersion}-x64.zip" `
        "${OutputDir}\*.pdb"

    Remove-Item -Force -Path "${OutputDir}\*.pdb"
}

# Ensure we have the proper executables in the PATH
echo "Checking for CMake..."
Get-Command cmake.exe -ErrorAction Stop

echo "Checking for 7zip..."
Get-Command 7z.exe -ErrorAction Stop

echo "Building 64-bit..."
BuildX64

echo "Copying files..."
CopyFilesX64

echo "Generating output..."
OutputsX64

echo "Copying PDB's into ZIP..."
ZipDebugX64
