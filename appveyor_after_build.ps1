cd build
7z a odamex.zip `
    ".\client\RelWithDebInfo\odamex.exe" ".\client\RelWithDebInfo\odamex.pdb" "..\wad\odamex.wad" `
    ".\SDL2-2.0.12\lib\x64\*.dll" ".\SDL2_mixer-2.0.4\lib\x64\*.dll" `
    "C:\Windows\System32\msvcp140.dll" "C:\Windows\System32\vcruntime140.dll" `
    "C:\Windows\System32\vcruntime140_1.dll"
mv odamex.zip ..
7z a odasrv.zip `
    ".\server\RelWithDebInfo\odasrv.exe" ".\server\RelWithDebInfo\odasrv.pdb" "..\wad\odamex.wad"
mv odasrv.zip ..
cd ..
