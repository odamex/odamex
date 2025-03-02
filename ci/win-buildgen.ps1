Set-PSDebug -Trace 1

& .\vcpkg\bootstrap-vcpkg.bat

mkdir build | Out-Null
Set-Location build

& cmake .. -G "Visual Studio 17 2022" `
    -DBUILD_OR_FAIL=1 `
    -DBUILD_CLIENT=1 -DBUILD_SERVER=1 `
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1 `
    -DCMAKE_TOOLCHAIN_FILE: "../vcpkg/scripts/buildsystems/vcpkg.cmake"

Set-Location ..
