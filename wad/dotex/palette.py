#
# dotex: "I don't care, just do the tex or whatever..."
#
# Written in 2025 by Lexi Mayfield
# Released into the Public Domain, see UNLICENSE.txt
#

import io
import logging
from pathlib import Path
from typing import Dict

from dotex.util import rgb_pack, rgb_unpack


class DoomPalette:
    playpal: Dict[int, int]

    def __init__(self):
        self.playpal = {}

    def _read_gimp_colors(self, fh: io.TextIOBase) -> None:
        """
        Read the main GIMP color palette colors.
        """
        index = 0
        while True:
            line = fh.readline()
            if line == "":
                break
            if line == "\n":
                continue
            elif line.startswith("#"):
                continue

            color = line.split()
            if len(color) < 3:
                raise Exception(f"Line '{line}' does not contain a color")

            rgb = rgb_pack(int(color[0]), int(color[1]), int(color[2]))
            self.playpal[rgb] = index
            index += 1

    def read_gimp_palette(self, file: Path) -> None:
        """
        Given a path, read a palette in GIMP palette format.

        See: https://developer.gimp.org/core/standards/gpl/
        """
        logging.debug(f"Reading '{file}' for palette.")

        with open(file, "rt") as fh:
            line = fh.readline().rstrip()
            if line != "GIMP Palette":
                raise Exception(f"{file.name} does not contain GIMP Palette")

            saved = fh.tell()
            line = fh.readline().rstrip("\r")
            if not line.startswith("Name:"):
                fh.seek(saved)
                return self._read_gimp_colors(fh)

            saved = fh.tell()
            line = fh.readline().rstrip("\r")
            if not line.startswith("Columns:"):
                fh.seek(saved)
                return self._read_gimp_colors(fh)

            return self._read_gimp_colors(fh)

    def add_close_color(self, rgb: int) -> int:
        """
        Seek out the closest color to a given palette index.
        """
        logging.debug(f"Finding color close to {rgb:06x}")

        r, g, b = rgb_unpack(rgb)

        closest_index: int | None = None
        closest_distsq: float = float("inf")
        for color in self.playpal.keys():
            # [LM] Calling rgb_unpack slows this down
            cr = color & 0xFF
            cg = (color >> 8) & 0xFF
            cb = (color >> 16) & 0xFF

            dr = r - cr
            dg = g - cg
            db = b - cb

            distsq = dr * dr + dg * dg + db * db
            if distsq < closest_distsq:
                closest_index = self.playpal[color]
                closest_distsq = distsq

        assert closest_index is not None
        self.playpal[rgb] = closest_index
        return closest_index
