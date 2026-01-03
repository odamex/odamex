#
# Written in 2025 by Lexi Mayfield
# Released into the Public Domain, see UNLICENSE.txt
#

import io
from pathlib import Path


class DoomPalette:
    playpal: dict[int, int]

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

            rgb = int(color[0]) | (int(color[1]) << 8) | (int(color[2]) << 16)
            self.playpal[rgb] = index
            index += 1

    def read_gimp_palette(self, file: Path) -> None:
        """
        Given a path, read a palette in GIMP palette format.

        See: https://developer.gimp.org/core/standards/gpl/
        """
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
