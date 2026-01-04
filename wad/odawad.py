#
# Copyright (C) 2006-2025 by The Odamex Team.
#

import configparser
from pathlib import Path

from dotex import DoomPalette, DoomPatch, DoomWADWriter


def read_patch(dir: str, lump: str) -> DoomPatch:
    files = Path(dir).glob(lump + ".*")

    # This is a loop, but we always want the first matched file.
    for file in files:
        graphic = DoomPatch()
        graphic.read_image(file)
        return graphic

    raise Exception(f"'{lump}' not found in '{dir}'")


config = configparser.ConfigParser(allow_no_value=True)
config.read("wadinfo.txt")

palette = DoomPalette()
palette.read_gimp_palette(Path(__file__).parent / "doom.gpl")

odamex_wad = DoomWADWriter(Path("odamex.wad"))


def pack_graphics():
    for graphics in config["graphics"]:
        filename, leftstr, topstr = graphics.split()

        graphic = read_patch("graphics", filename)
        graphic.set_left_top(int(leftstr), int(topstr))
        data = graphic.to_bytes(palette)
        odamex_wad.write_lump(filename.upper(), data)


def pack_sprites():
    odamex_wad.write_lump("SS_START")

    for sprites in config["sprites"]:
        filename, leftstr, topstr = sprites.split()

        sprite = read_patch("sprites", filename)
        sprite.set_left_top(int(leftstr), int(topstr))
        data = sprite.to_bytes(palette)
        odamex_wad.write_lump(filename.upper(), data)

    odamex_wad.write_lump("SS_END")


pack_graphics()
pack_sprites()
odamex_wad.finalize()
