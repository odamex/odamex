Set-PSDebug -Trace 1

mkdir "build-x32" | Out-Null
Set-Location "build-x32"

& cmake .. `
    -G "Visual Studio 16 2022" -A Win32 -DBUILD_ODALAUNCH=0 `

Set-Location ..
