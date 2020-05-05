![Odamex](https://github.com/odamex/odamex/blob/development/media/logo_128.png?raw=true)

Informations required for Odamex on Nintendo Wii.

## Known Bugs :
- Inputs in the menu are currently missing with a joystick.
- Due to a recent update, You cannot write anything in the console.
- Some wads (such as DUEL2020OA.wad) may crash on startup.
- Switches disappear once pressed due to recent updates.

## Requirements to compile Odamex-Wii:
- The latest release of **DevKitPro** with the *wii-development* option ticked.
- Cmake package
- [SDL-Wii](https://github.com/dborth/sdl-wii) by **dborth**


## Building from scratch
Make sure your Devkitpro environment is always up to date before attempting to compile :
```bash
pacman -SYu
```

1) Install the portlibs package and CMake :
```bash
pacman -Sy ppc-portlibs cmake
```

2) in a separate folder, clone the SDL-Wii github repository :
```bash
git clone https://github.com/dborth/sdl-wii
```

3) Go to the SDL folder and compile it :
```bash
cd sdl-wii && make && make install
```

4) Prepare your building environment :
```bash
mkdir odamex_wii && cd odamex_wii
```

5) Ran these commands :
```bash
source $DEVKITPRO/ppcvars.sh
export PATH=/opt/devkitpro/devkitPPC/bin:/opt/devkitpro/tools/bin:$PATH

cmake -DVWII=1 -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/wii.cmake" ../odamex
```

4) Compile odamex for Wii.
```bash
make
```

