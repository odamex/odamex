cd build
7z a odamex.zip ".\client\RelWithDebInfo\odamex.exe" ".\client\RelWithDebInfo\odamex.pdb" "..\odamex.wad" ".\SDL2-2.0.5\lib\x86\*.dll" ".\SDL2_mixer-2.0.1\lib\x86\*.dll"
mv odamex.zip ..
7z a odasrv.zip ".\server\RelWithDebInfo\odasrv.exe" ".\server\RelWithDebInfo\odasrv.pdb" "..\odamex.wad"
mv odasrv.zip ..
cd ..