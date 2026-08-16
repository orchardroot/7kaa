#!/usr/bin/env python3
"""Change the in-game menu's baked-in "Quit to Windows" to "Quit to OS".

Operates on the GAMEMENU bitmap from an unpacked I_IF.RES directory
(uncompressed: short w, short h, w*h palette bytes). The replacement is
composed from the bitmap's own art: the "Quit to " glyphs stay in place,
and the O and S are cloned from the "(O)" / "(S)" hotkey hints on the
Options and Save Game buttons, so the typeface matches exactly.

Usage: python3 tools/patch_gamemenu_os.py <iif_dir>
 where <iif_dir> was produced by: perl tools/deresx data/RESOURCE/I_IF.RES <iif_dir> .bin
"""
import glob
import struct
import sys


def main():
    iif_dir = sys.argv[1] if len(sys.argv) > 1 else "scratch/turret_data/iif"
    (path,) = glob.glob(iif_dir + "/*-GAMEMENU.bin")
    d = open(path, "rb").read()
    w, h = struct.unpack("<hh", d[:4])
    pix = bytearray(d[4:])
    orig = bytes(pix)

    pal = open("data/RESOURCE/PAL_STD.RES", "rb").read()
    brightness = [pal[8 + j * 3] + pal[9 + j * 3] + pal[10 + j * 3]
                  for j in range(256)]

    def copy_rect(sx, sy, dx, dy, cw, ch, ink_only=False, skip=None):
        """copy cw x ch pixels from (sx,sy) in the original to (dx,dy);
        skip(sx,sy) marks source pixels to leave out (stray paren tips)"""
        for y in range(ch):
            for x in range(cw):
                if skip and skip(sx + x, sy + y):
                    continue
                p = orig[(sy + y) * w + sx + x]
                if ink_only and brightness[p] >= 500:
                    continue          # glyph pixels only, keep dest texture
                pix[(dy + y) * w + dx + x] = p

    # Safe to re-run: all sources are taken from the unmodified `orig`.

    # 1. erase the whole "Quit to Windows" label by tiling the button's own
    #    left margin (x 96..115 is clean parchment on every affected row)
    for y in range(303, 326):
        for x in range(116, 235):
            pix[y * w + x] = orig[y * w + 96 + (x - 96) % 20]

    # 2. paste "Quit to" (x119..166 in the original), re-centered for the
    #    shorter label (new label spans ~x136..215, button center x175)
    copy_rect(119, 303, 136, 303, 48, 23)

    # 3. clone O from "Options (O)" and S from "Save Game (S)", ink only,
    #    baselines aligned to the row's y320 baseline
    copy_rect(196, 100, 186, 305, 13, 17, ink_only=True,   # O
              skip=lambda x, y: y <= 103 and x <= 200)     # "(" top tip
    copy_rect(208, 134, 200, 303, 11, 18, ink_only=True,   # S
              skip=lambda x, y: (y <= 137 and x <= 211) or  # "(" top tip
                                (y >= 149 and x >= 215))    # ")" bottom tip

    open(path, "wb").write(d[:4] + bytes(pix))
    print("patched", path)


if __name__ == "__main__":
    main()
