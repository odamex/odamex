![Odamex](https://raw.githubusercontent.com/odamex/odamex/stable/media/logo_128.png?raw=true)

| Windows Build Status | Mac Build Status | Linux Build Status | Join our Discord! |
| -------------------- | ---------------- | ------------------ | ----------------- |
| [![Windows](https://github.com/odamex/odamex/workflows/Windows/badge.svg)](https://github.com/odamex/odamex/actions?query=workflow%3AWindows) | [![macOS](https://github.com/odamex/odamex/workflows/macOS/badge.svg)](https://github.com/odamex/odamex/actions?query=workflow%3AmacOS) | [![Linux](https://github.com/odamex/odamex/workflows/Linux/badge.svg)](https://github.com/odamex/odamex/actions?query=workflow%3ALinux) | [![Join our discord](https://discordapp.com/api/guilds/236518337671200768/widget.png?style=shield)](https://discord.gg/aMUzcZE) |

Odamex is a modification of DOOM to allow players to compete with each other over the Internet using a client/server architecture. Thanks to the source code release of DOOM by id Software in December 1997, there have been many modifications that enhanced DOOM in various ways. These modifications are known as "source ports", as early modifications mainly ported DOOM to other platforms and operating systems such as Windows and Macintosh.

Odamex is based on the CSDoom 0.62 source code originally created by Sergey Makovkin, which is based on the ZDoom 1.22 source code created by Marisa Heit.

Features
--------

Odamex supports the following features:

* Full Client/Server multiplayer architecture with network compensation features (unlagged, client interpolation and prediction)
* Support for up to 255 players
* Various compatibility settings, to emulate vanilla Doom, Boom, or ZDoom physics and fixes
* Playback and recording of vanilla demos
* A fully-featured client netdemo record system with playback control
* Removal of most vanilla Doom Static limits
* Support for most Boom and MBF mapping features
* The traditional old-school style of Deathmatch and a Cooperative mode, but also other game modes such as Team Deathmatch and Capture the Flag
* Support for Survival, Last Man Standing, Last Team Standing, LMS CTF, 3-WAY CTF, and Attack & Defend game modes
* Competitive-ready features, such as a warmup mode, round system, player queue, or playercolor overriding
* Several modern ZDoom additions, such as slopes, LANGUAGEv2 or MAPINFOv2 lump support
* An array of editing features, including the Hexen map format, DeHackEd and BEX patch support and ACS up to ZDoom 1.23
* Native Joystick support
* Several additional music formats, such as MOD and OGG
* In-Engine WAD downloader
* Allow on-the-fly WAD loading
* A 32-bit Color depth/true color renderer
* Full Widescreen support
* Ultra high resolution support (up to 8K)
* Uncapped and raised framerate
* Takes advantage of widely used libraries to port it to new devices with ease

Compilation instructions
------------------------

Clone the repositories, with all submodules:

    git clone https://github.com/odamex/odamex.git --recurse-submodules

If you downloaded the zip, use these commands on the git directory:

    git submodule init
    git submodule update

Odamex requires the following tools and libraries:

* [CMake](https://cmake.org/download/) 3.13 or later
* [SDL 2.0](https://www.libsdl.org/download-2.0.php)
* [SDL2-Mixer](https://libsdl.org/projects/SDL_mixer/)
* [cURL](https://curl.se/)
* [libPNG](http://www.libpng.org/pub/png/libpng.html)
* [zlib](https://zlib.net/)
* [DeuTex](https://github.com/Doom-Utils/deutex/releases/) (for building the WAD)
* [wxWidgets](https://www.wxwidgets.org/downloads/) (for the launcher)

`cURL`, `libPNG` and `zlib` are automatically included in-tree as submodules.

On Windows, all libraries are automatically downloaded if not found.  On \*nix/MacOSX, you will need to download through your package manager `libsdl2-dev`, `libsdl2-mixer-dev` and `libwxgtk3.0-dev`.

Please check [this page][1] for further instructions on how to compile Odamex for your platform.

[1]: https://odamex.net/wiki/How_to_build_from_source

Contributing to the project
---------------------------

Please report any oddity, physics inaccuracies, bugs or game-breaking glitches to our [issues page][2]. You can also submit patches as a Pull Request.

[2]: https://github.com/odamex/odamex/issues

Before submitting a pull request, please make sure it follows [our coding standards][3]!

[3]: https://odamex.net/wiki/Coding_standard

For historical purposes, you can also view [our bugtracker's archive][4].

[4]: https://odamex.net/bugs/

External Links
--------------

Please visit the following websites for more information about the development of the port and our community:

* [**Odamex Website**](https://odamex.net)
* [Wiki](https://odamex.net/wiki/Main_Page)
* [Forums](https://odamex.net/boards/)
* [Discord](https://discord.gg/aMUzcZE)
* [Twitter](https://twitter.com/odamex)

License
-------

Odamex is released under the GNU General Public License v2. Please read [`LICENSE`](LICENSE) for further details regarding the license.
