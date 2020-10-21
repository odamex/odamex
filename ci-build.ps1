Set-PSDebug -Trace 1

mkdir build | Out-Null
Set-Location build

& cmake .. -G "Visual Studio 16 2019"

Set-Location ..
