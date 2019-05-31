## Compilation

### 1) Getting the correct libraries
You need the latest devkitpro (build was tested as of March 31st 2019) in order to make it work.

- Step 1 : Update everything : 
pacman -Syu 

- Step 2 : Get all the required libraries :
pacman -Syu switch-sdl2 switch-sdl2_mixer

- Step 3 : Recompile and enable Timidity Support from this fork
https://github.com/fgsfdsfgs/SDL_mixer

source $DEVKITPRO/switchvars.sh

sed 's|\$(objects)/play.*mus\$(EXE)||' -i Makefile.in

  LIBS="-lm" ./configure --prefix="${PORTLIBS_PREFIX}" \
    --host=aarch64-none-elf --disable-shared --enable-static \
    --disable-music-cmd \
    --enable-music-ogg-tremor \
	--enable-music-mod-modplug \
	--enable-music-midi-timidity
	
make 

Then replace all of the libSDL2_mixer.a/.la to $DEVKITPRO

- Step 4 :

go to your odamex folder. 

mkdir build && cd build

source $DEVKITPRO/switchvars.sh

then this command :

cmake \
-G"Unix Makefiles" \
-DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/switch.cmake" \
-DCMAKE_BUILD_TYPE=Release \
-DPKG_CONFIG_EXECUTABLE="$DEVKITPRO/portlibs/switch/bin/aarch64-none-elf-pkg-config" \
-DBUILD_CLIENT=ON \
-DBUILD_SERVER=OFF \
-DBUILD_MASTER=OFF \
-DBUILD_ODALAUNCH=OFF \
-DUSE_MINIUPNP=OFF \
-DENABLE_PORTMIDI=OFF \
..

make



