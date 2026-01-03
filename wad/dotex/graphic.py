#
# Written in 2025 by Lexi Mayfield
# Released into the Public Domain, see UNLICENSE.txt
#

import io
import struct
from array import array
from pathlib import Path

import dotex.png as png
from dotex.palette import DoomPalette


class DoomGraphic:
    name: str
    width: int
    height: int
    left: int
    top: int
    data: array  # Array of uint containing RGBA data.

    def __init__(self):
        self.name = ""
        self.width = 0
        self.height = 0
        self.left = 0
        self.top = 0
        self.data = array("L")

    def __repr__(self) -> str:
        return f"<DoomGraphic name={self.name} res={self.width}x{self.height} left={self.left} top={self.top}>"

    def set_left_top(self, left: int, top: int) -> None:
        """
        Set the left and top attributes of the graphic.  You should get these
        as a pair.
        """

        self.left = left
        self.top = top

    def read_image(self, file: Path) -> None:
        """
        Given a path, try and use Imagemagick to read the graphic data.
        """

        reader = png.Reader(filename=file)
        self.name = file.stem
        self.width, self.height, pixels, metadata = reader.read()
        if "palette" in metadata:
            # Paletted image
            for row in pixels:
                for pixel in row:
                    color = metadata["palette"][pixel]
                    co = color[0]
                    co |= color[1] << 8
                    co |= color[2] << 16
                    co |= color[3] << 24
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
        data = io.BytesIO()

        # Patch header
        data.write(struct.pack("<HHhh", self.width, self.height, self.left, self.top))

        # Keep track of column offsets.
        columnofs: list[int] = []
        columns_off = data.tell()

        # Write out an empty columnofs for now.
        data.write(b"\x00\x00\x00\x00" * self.width)
        for x in range(self.width):
            # Align start of column data to four bytes
            while data.tell() % 4 != 0:
                data.write(b"\x00")

            # Note the start of the current column.
            columnofs.append(data.tell())

            y = 0
            post_start = 0
            post_len = 0
            last_icolor: int | None = None

            def end_post():
                """
                End the current post.
                """
                assert last_icolor is not None
                data.write(struct.pack("<B", last_icolor))
                saved = data.tell()
                data.seek(post_start + 1)
                data.write(struct.pack("<B", post_len))
                data.seek(saved)

            while y < self.height:
                # Break the pixel into RGB and A.
                rgba = self.data[y * self.width + x]
                rgb = rgba & 0xFFFFFF
                a = (rgba & 0xFF000000) >> 24

                # Figure out our color by index.
                icolor: int | None
                if a > 0:
                    icolor = pal.playpal.get(rgb)
                    if icolor is None:
                        raise Exception(f"Could not find color {hex(rgb)} in palette")
                else:
                    icolor = None

                if icolor is not None and post_len == 0:
                    # Got pixel data, but we need to start the post header.
                    post_start = data.tell()
                    data.write(struct.pack("<BBBB", y, 0xFF, icolor, icolor))
                    last_icolor = icolor
                    post_len += 1
                elif icolor is not None and post_len > 0:
                    # In the middle of the post.
                    data.write(struct.pack("<B", icolor))
                    last_icolor = icolor
                    post_len += 1
                elif icolor is None and post_len > 0:
                    # Write the end of a post.
                    end_post()

                # Advance to the next pixel
                y += 1

            if post_len > 0:
                # Write the end of a post.
                end_post()

            # Write end-of-column flag footer.
            data.write(b"\xff")

        # Write the column offsets.
        data.seek(columns_off)
        for col in columnofs:
            data.write(struct.pack("<I", col))

        data.seek(0)
        return data.read()
