#
# Copyright (C) 2006-2025 by The Odamex Team.
#

import configparser
from pathlib import Path

from dotex import DoomGraphic, DoomPalette


def read_graphic(dir: str, lump: str) -> DoomGraphic:
    files = Path(dir).glob(lump + ".*")

    # This is a loop, but we always want the first matched file.
    for file in files:
        graphic = DoomGraphic()
        graphic.read_image(file)
        return graphic

    raise Exception(f"'{lump}' not found in '{dir}'")


config = configparser.ConfigParser(allow_no_value=True)
config.read("wadinfo.txt")

palette = DoomPalette()
palette.read_gimp_palette(Path(__file__).parent / "doom.gpl")

for graphics in config["graphics"]:
    filename, leftstr, topstr = graphics.split()

    graphic = read_graphic("graphics", filename)
    graphic.set_left_top(int(leftstr), int(topstr))
    data = graphic.to_bytes(palette)

    with open(graphic.name + ".lmp", "wb") as wh:
        wh.write(data)
