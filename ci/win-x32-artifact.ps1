Set-PSDebug -Trace 1

. "$PSScriptRoot\cvardoc.ps1"

# Construct a filename

$SHORTHASH = "${Env:GITHUB_SHA}".substring(0, 7)
$SHORTREF = "${Env:GITHUB_REF}".replace("refs/heads/", "").replace("/", "-")

$OUTFILENAME = "Odamex-x32-$SHORTREF-$Env:GITHUB_RUN_NUMBER-$SHORTHASH"

Set-Location "build-x32"
mkdir artifact | Out-Null

Write-CvarDocJson ".\client\RelWithDebInfo\odamex.exe" | Out-Null
Write-CvarDocJson ".\server\RelWithDebInfo\odasrv.exe" | Out-Null

# Copy all built files into artifact directory
Copy-Item `
    -Path ".\client\RelWithDebInfo\odamex.exe", ".\client\RelWithDebInfo\odamex.pdb", `
    ".\client\RelWithDebInfo\odamex_cvardoc.json", `
    ".\server\RelWithDebInfo\odasrv.exe", ".\server\RelWithDebInfo\odasrv.pdb", `
    ".\server\RelWithDebInfo\odasrv_cvardoc.json", `
    ".\wad\odamex.wad", `
    ".\libraries\SDL2-2.32.8\lib\x86\*.dll", ".\libraries\SDL2_mixer-2.8.1\lib\x86\*.dll", ".\libraries\SDL2_mixer-2.8.1\lib\x86\optional\*.dll", `
    "C:\Windows\System32\msvcp140.dll", "C:\Windows\System32\vcruntime140.dll", `
    "C:\Windows\System32\vcruntime140_1.dll" `
    -Destination "artifact"

# Archive files into a zip

Set-Location artifact

7z.exe a -tzip "../archive/$OUTFILENAME.zip" "*"

Set-Location ..

Set-Location ..
