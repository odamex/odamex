#
# dotex: "I don't care, just do the tex or whatever..."
#
# Written in 2025 by Lexi Mayfield
# Released into the Public Domain, see UNLICENSE.txt
#

import logging
import io
import struct
from array import array
from pathlib import Path

import dotex.png as png
from dotex.palette import DoomPalette


class DoomFlat:
    name: str
    width: int
    height: int
    data: array  # Array of uint containing RGBA data.

    def __init__(self):
        self.name = ""
        self.width = 0
        self.height = 0
        self.data = array("L")

    def read_image(self, file: Path) -> None:
        """
        Given a path, read in the image data.
        """
        logging.debug(f"Reading '{file}' for flat.")

        reader = png.Reader(filename=file)
        self.name = file.stem
        self.width, self.height, pixels, metadata = reader.read()
        if self.width != 64 or self.height != 64:
            raise Exception("Flat must be 64x64")

        if "palette" in metadata:
            # Paletted image
            for row in pixels:
                for pixel in row:
                    color = metadata["palette"][pixel]
                    co = color[0]
                    co |= color[1] << 8
                    co |= color[2] << 16
                    if len(color) == 4:
                        if color[3] != 255:
                            raise Exception("Transparency not allowed in flat")

                        co |= color[3] << 24
                    else:
                        co |= 0xFF << 24
                    self.data.append(co)
        elif metadata["greyscale"] is True:
            # Greyscale image
            for row in pixels:
                for pixel in row:
                    co = pixel
                    co |= pixel << 8
                    co |= pixel << 16
                    co |= 0xFF << 24
                    self.data.append(co)
        else:
            raise Exception("Non-palette PNG not supported (yet)")

    def to_bytes(self, pal: DoomPalette) -> bytes:
        """
        Convert raw graphic data to Doom Patch format.

        Based on the pseudo-code from https://doomwiki.org/wiki/Picture_format
        """
        logging.debug(f"Writing flat '{self.name}'.")

        data = io.BytesIO()

        for y in range(self.height):
            for x in range(self.height):
                rgba = self.data[y * self.width + x]
                rgb = rgba & 0xFFFFFF

                icolor = pal.playpal.get(rgb)
                if icolor is None:
                    icolor = pal.add_close_color(rgb)

                data.write(struct.pack("<B", icolor))

        data.seek(0)
        return data.read()
