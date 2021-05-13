![Odamex](https://raw.githubusercontent.com/odamex/odamex/stable/media/logo_128.png?raw=true)

Informations required for Odamex on Nintendo Wii.

## Installation
- Create a folder named **odx_data** to the *root* of the SD Card, and put your IWADs in that specific folder.
- Add the **odamex** folder containing boot.dol, meta.xml and icon.png to your *sd:/apps* folder
- Run the game in the Homebrew Channel.

#### Known Bugs :
- Controllers cannot interact in the menu yet, so you may need to navigate using an USB Keyboard.
- Due to a recent update, you cannot write anything in the console.
- Some wads (such as *DUEL2020OA.wad*) may crash on specific maps.
- Switches disappear once pressed due to recent updates, resulting in a HOM.
- Wiimote pointing is not working (and may be removed in a future update).

## Building from scratch

### Requirements to compile Odamex-Wii:
- The latest release of **DevKitPro** with the *wii-development* toolchain ticked.
- *Cmake* 3.X.X
- [SDL-Wii](https://github.com/dborth/sdl-wii) by **dborth**

### Compilation instructions

Make sure your Devkitpro environment is always up to date before attempting to compile :
```bash
pacman -Syu
```

1) Install the portlibs package and CMake :
```bash
pacman -Sy cmake ppc-portlibs devkitpro-pkgbuild-helpers
```

2) In a separate folder, clone the SDL-Wii github repository :
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

5) Run these commands :
```bash
source $DEVKITPRO/ppcvars.sh
export PATH=/opt/devkitpro/devkitPPC/bin:/opt/devkitpro/tools/bin:$PATH

cmake -DVWII=1 -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/wii.cmake" ../odamex
```

4) Compile odamex for Wii.
```bash
make
```

