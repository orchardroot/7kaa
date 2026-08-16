#!/usr/bin/env python3
"""Generate the Turret build-menu icon (F-11) from the Fort icon (F-5).

Decompresses the Fort icon from an unpacked I_BUTTON.RES directory,
clone-stamps parchment over the baked-in "Fort" label, draws "TURRET"
in its place, recompresses, and writes the F-11 record file.

Usage: python3 tools/make_turret_icon.py <buttons_dir>
 where <buttons_dir> was produced by: perl tools/deresx data/RESOURCE/I_BUTTON.RES <buttons_dir> .bin
"""
import glob
import struct
import sys

TRANSPARENT = 0xFF
TEXT_COLOR = 0    # black, same as the original label

# 4x6 caps, one string per row per glyph
FONT = {
    'T': ["1111", "0100", "0100", "0100", "0100", "0100"],
    'U': ["1001", "1001", "1001", "1001", "1001", "1111"],
    'R': ["1110", "1001", "1110", "1010", "1001", "1001"],
    'E': ["1111", "1000", "1110", "1000", "1000", "1111"],
}


def decompress(data, w, h):
    out = []
    i = 4
    while i < len(data) and len(out) < w * h:
        b = data[i]
        if b == 0xFF:
            out.append(TRANSPARENT)
            i += 1
        elif 0xF8 <= b <= 0xFE:
            if b == 0xF8:
                out += [TRANSPARENT] * data[i + 1]
                i += 2
            else:
                out += [TRANSPARENT] * (256 - b)
                i += 1
        else:
            out.append(b)
            i += 1
    assert len(out) == w * h, (len(out), w * h)
    return out


def compress(pix):
    out = bytearray()
    i = 0
    while i < len(pix):
        if pix[i] == TRANSPARENT:
            n = 1
            while i + n < len(pix) and pix[i + n] == TRANSPARENT and n < 255:
                n += 1
            if n == 1:
                out.append(0xFF)
            else:
                out += bytes((0xF8, n))
            i += n
        else:
            out.append(pix[i])
            i += 1
    return bytes(out)


def main():
    buttons_dir = sys.argv[1] if len(sys.argv) > 1 else "scratch/turret_data/buttons"
    (f5_path,) = glob.glob(buttons_dir + "/*-F-5.bin")
    data = open(f5_path, "rb").read()
    w, h = struct.unpack("<hh", data[:4])
    pix = decompress(data, w, h)

    # erase the "Fort" label: clone the parchment texture from 16px below
    for y in range(3, 21):
        for x in range(62, 98):
            pix[y * w + x] = pix[(y + 16) * w + x]

    # draw "TURRET", 4x6 glyphs with 1px spacing
    x0, y0 = 66, 9
    for ci, ch in enumerate("TURRET"):
        for ry, row in enumerate(FONT[ch]):
            for rx, bit in enumerate(row):
                if bit == "1":
                    pix[(y0 + ry) * w + x0 + ci * 5 + rx] = TEXT_COLOR

    out = struct.pack("<hh", w, h) + compress(pix)
    out_path = buttons_dir + "/99990-F-11.bin"
    open(out_path, "wb").write(out)
    print("wrote", out_path, len(out), "bytes")


if __name__ == "__main__":
    main()
