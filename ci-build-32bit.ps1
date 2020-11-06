Set-PSDebug -Trace 1

mkdir build | Out-Null
Set-Location "build-x32"

& cmake .. `
    -G "Visual Studio 16 2019" -A Win32 -DBUILD_ODALAUNCH=0 `

Set-Location ..
