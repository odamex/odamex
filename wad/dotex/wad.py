#
# dotex: "I don't care, just do the tex or whatever..."
#
# Written in 2025 by Lexi Mayfield
# Released into the Public Domain, see UNLICENSE.txt
#

import io
import logging
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional


@dataclass
class Lump:
    filepos: int
    size: int
    name: str


class DoomWADWriter:
    name: str
    handle: io.BufferedRandom
    lumps: List[Lump]

    def __init__(self, file: Path):
        self.name = str(file.name)
        self.lumps = []
        self.handle = open(file, "wb+")
        self.handle.write(b"\x00" * 12)  # Blank WAD header

    def write_lump(self, name: str, data: Optional[bytes] = None) -> None:
        """Write out lump data, while also noting its name and location."""
        logging.debug(f"Writing lump '{name}' to '{self.name}'.")

        if data is None:
            self.lumps.append(Lump(filepos=0, size=0, name=name))
            return

        self.lumps.append(Lump(filepos=self.handle.tell(), size=len(data), name=name))
        self.handle.write(data)

    def finalize(self, iwad: bool = False):
        """
        Finalize the WAD file - write out the directory, fix up the header
        to match, then close the file.
        """
        logging.debug(f"Finalizing WAD '{self.name}'.")

        # Pad to 4 bytes.
        while self.handle.tell() % 4 != 0:
            self.handle.write(b"\x00")

        infotableofs = self.handle.tell()

        for lump in self.lumps:
            self.handle.write(
                struct.pack("<II8s", lump.filepos, lump.size, lump.name.encode("ascii"))
            )

        self.handle.seek(0)
        if iwad:
            self.handle.write(b"IWAD")
        else:
            self.handle.write(b"PWAD")

        self.handle.write(struct.pack("<II", len(self.lumps), infotableofs))
        self.handle.close()
