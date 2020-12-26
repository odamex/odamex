Set-PSDebug -Trace 1

mkdir build-gcc | Out-Null
Set-Location build-gcc

& cmake .. -G "Ninja" `
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_OR_FAIL=1 `
    -DBUILD_CLIENT=1 -DBUILD_SERVER=1 `
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1

Set-Location ..
