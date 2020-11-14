Set-PSDebug -Trace 1

Set-Location build
mkdir artifact | Out-Null

# Copy all built files into artifact directory
Copy-Item -Path `
    ".\client\RelWithDebInfo\odamex.exe", `
    ".\client\RelWithDebInfo\odamex.pdb", `
    ".\client\RelWithDebInfo\*.dll", `
    ".\server\RelWithDebInfo\odasrv.exe", `
    ".\server\RelWithDebInfo\odasrv.pdb", `
    ".\odalaunch\RelWithDebInfo\odalaunch.exe", `
    ".\odalaunch\RelWithDebInfo\odalaunch.pdb", `
    ".\odalaunch\RelWithDebInfo\*.dll", `
    "..\wad\odamex.wad", `
    "C:\Windows\System32\msvcp140.dll", `
    "C:\Windows\System32\vcruntime140.dll", `
    "C:\Windows\System32\vcruntime140_1.dll" `
    -Destination "artifact"

Set-Location ..
