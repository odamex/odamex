function Build64 {
    Remove-Item -Recurse -Path "${PSScriptRoot}\Build64"
    New-Item  -Force -ItemType "directory" -Path "${PSScriptRoot}\Build64"
    Set-Location -Path "${PSScriptRoot}\Build64"
    
    cmake.exe -G "Visual Studio 16 2019" -A "x64" "${PSScriptRoot}\..\.." `
        -DBUILD_OR_FAIL=1 `
        -DBUILD_CLIENT=1 -DBUILD_SERVER=1 `
        -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1
    cmake.exe --build . --parallel

    Set-Location -Path "${PSScriptRoot}"
}

function Build32 {
    Remove-Item -Recurse -Path "${PSScriptRoot}\Build32"
    New-Item  -Force -ItemType "directory" -Path "${PSScriptRoot}\Build32"
    Set-Location -Path "${PSScriptRoot}\Build32"
    
    cmake.exe -G "Visual Studio 16 2019" -A "Win32" "${PSScriptRoot}\..\.." `
        -DBUILD_OR_FAIL=1 `
        -DBUILD_CLIENT=1 -DBUILD_SERVER=1 `
        -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1
    cmake.exe --build . --parallel

    Set-Location -Path "${PSScriptRoot}"
}

function Installer {
    ISCC.exe odamex.iss
}

Build64
Build32
# Installer
