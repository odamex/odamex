![Odamex](https://github.com/odamex/odamex/blob/development/media/logo_128.png?raw=true)

Informations required for Odamex on Nintendo Switch.

## Installation
- Create a folder named **odx_data** to the *root* of the SD Card, and put your IWADs in that specific folder.
- Add the **odamex** folder containing boot.dol, meta.xml and icon.png to your *sd:/apps* folder
- Run the game in the Homebrew Channel.

#### Known Bugs :
- Loading times makes the game slightly faster.

## Building from scratch

### 1) Getting the correct libraries
You need the latest DevKitPro (build was tested as of November 7th 2020) in order to make it work.

- Step 1 : Update everything : 
```sh
pacman -Syu 
```

- Step 2 : Get all the required libraries :
```sh
pacman -Syu switch-sdl2 switch-curl switch-libvorbis switch-opusfile switch-zlib switch-pkg-config switch-libvorbisidec switch-libogg switch-libopus switch-libpng
```

- Step 3 : We will install a modified SDL2-Mixer that will support Timidity:

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

Then replace all of the libSDL2_mixer.a/.la to $DEVKITPRO

- Step 4 :

go to your odamex folder. 

```sh
mkdir odamex_switch && cd odamex_switch

source $DEVKITPRO/switchvars.sh

cmake -DSWITCH_LIBNX=1 -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/switch.cmake" ../odamex

make
```


