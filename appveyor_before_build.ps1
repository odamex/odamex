mkdir build | Out-Null
cd build

if (!(Test-Path -Path "SDL2-devel-2.0.5-VC.zip" -PathType leaf)) {
    Start-FileDownload "https://www.libsdl.org/release/SDL2-devel-2.0.5-VC.zip"
}
if (!(Test-Path -Path "SDL2-2.0.5")) {
    7z x "SDL2-devel-2.0.5-VC.zip"
}

if (!(Test-Path -Path "SDL2_mixer-devel-2.0.1-VC.zip" -PathType leaf)) {
    Start-FileDownload "https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.1-VC.zip"
}
if (!(Test-Path -Path "SDL2_mixer-2.0.1")) {
    7z x "SDL2_mixer-devel-2.0.1-VC.zip"
}

& "C:\Program Files (x86)\cmake\bin\cmake.exe" .. -G "Visual Studio 14 2015" -T v140_xp -DUSE_MINIUPNP=False -DSDL2_INCLUDE_DIR="./SDL2-2.0.5/include/" -DSDL2_LIBRARY="./SDL2-2.0.5/lib/x86/SDL2.lib" -DSDL2MAIN_LIBRARY="./lib/x86/SDL2main.lib" -DSDL2_MIXER_INCLUDE_DIR="./SDL2_mixer-2.0.1/include/" -DSDL2_MIXER_LIBRARY="./SDL2_mixer-2.0.1/lib/x86/SDL2_mixer.lib"
cd ..
