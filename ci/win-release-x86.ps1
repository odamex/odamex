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
Set-Variable -Name "X86Dir" -Value "${CurrentDir}\OutX86"
Set-Variable -Name "OutputDir" -Value "${CurrentDir}\Output"
Set-Variable -Name "PdbDir" -Value "${OutputDir}\pdb"
Set-Variable -Name "ZipDir" -Value "${OutputDir}\zip"

function BuildX86 {
    if (Test-Path "${CurrentDir}\BuildX86")
    {
        Remove-Item -Recurse -Path "${CurrentDir}\BuildX86"
    }
    New-Item  -Force -ItemType "directory" -Path "${CurrentDir}\BuildX86"
    Set-Location -Path "${CurrentDir}\BuildX86"
    
    cmake.exe -G "Visual Studio 17 2022" -A "Win32" "${CurrentDir}" `
        -DBUILD_OR_FAIL=1 `
        -DBUILD_CLIENT=1 -DBUILD_SERVER=1 `
        -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1
    cmake.exe --build . --config RelWithDebInfo

    Set-Location -Path "${CurrentDir}"
}

function CopyFilesX86 {
    if (Test-Path "${CommonDir}")
    {
        Remove-Item -Force -Recurse -Path "${CommonDir}"
    }

    if (Test-Path "${X86Dir}")
    {
        Remove-Item -Force -Recurse -Path "${X86Dir}"
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
    Copy-Item -Force -Path "${CurrentDir}\BuildX86\wad\odamex.wad" `
        -Destination "${CommonDir}"
    Copy-Item -Force -Path "${CurrentDir}\BuildX86\libraries\SDL2_mixer-2.0.4\COPYING.txt" `
        -Destination "${CommonDir}\licenses\COPYING.SDL2_mixer.txt"
    Copy-Item -Force -Path "${CurrentDir}\BuildX86\libraries\SDL2_mixer-2.0.4\lib\x86\LICENSE.FLAC.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX86\libraries\SDL2_mixer-2.0.4\lib\x86\LICENSE.modplug.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX86\libraries\SDL2_mixer-2.0.4\lib\x86\LICENSE.mpg123.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX86\libraries\SDL2_mixer-2.0.4\lib\x86\LICENSE.ogg-vorbis.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX86\libraries\SDL2_mixer-2.0.4\lib\x86\LICENSE.opus.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX86\libraries\SDL2_mixer-2.0.4\lib\x86\LICENSE.opusfile.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${CurrentDir}\BuildX86\libraries\SDL2-2.0.20\COPYING.txt" `
        -Destination "${CommonDir}\licenses\COPYING.SDL2.txt"

    ########################################
    ## 32-BIT FILES
    ########################################

    New-Item -Force -ItemType "directory" -Path "${X86Dir}"
    New-Item -Force -ItemType "directory" -Path "${X86Dir}\redist"

    Copy-Item -Force -Path `
        "${CurrentDir}\BuildX86\client\RelWithDebInfo\libFLAC-8.dll", `
        "${CurrentDir}\BuildX86\client\RelWithDebInfo\libmodplug-1.dll", `
        "${CurrentDir}\BuildX86\client\RelWithDebInfo\libmpg123-0.dll", `
        "${CurrentDir}\BuildX86\client\RelWithDebInfo\libogg-0.dll", `
        "${CurrentDir}\BuildX86\client\RelWithDebInfo\libopus-0.dll", `
        "${CurrentDir}\BuildX86\client\RelWithDebInfo\libvorbis-0.dll", `
        "${CurrentDir}\BuildX86\client\RelWithDebInfo\libvorbisfile-3.dll", `
        "${CurrentDir}\BuildX86\client\RelWithDebInfo\odamex.exe", `
        "${CurrentDir}\BuildX86\client\RelWithDebInfo\SDL2_mixer.dll", `
        "${CurrentDir}\BuildX86\client\RelWithDebInfo\SDL2.dll", `
        "${CurrentDir}\BuildX86\odalaunch\RelWithDebInfo\odalaunch.exe", `
        "${CurrentDir}\BuildX86\odalaunch\RelWithDebInfo\wxbase315u_net_vc14x.dll", `
        "${CurrentDir}\BuildX86\odalaunch\RelWithDebInfo\wxbase315u_vc14x.dll", `
        "${CurrentDir}\BuildX86\odalaunch\RelWithDebInfo\wxbase315u_xml_vc14x.dll", `
        "${CurrentDir}\BuildX86\odalaunch\RelWithDebInfo\wxmsw315u_core_vc14x.dll", `
        "${CurrentDir}\BuildX86\odalaunch\RelWithDebInfo\wxmsw315u_html_vc14x.dll", `
        "${CurrentDir}\BuildX86\odalaunch\RelWithDebInfo\wxmsw315u_xrc_vc14x.dll", `
        "${CurrentDir}\BuildX86\server\RelWithDebInfo\odasrv.exe" `
        -Destination "${X86Dir}\"

    # Get VC++ Redist
    Invoke-WebRequest -Uri "https://aka.ms/vs/17/release/vc_redist.x86.exe" -OutFile "${X86Dir}\redist\vc_redist.x86.exe"
}

function OutputsX86 {
    if (Test-Path "${OutputDir}")
    {
        Remove-Item -Force -Recurse -Path "${OutputDir}"
    }
    New-Item  -Force -ItemType "directory" -Path "${OutputDir}"
    New-Item  -Force -ItemType "directory" -Path "${ZipDir}"

    # Generate archives
    7z.exe a `
        "${ZipDir}\odamex-win32-${OdamexVersion}${OdamexTestSuffix}.zip" `
        "${CommonDir}\*" "${X86Dir}\*" `
        "-x!${CommonDir}\odamex-installed.txt"
}

function ZipDebugX86 {
    if (Test-Path "${PdbDir}")
    {
        Remove-Item -Force -Recurse -Path "${PdbDir}"
    }

    New-Item  -Force -ItemType "directory" -Path "${PdbDir}"
    # Copy pdb files into zip.  DO NOT THROW THESE AWAY!
    Copy-Item -Force -Path `
        "${CurrentDir}\BuildX86\client\RelWithDebInfo\odamex.pdb" `
        -Destination "${OutputDir}\odamex-x86-${OdamexVersion}.pdb"
    Copy-Item -Force -Path `
        "${CurrentDir}\BuildX86\server\RelWithDebInfo\odasrv.pdb" `
        -Destination "${OutputDir}\odasrv-x86-${OdamexVersion}.pdb"
    Copy-Item -Force -Path `
        "${CurrentDir}\BuildX86\odalaunch\RelWithDebInfo\odalaunch.pdb" `
        -Destination "${OutputDir}\odalaunch-x86-${OdamexVersion}.pdb"

    7z.exe a `
        "${PdbDir}\odamex-debug-pdb-${OdamexVersion}-x86.zip" `
        "${OutputDir}\*.pdb"

    Remove-Item -Force -Path "${OutputDir}\*.pdb"
}

# Ensure we have the proper executables in the PATH
echo "Checking for CMake..."
Get-Command cmake.exe -ErrorAction Stop

echo "Checking for 7zip..."
Get-Command 7z.exe -ErrorAction Stop

echo "Building 32-bit..."
BuildX86

echo "Copying files..."
CopyFilesX86

echo "Generating output..."
OutputsX86

echo "Copying PDB's into ZIP..."
ZipDebugX86
