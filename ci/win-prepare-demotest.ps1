Set-PSDebug -Trace 1

Set-Location "build"
New-Item -Name "demotest" -ItemType "directory" | Out-Null

# Copy all built files into artifact directory
Copy-Item -Path `
    ".\odamex\RelWithDebInfo\odamex.exe", `
    ".\odamex\RelWithDebInfo\odamex.pdb", `
    ".\odamex\RelWithDebInfo\*.dll", `
    ".\odamex\RelWithDebInfo\odasrv.exe", `
    ".\odamex\RelWithDebInfo\odasrv.pdb", `
    ".\odamex\RelWithDebInfo\odalaunch.exe", `
    ".\odamex\RelWithDebInfo\odalaunch.pdb", `
    ".\odamex\RelWithDebInfo\*.dll", `
    ".\odamex\RelWithDebInfo\odamex.wad", `
    "C:\Windows\System32\msvcp140.dll", `
    "C:\Windows\System32\vcruntime140.dll", `
    "C:\Windows\System32\vcruntime140_1.dll" `
    -Destination "demotest"

Set-Location ..
