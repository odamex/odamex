Set-PSDebug -Trace 1

# Construct a filename

$SHORTHASH = "${Env:GITHUB_SHA}".substring(0, 7)
$SHORTREF = "${Env:GITHUB_REF}".replace("refs/heads/", "").replace("/", "-")

$OUTFILENAME = "Odamex-x64-$SHORTREF-$Env:GITHUB_RUN_NUMBER-$SHORTHASH"

Set-Location "build"
New-Item -Name "artifact" -ItemType "directory" | Out-Null
New-Item -Name "archive" -ItemType "directory" | Out-Null

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
    ".\shim\RelWithDebInfo\odashim.exe", `
    ".\shim\discord_game_sdk.dll", `
    ".\wad\odamex.wad", `
    "C:\Windows\System32\msvcp140.dll", `
    "C:\Windows\System32\vcruntime140.dll", `
    "C:\Windows\System32\vcruntime140_1.dll" `
    -Destination "artifact"

# Archive files into a zip

Set-Location artifact

7z.exe a -tzip "../archive/$OUTFILENAME.zip" "*"

Set-Location ..

Set-Location ..
