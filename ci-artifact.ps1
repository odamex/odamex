Set-PSDebug -Trace 1

Set-Location build
mkdir artifact | Out-Null

# Copy all built files into artifact directory
Copy-Item `
    -Path ".\client\RelWithDebInfo\odamex.exe", ".\client\RelWithDebInfo\odamex.pdb", `
    ".\server\RelWithDebInfo\odasrv.exe", ".\server\RelWithDebInfo\odasrv.pdb", `
    "..\wad\odamex.wad", `
    ".\libraries\SDL2-2.0.12\lib\x64\*.dll", ".\libraries\SDL2_mixer-2.0.4\lib\x64\*.dll", `
    "C:\Windows\System32\msvcp140.dll", "C:\Windows\System32\vcruntime140.dll", `
    "C:\Windows\System32\vcruntime140_1.dll" `
    -Destination "artifact"

Set-Location ..
