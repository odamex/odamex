![Odamex](https://github.com/odamex/odamex/blob/development/media/logo_128.png?raw=true)

Informations required for Odamex on Nintendo Switch.

# IMPORTANT INFO
- You **NEED** to start the Homebrew with full applet support (= Start the homebrew launcher through an existing application, and NOT the gallery)!

## Installation
- Create a folder named **odamex** to the */switch* folder of your microSD Card, and navigate to that folder.
- Put *odamex.nsp* and your *IWADs/PWADs* (DOOM.WAD, DOOM2.WAD, FREEDOOM2.WAD, ...) to that folder.

#### Known Bugs :
- Loading times makes the game go faster, to keep with uncapped framerates. It could lead to desyncs online until it syncs back after a few seconds. The same behaviour can happen if you go to the home menu, or put the console into sleep mode.
- When starting the game, the keyboard will suddently appear. This is because of the way we currently handle the SDL2 keyboard.
- Custom wad folders aren't supported right now.
- Partial invisibility is blending in red, not black.
- Quitting the game may give an error message that could make you unable to start any application, unless restarting the console.

## Building from scratch

**This has been built using devkitpro on Windows (using msys2). Linux/MacOSX users may slightly modify the commands if needed.**

Latest compilation has been tested as of December 30th 2020.

### Preparing the environment 
Make sure you have the latest DevKitPro/msys2 packages:

```sh
pacman -Syu 
```

### Getting all the required libraries :

```sh
pacman -Sy cmake switch-sdl2 switch-sdl2_mixer switch-curl switch-libvorbis switch-opusfile switch-zlib switch-pkg-config switch-libvorbisidec switch-libogg switch-libopus switch-libpng devkitpro-pkgbuild-helpers
```

### (optional) - We will build an older SDL2-Mixer version that will support Timidity:

```sh

git clone https://github.com/fgsfdsfgs/SDL_mixer && cd SDL_Mixer

source $DEVKITPRO/switchvars.sh

sed 's|\$(objects)/play.*mus\$(EXE)||' -i Makefile.in

  LIBS="-lm" ./configure --prefix="${PORTLIBS_PREFIX}" \
    --host=aarch64-none-elf --disable-shared --enable-static \
    --disable-music-cmd \
    --enable-music-ogg-tremor \
	--enable-music-mod-modplug \
	--enable-music-midi \
	--enable-music-midi-timidity
	
make 
make install
```

Then replace all of the libSDL2_mixer.a/.la libraries to $DEVKITPRO

### Making the required folders & compiling :

go to your odamex folder. 

```sh
mkdir odamex_switch && cd odamex_switch

source $DEVKITPRO/switchvars.sh

cmake -Wno-dev -DSWITCH_LIBNX=1 -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/switch.cmake" ../odamex

make
```


