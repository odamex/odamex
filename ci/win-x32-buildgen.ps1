Set-PSDebug -Trace 1

mkdir "build-x32" | Out-Null
Set-Location "build-x32"

& cmake .. -G "Visual Studio 17 2022" `
    -A Win32 `
    -DBUILD_OR_FAIL=1 `
    -DBUILD_CLIENT=1 -DBUILD_SERVER=1 `
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1

Set-Location ..
