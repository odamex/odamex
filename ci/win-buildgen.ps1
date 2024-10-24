Set-PSDebug -Trace 1

mkdir build | Out-Null
Set-Location build

& cmake .. -G "Visual Studio 17 2022" `
    -DBUILD_OR_FAIL=1 `
    -DBUILD_CLIENT=1 -DBUILD_SERVER=1 `
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1 `
    -DBUILD_SHIM=1

Set-Location ..
