#!/usr/bin/env python
#
# -----------------------------------------------------------------------------
#
# Copyright (C) 2006-2024 by The Odamex Team.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# DESCRIPTION:
#  Updates the version number and copyright date on a given set of source
#  files.  This is a version that is less safe and assumes a prior version
#  to make up the difference in safety.
#
# -----------------------------------------------------------------------------

from typing import List, Tuple

import re, sys, os
from configparser import ConfigParser
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT_DIR = Path(SCRIPT_DIR.parent.parent)


def re_replace(pattern: str, regex: str, repl: str):
    """
    Replace a pattern in a file.
    """

    for inode in ROOT_DIR.glob(pattern):
        with open(inode, "rb") as f:
            contents = f.read().decode("utf8", errors="surrogateescape")
            (contents, count) = re.subn(regex, repl, contents)
        if count > 0:
            with open(inode, "wb") as f:
                f.write(contents.encode("utf8", errors="surrogateescape"))
            print(f'Wrote "{inode}".')
        else:
            print(f'No matches found in "{inode}", skipping...')


def savestr(major: int, minor: int, patch: int) -> str:
    """
    Turn a version into a save-string.

    This string is exactly 16 characters and used as the header of the save
    file.  This system automatically flexes from single-digit numbers into
    taking up all 16 characters when we escape single digits to avoid ambiguous
    version headers.
    """

    if major <= 9 and minor <= 9 and patch <= 9:
        # We can do single-digit maj/min/patch.
        buffer = f"ODAMEXSAVE{major}{minor}{patch}   "
    else:
        # Patch can only be 0-9, Minor can only be 1-25.
        buffer = f"ODAMEXSAVE{major:03}{minor:02}{patch:01}"

    assert len(buffer) == 16
    return buffer


def configstr(major: int, minor: int, patch: int) -> str:
    """
    Turn a version into a config-string.

    This is hoisted into the config as a string, but is intended to be read
    as a number.  Numbers for newer versions must always compare greater
    than older versions when the string is turned into an int.
    """

    if major == 0 and minor <= 9 and patch <= 9:
        # We can do single-digit min/patch.
        buffer = f"{minor}{patch}"
    else:
        # Patch can only be 0-9, Minor can only be 1-25.
        buffer = f"{major:03}{minor:02}{patch:01}"

    return buffer


def ini_version(input: str) -> Tuple[int, int, int]:
    ver = [int(x) for x in input.split(".")]
    assert len(ver) == 3
    return tuple(ver)


def ini_files(input: str) -> List[str]:
    files = input.split("\n")
    return [x for x in files if len(x) > 0]


def main():
    with open(SCRIPT_DIR / "upversion.ini", "r") as fh:
        config = ConfigParser()
        config.read_file(fh)

    if not os.getenv('OLD_VERSION'):
        OLD_VERSION = "1.0.0"
    else:
        OLD_VERSION = os.getenv('OLD_VERSION')

    if not os.getenv('NEW_VERSION'):
        NEW_VERSION = "2.0.0"
    else:
        NEW_VERSION = os.getenv('NEW_VERSION')

    if not os.getenv('OLD_YEAR'):
        OLD_YEAR = r"2006-2024"
    else:
        OLD_YEAR = os.getenv('OLD_YEAR')

    if not os.getenv('NEW_YEAR'):
        NEW_YEAR = r"2006-2024"
    else:
        NEW_YEAR = os.getenv('NEW_YEAR')

    DOTTED_FILES = ini_files(config.get("files", "dotted"))
    COMMA_FILES = ini_files(config.get("files", "comma"))
    SAVESTR_FILES = ini_files(config.get("files", "savestr"))
    CONFIGSTR_FILES = ini_files(config.get("files", "configstr"))
    PLAINYEAR_FILES = ini_files(config.get("files", "plainyear"))
    FANCYYEAR_FILES = ini_files(config.get("files", "fancyyear"))

    oldmaj = int(OLD_VERSION.split(".")[0])
    oldmin = int(OLD_VERSION.split(".")[1])
    oldpatch = int(OLD_VERSION.split(".")[2])

    newmaj = int(NEW_VERSION.split(".")[0])
    newmin = int(NEW_VERSION.split(".")[1])
    newpatch = int(NEW_VERSION.split(".")[2])

    # Dotted version.
    DOTTED_RE = f"{oldmaj}\\.{oldmin}\\.{oldpatch}"
    DOTTED_REPL = f"{newmaj}.{newmin}.{newpatch}"
    [re_replace(glob, DOTTED_RE, DOTTED_REPL) for glob in DOTTED_FILES]

    # Comma-separated version.
    COMMA_RE = f"{oldmaj}, {oldmin}, {oldpatch}"
    COMMA_REPL = f"{newmaj}, {newmin}, {newpatch}"
    [re_replace(glob, COMMA_RE, COMMA_REPL) for glob in COMMA_FILES]

    # Fixed-length save strings.
    [
        re_replace(glob, savestr(oldmaj, oldmin, oldpatch), savestr(newmaj, newmin, newpatch))
        for glob in SAVESTR_FILES
    ]

    # Config strings.
    [
        re_replace(glob, configstr(oldmaj, oldmin, oldpatch), configstr(newmaj, newmin, newpatch))
        for glob in CONFIGSTR_FILES
    ]

    # CMake PROJECT_RC_VERSION
    CMAKERC_RE = f'"{oldmaj},{oldmin},{oldpatch},0"'
    CMAKERC_REPL = f'"{newmaj},{newmin},{newpatch},0"'
    re_replace("CMakeLists.txt", CMAKERC_RE, CMAKERC_REPL)

    # CMake PROJECT_COPYRIGHT
    re_replace("CMakeLists.txt", f'"{OLD_YEAR}"', f'"{NEW_YEAR}"')

    # Plain old copyright replacement - avoids comments.
    PLAINYEAR_RE = f"([^ ])Copyright \\(C\\) {OLD_YEAR} The Odamex Team([^ ])"
    PLAINYEAR_REPL = f"\\1Copyright (C) {NEW_YEAR} The Odamex Team\\2"
    [re_replace(glob, PLAINYEAR_RE, PLAINYEAR_REPL) for glob in PLAINYEAR_FILES]

    # Fancy copyright replacement - avoids comments.
    FANCYYEAR_RE = f"Copyright © {OLD_YEAR} The Odamex Team"
    FANCYYEAR_REPL = f"Copyright © {NEW_YEAR} The Odamex Team"
    [re_replace(glob, FANCYYEAR_RE, FANCYYEAR_REPL) for glob in FANCYYEAR_FILES]


if __name__ == "__main__":
    main()
