Set-Variable -Name "CurrentDir" -Value (Get-Location) # cd to the base odamex git path before executing
Set-Variable -Name "CommonDir" -Value "${CurrentDir}\OutCommon"
Set-Variable -Name "OutX86" -Value "${CurrentDir}\OutX86"
Set-Variable -Name "OutX64" -Value "${CurrentDir}\OutX64"
Set-Variable -Name "UnzippedX64" -Value "${CurrentDir}\Odamex-Win-x64"
Set-Variable -Name "UnzippedX86" -Value "${CurrentDir}\Odamex-Win-x86"
Set-Variable -Name "OutputDir" -Value "${CurrentDir}\Output"


if ($env:new_version.length -gt 0)
{
    Set-Variable -Name "OdamexVersion" -Value "${env:new_version}"
}
else
{
    Set-Variable -Name "OdamexVersion" -Value "10.6.0"
}

if ($env:build_number.length -gt 0)
{
    Set-Variable -Name "OdamexTestSuffix" -Value "-build_${env:build_number}" # "-build_112"
}
else
{
    Set-Variable -Name "OdamexTestSuffix" -Value ""
}

function UnzipArtifacts {
    7z.exe x odamex-win64-*.zip "-o${UnzippedX64}"
    7z.exe x odamex-win32-*.zip "-o${UnzippedX86}"
}

# Lay files out in a path that Inno Setup expects.
function BuildOutCommon {
    if (Test-Path "${CommonDir}")
    {
        Remove-Item -Recurse -Path "${CommonDir}"
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
    Copy-Item -Force -Path "${UnzippedX64}\odamex.wad" `
        -Destination "${CommonDir}"
    Copy-Item -Force -Path "${UnzippedX64}\licenses\COPYING.SDL2_mixer.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${UnzippedX64}\licenses\LICENSE.FLAC.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${UnzippedX64}\licenses\LICENSE.modplug.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${UnzippedX64}\licenses\LICENSE.mpg123.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${UnzippedX64}\licenses\LICENSE.ogg-vorbis.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${UnzippedX64}\licenses\LICENSE.opus.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${UnzippedX64}\licenses\LICENSE.opusfile.txt" `
        -Destination "${CommonDir}\licenses"
    Copy-Item -Force -Path "${UnzippedX64}\licenses\COPYING.SDL2.txt" `
        -Destination "${CommonDir}\licenses"
}

function BuildOutX86 {
    if (Test-Path "${OutX86}")
    {
        Remove-Item -Recurse -Path "${OutX86}"
    }

    New-Item -Force -ItemType "directory" -Path "${OutX86}"
    New-Item -Force -ItemType "directory" -Path "${OutX86}\redist"

    Copy-Item -Force -Path `
        "${UnzippedX86}\libFLAC-8.dll", `
        "${UnzippedX86}\libmodplug-1.dll", `
        "${UnzippedX86}\libmpg123-0.dll", `
        "${UnzippedX86}\libogg-0.dll", `
        "${UnzippedX86}\libopus-0.dll", `
        "${UnzippedX86}\libvorbis-0.dll", `
        "${UnzippedX86}\libvorbisfile-3.dll", `
        "${UnzippedX86}\odamex.exe", `
        "${UnzippedX86}\SDL2_mixer.dll", `
        "${UnzippedX86}\SDL2.dll", `
        "${UnzippedX86}\odalaunch.exe", `
        "${UnzippedX86}\wxbase315u_net_vc14x.dll", `
        "${UnzippedX86}\wxbase315u_vc14x.dll", `
        "${UnzippedX86}\wxbase315u_xml_vc14x.dll", `
        "${UnzippedX86}\wxmsw315u_core_vc14x.dll", `
        "${UnzippedX86}\wxmsw315u_html_vc14x.dll", `
        "${UnzippedX86}\wxmsw315u_xrc_vc14x.dll", `
        "${UnzippedX86}\odasrv.exe" `
        -Destination "${OutX86}\"

    Copy-Item -Force -Path `
    "${UnzippedX86}\redist\vc_redist.x86.exe" `
    -Destination "${OutX86}\redist"
}

function BuildOutX64 {
    if (Test-Path "${OutX64}")
    {
        Remove-Item -Recurse -Path "${OutX64}"
    }

    New-Item -Force -ItemType "directory" -Path "${OutX64}"
    New-Item -Force -ItemType "directory" -Path "${OutX64}\redist"

    Copy-Item -Force -Path `
        "${UnzippedX64}\libFLAC-8.dll", `
        "${UnzippedX64}\libmodplug-1.dll", `
        "${UnzippedX64}\libmpg123-0.dll", `
        "${UnzippedX64}\libogg-0.dll", `
        "${UnzippedX64}\libopus-0.dll", `
        "${UnzippedX64}\libvorbis-0.dll", `
        "${UnzippedX64}\libvorbisfile-3.dll", `
        "${UnzippedX64}\odamex.exe", `
        "${UnzippedX64}\SDL2_mixer.dll", `
        "${UnzippedX64}\SDL2.dll", `
        "${UnzippedX64}\odalaunch.exe", `
        "${UnzippedX64}\wxbase315u_net_vc14x_x64.dll", `
        "${UnzippedX64}\wxbase315u_vc14x_x64.dll", `
        "${UnzippedX64}\wxbase315u_xml_vc14x_x64.dll", `
        "${UnzippedX64}\wxmsw315u_core_vc14x_x64.dll", `
        "${UnzippedX64}\wxmsw315u_html_vc14x_x64.dll", `
        "${UnzippedX64}\wxmsw315u_xrc_vc14x_x64.dll", `
        "${UnzippedX64}\odasrv.exe" `
        -Destination "${OutX64}\"

    Copy-Item -Force -Path `
    "${UnzippedX64}\redist\vc_redist.x64.exe" `
    -Destination "${OutX64}\redist"
}

function CompileSetup {
    if (Test-Path "${OutputDir}")
    {
        Remove-Item -Force -Recurse -Path "${OutputDir}"
    }

    New-Item  -Force -ItemType "directory" -Path "${OutputDir}"
    # Generate installer
    ISCC.exe "${CurrentDir}\installer\windows\odamex.iss" `
        /DOdamexVersion=${OdamexVersion} `
        /DOdamexTestSuffix=${OdamexTestSuffix} `
        /DSourcePath=${CurrentDir} `
        /O${OutputDir}
}

echo "Checking for 7zip..."
Get-Command 7z.exe -ErrorAction Stop

echo "Checking for Inno Setup Command-Line Compiler..."
Get-Command ISCC.exe -ErrorAction Stop

echo "Unzipping artifacts"
UnzipArtifacts

echo "Reconstructing common directory"
BuildOutCommon

echo "Reconstructing x64 directory"
BuildOutX64

echo "Reconstructing x86 directory"
BuildOutX86

echo "Compiling setup file"
CompileSetup
