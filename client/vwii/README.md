COMPILING ODAMEX ON WII

- Requirements :
- The latest release of DevKitPRO with the wii-development ticked.

0) Always important after installing DEVKITPRO: make sure your building environment is always up to date.
> pacman -SYu

If it asks you to restart, do it so.

1) Install the portlibs packages for the wii
> pacman -Sy ppc-portlibs

2a) in a separate folder, clone the SDL-Wii github repository 
> Git clone https://github.com/dborth/sdl-wii 

2b) If the patch hasn't been applied yet, apply this patch : 
https://github.com/dborth/sdl-wii/pull/55/commits/48cd54546d2dad8ec9fa6fb4ec74efb621c03a88

2c) make, and install them.
> make && make install

3) Compile odamex for Wii.
> cd (your ODAMEX folder)/Wii && make