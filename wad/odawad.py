#
# Copyright (C) 2006-2025 by The Odamex Team.
#

import configparser
import logging
from configparser import ConfigParser
from dataclasses import dataclass
from pathlib import Path

from dotex import DoomFlat, DoomPalette, DoomPatch, DoomWADWriter


@dataclass
class BuildContext:
    config: ConfigParser
    pal: DoomPalette
    wad: DoomWADWriter


def read_lump(dir: str, lump: str) -> bytes:
    files = Path(dir).glob(lump + ".*")

    # This is a loop, but we always want the first matched file.
    for file in files:
        with open(file, "rb+") as fh:
            return fh.read()

    raise Exception(f"'{lump}' not found in '{dir}'")


def read_patch(dir: str, lump: str) -> DoomPatch:
    files = Path(dir).glob(lump + ".*")

    # This is a loop, but we always want the first matched file.
    for file in files:
        patch = DoomPatch()
        patch.read_image(file)
        return patch

    raise Exception(f"'{lump}' not found in '{dir}'")


def read_flat(dir: str, lump: str) -> DoomFlat:
    files = Path(dir).glob(lump + ".*")

    # This is a loop, but we always want the first matched file.
    for file in files:
        flat = DoomFlat()
        flat.read_image(file)
        return flat

    raise Exception(f"'{lump}' not found in '{dir}'")


def pack_lumps(ctx: BuildContext):
    for lump in ctx.config["lumps"]:
        data = read_lump("lumps", lump)
        ctx.wad.write_lump(lump.upper(), data)


def pack_graphics(ctx: BuildContext):
    for graphics in ctx.config["graphics"]:
        filename, leftstr, topstr = graphics.split()

        graphic = read_patch("graphics", filename)
        graphic.set_left_top(int(leftstr), int(topstr))
        data = graphic.to_bytes(ctx.pal)
        ctx.wad.write_lump(filename.upper(), data)


def pack_sprites(ctx: BuildContext):
    ctx.wad.write_lump("SS_START")

    for sprites in ctx.config["sprites"]:
        filename, leftstr, topstr = sprites.split()

        sprite = read_patch("sprites", filename)
        sprite.set_left_top(int(leftstr), int(topstr))
        data = sprite.to_bytes(ctx.pal)
        ctx.wad.write_lump(filename.upper(), data)

    ctx.wad.write_lump("SS_END")


def pack_flats(ctx: BuildContext):
    ctx.wad.write_lump("FF_START")

    for flat in ctx.config["flats"]:
        filename = flat

        flat = read_flat("flats", filename)
        data = flat.to_bytes(ctx.pal)
        ctx.wad.write_lump(filename.upper(), data)

    ctx.wad.write_lump("FF_END")


def main(output_file: str, manifest: str):
    output_path = Path(output_file)
    manifest_path = Path(manifest)

    logging.info(f"Reading '{manifest_path.name}'...")

    config = configparser.ConfigParser(allow_no_value=True)
    config.read(Path(manifest))

    palette = DoomPalette()
    palette.read_gimp_palette(Path(__file__).parent / "doom.gpl")

    odamex_wad = DoomWADWriter(output_path)

    ctx = BuildContext(config=config, pal=palette, wad=odamex_wad)

    logging.info("Packing lumps...")
    pack_lumps(ctx)

    logging.info("Packing graphics...")
    pack_graphics(ctx)

    logging.info("Packing sprites...")
    pack_sprites(ctx)

    logging.info("Packing flats...")
    pack_flats(ctx)

    logging.info(f"Finalizing '{output_path.name}'...")
    odamex_wad.finalize()

    logging.info("...Done!")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()

    parser.add_argument(
        "-o",
        "--output",
        required=True,
        help="location and name of output WAD file",
        metavar="output_file",
    )
    parser.add_argument("manifest", help="WAD manifest file to read")

    args = parser.parse_args()

    logging.basicConfig(
        level=logging.INFO, format="%(relativeCreated)8.3fms %(levelname)s: %(message)s"
    )

    main(args.output, args.manifest)
