Set-PSDebug -Trace 1

mkdir build-gcc | Out-Null
Set-Location build-gcc

# [AM] Odalaunch doesn't compile correctly in Visual Studio, so we must use MinGW.

# Grab wxWidgets libraries - all of them.
if (!(Test-Path -Path "wxWidgets-3.1.3-headers.7z" -PathType leaf)) {
    curl.exe --fail --location --max-time 30 --remote-name --silent --show-error `
        "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.3/wxWidgets-3.1.3-headers.7z"
}
if (!(Test-Path -Path "wxMSW-3.1.3_gcc810_x64_Dev.7z" -PathType leaf)) {
    curl.exe --fail --location --max-time 30 --remote-name --silent --show-error `
        "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.3/wxMSW-3.1.3_gcc810_x64_Dev.7z"
}
# [AM] Not needed?
# if (!(Test-Path -Path "wxMSW-3.1.3_gcc810_x64_ReleaseDLL.7z" -PathType leaf)) {
#     curl.exe --fail --location --max-time 30 --remote-name --silent --show-error `
#         "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.3/wxMSW-3.1.3_gcc810_x64_ReleaseDLL.7z"
# }

if (!(Test-Path -Path "wxMSW")) {
    mkdir wxMSW | Out-Null
}

Set-Location wxMSW

if (!(Test-Path -Path "include")) {
    7z x "..\wxWidgets-3.1.3-headers.7z"
}
if (!(Test-Path -Path "build")) {
    7z x "..\wxMSW-3.1.3_gcc810_x64_Dev.7z"
}
# [AM] Not needed?
# if (!(Test-Path -Path "lib\gcc_dll\wxbase313u_gcc810_x64.dll" -PathType leaf)) {
#     7z x "..\wxMSW-3.1.3_gcc810_x64_ReleaseDLL.7z"
# }

# We need to do this or else CMake's wxWidgets finder can't find the library
# directory.
Move-Item "lib\gcc810_x64_dll" "lib\gcc_dll"

Set-Location ..

& cmake .. -G "Ninja" `
    -DwxWidgets_ROOT_DIR="./wxMSW" `
    -DwxWidgets_wxrc_EXECUTABLE="./wxMSW/lib/gcc_dll/wxrc.exe"

Set-Location ..
