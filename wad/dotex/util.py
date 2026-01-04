#
# dotex: "I don't care, just do the tex or whatever..."
#
# Written in 2025 by Lexi Mayfield
# Released into the Public Domain, see UNLICENSE.txt
#


def rgb_pack(r: int, g: int, b: int) -> int:
    """Convert RGB to packed form."""
    return (r & 0xFF) | ((g & 0xFF) << 8) | ((b & 0xFF) << 16)


def rgb_unpack(rgb: int) -> tuple[int, int, int]:
    """Convert packed RGB to channel values."""
    r = rgb & 0xFF
    g = (rgb >> 8) & 0xFF
    b = (rgb >> 16) & 0xFF
    return (r, g, b)


def rgb_str(rgb: int) -> str:
    """A string repr of a packed RGB."""
    return f"#{rgb:06x}"
