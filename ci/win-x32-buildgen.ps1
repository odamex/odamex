Set-PSDebug -Trace 1

& .\vcpkg\bootstrap-vcpkg.bat

& cmake --preset "github-win-x32"
