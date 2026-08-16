#!/usr/bin/env python3
"""Give the Turret its own building sprite, distinct from the Fort.

Takes the Fort's main bitmap (CAMP_A in I_FIRM.RES), scales it to 2/3
size and recolours the yellow roofs slate-blue, producing a compact keep
with a 2x2 footprint. Appends the new bitmap to I_FIRM.RES, then patches
the DBFs in an unpacked STD.SET directory:

  FBITMAP  + TURRET record (new bitmap, loc 2x2)
  FFRAME   + TURRET record (one frame)
  FBUILD     TURRET row repointed at the new frame; under-construction
             and ground bitmaps cleared (engine falls back sanely)
  FDBUILD  + TURRET record cloning the Fort's destruction animation --
             without this, destroying a turret reads out of bounds

Also regenerates the F-11 build-menu icon from the new sprite.

Usage: python3 tools/make_turret_sprite.py <set_dir> <buttons_dir>
  set_dir/buttons_dir from: perl tools/deresx data/RESOURCE/STD.SET <set_dir> .dbf
                            perl tools/deresx data/RESOURCE/I_BUTTON.RES <buttons_dir> .bin
Then repack both archives with tools/libresx.
"""
import glob
import struct
import sys

TRANSPARENT = 0xFF
CAMP_A_OFFSET = 274407          # CAMP_A record in the pristine I_FIRM.RES
SCALE_NUM, SCALE_DEN = 2, 3     # 2/3 size


# ---------------- bitmap helpers ----------------

def decompress(body, w, h):
    out = []
    i = 0
    while len(out) < w * h and i < len(body):
        b = body[i]
        if b == 0xFF:
            out.append(TRANSPARENT)
            i += 1
        elif 0xF8 <= b <= 0xFE:
            if b == 0xF8:
                out += [TRANSPARENT] * body[i + 1]
                i += 2
            else:
                out += [TRANSPARENT] * (256 - b)
                i += 1
        else:
            out.append(b)
            i += 1
    assert len(out) == w * h
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


def load_palette():
    pal = open("data/RESOURCE/PAL_STD.RES", "rb").read()
    return [(pal[8 + j * 3], pal[9 + j * 3], pal[10 + j * 3])
            for j in range(256)]


def build_roof_recolour(palette):
    """map the yellow roof ramp onto the palette's blue-grey ramp"""
    yellows = [i for i, (r, g, b) in enumerate(palette)
               if r > 110 and g > 80 and b < 70 and r >= g > b]
    blues = [i for i, (r, g, b) in enumerate(palette)
             if b > r + 15 and b > 60 and abs(int(r) - g) < 40]
    blues.sort(key=lambda i: sum(palette[i]))
    mapping = {}
    for y in yellows:
        lum = sum(palette[y])
        mapping[y] = min(blues, key=lambda b: abs(sum(palette[b]) - lum))
    return mapping


# ---------------- generic DBF editing ----------------

class Dbf:
    def __init__(self, path):
        self.path = path
        d = open(path, "rb").read()
        self.records, self.hsize, self.rsize = struct.unpack("<LHH", d[4:12])
        self.header = bytearray(d[:self.hsize])
        self.fields = []            # (name, offset_in_record, length)
        off = 1                     # record status byte
        p = 32
        while d[p] != 0x0D:
            name = d[p:p + 11].split(b"\0")[0].decode()
            flen = d[p + 16]
            self.fields.append((name, off, flen))
            off += flen
            p += 32
        body = d[self.hsize:]
        self.rows = [bytearray(body[i * self.rsize:(i + 1) * self.rsize])
                     for i in range(self.records)]

    def field(self, name):
        for n, off, flen in self.fields:
            if n == name:
                return off, flen
        raise KeyError(name)

    def get(self, row, name):
        off, flen = self.field(name)
        return bytes(self.rows[row][off:off + flen])

    def set(self, row, name, value):
        off, flen = self.field(name)
        if isinstance(value, str):
            value = value.ljust(flen).encode()
        assert len(value) == flen, (name, value)
        self.rows[row][off:off + flen] = value

    def set_num(self, row, name, value):
        off, flen = self.field(name)
        self.set(row, name, str(value).rjust(flen))

    def find(self, name, prefix):
        off, flen = self.field(name)
        for i, r in enumerate(self.rows):
            if r[off:off + flen].startswith(prefix.encode()):
                return i
        return -1

    def clone(self, row):
        self.rows.append(bytearray(self.rows[row]))
        return len(self.rows) - 1

    def write(self):
        self.header[4:8] = struct.pack("<L", len(self.rows))
        out = bytes(self.header) + b"".join(bytes(r) for r in self.rows)
        open(self.path, "wb").write(out + b"\x1a")


# ---------------- main ----------------

def main():
    set_dir = sys.argv[1] if len(sys.argv) > 1 else "scratch/turret_data/set"
    buttons_dir = sys.argv[2] if len(sys.argv) > 2 else "scratch/turret_data/buttons"

    palette = load_palette()

    # ---- build the sprite ----
    res_path = "data/RESOURCE/I_FIRM.RES"
    res = open(res_path, "rb").read()
    size, = struct.unpack("<i", res[CAMP_A_OFFSET:CAMP_A_OFFSET + 4])
    w, h = struct.unpack("<hh", res[CAMP_A_OFFSET + 4:CAMP_A_OFFSET + 8])
    fort = decompress(res[CAMP_A_OFFSET + 8:CAMP_A_OFFSET + 4 + size], w, h)

    nw, nh = w * SCALE_NUM // SCALE_DEN, h * SCALE_NUM // SCALE_DEN
    roof = build_roof_recolour(palette)
    pix = []
    for y in range(nh):
        for x in range(nw):
            p = fort[(y * SCALE_DEN // SCALE_NUM) * w + (x * SCALE_DEN // SCALE_NUM)]
            pix.append(roof.get(p, p))

    if any(b"TURRET" in open(f, "rb").read() for f in glob.glob(set_dir + "/*-FBITMAP.dbf")):
        print("FBITMAP already has a TURRET record; nothing to do")
        return

    new_offset = len(res)
    body = struct.pack("<hh", nw, nh) + compress(pix)
    rec = struct.pack("<i", len(body)) + body
    open(res_path, "ab").write(rec)
    print("sprite %dx%d appended to I_FIRM.RES at offset %d" % (nw, nh, new_offset))

    # ---- patch the DBFs ----
    (fbitmap_path,) = glob.glob(set_dir + "/*-FBITMAP.dbf")
    (fframe_path,) = glob.glob(set_dir + "/*-FFRAME.dbf")
    (fbuild_path,) = glob.glob(set_dir + "/*-FBUILD.dbf")
    (fdbuild_path,) = glob.glob(set_dir + "/*-FDBUILD.dbf")

    fbitmap = Dbf(fbitmap_path)
    camp_a = fbitmap.find("FILENAME", "CAMP_A")
    t = fbitmap.clone(camp_a)
    fbitmap.set(t, "FIRM", "TURRET")
    fbitmap.set(t, "FILENAME", "TURRET")
    fbitmap.set_num(t, "LOC_WIDTH", 2)
    fbitmap.set_num(t, "LOC_HEIGHT", 2)
    fbitmap.set_num(t, "OFFSET_X", -2)
    fbitmap.set_num(t, "OFFSET_Y", 4)
    fbitmap.set(t, "BITMAPPTR", struct.pack("<I", new_offset))
    fbitmap.write()
    turret_bitmap_recno = t + 1

    fframe = Dbf(fframe_path)
    camp_f = fframe.find("FIRM", "CAMP")
    t = fframe.clone(camp_f)
    fframe.set(t, "FIRM", "TURRET")
    fframe.set_num(t, "FIRSTBMP", turret_bitmap_recno)
    fframe.set_num(t, "BMPCOUNT", 1)
    fframe.write()
    turret_frame_recno = t + 1

    fbuild = Dbf(fbuild_path)
    t = fbuild.find("FIRM", "TURRET")
    assert t >= 0, "run tools/add_turret_data.pl first"
    fbuild.set_num(t, "FIRSTFRAME", turret_frame_recno)
    fbuild.set_num(t, "FRAMECOUNT", 1)
    fbuild.set_num(t, "UNDERC_BMP", 0)   # engine falls back to main bitmap
    fbuild.set_num(t, "UNDERC_MAX", 0)
    fbuild.set_num(t, "GROUND_BMP", 0)
    fbuild.write()

    # destruction animation: clone the Fort's FDBUILD row so that
    # firm_build_id (the turret's FBUILD recno) resolves in FirmDieRes
    fdbuild = Dbf(fdbuild_path)
    while len(fdbuild.rows) < t:         # pad so the new row's recno == t+1
        fdbuild.clone(fdbuild.find("FIRM", "CAMP"))
    fd = fdbuild.clone(fdbuild.find("FIRM", "CAMP"))
    fdbuild.set(fd, "FIRM", "TURRET")
    fdbuild.write()
    print("DBF records: FBITMAP %d, FFRAME %d, FBUILD row %d, FDBUILD rows %d"
          % (turret_bitmap_recno, turret_frame_recno, t + 1, len(fdbuild.rows)))

    # ---- rebuild the F-11 icon from the new sprite ----
    (f5_path,) = glob.glob(buttons_dir + "/*-F-5.bin")
    icon = open(f5_path, "rb").read()
    iw, ih = struct.unpack("<hh", icon[:4])
    ipix = decompress(icon[4:], iw, ih)

    # erase the old "Fort" label first (clone parchment from below it), so
    # the fort-picture erase can safely sample from the right half
    for y in range(3, 21):
        for x in range(62, iw - 2):
            ipix[y * iw + x] = ipix[(y + 16) * iw + x]

    # erase the fort picture (x2..67) with scattered parchment samples from
    # a clean patch of the right half -- deterministic hash avoids both
    # tiling seams and the border vignette
    for y in range(3, ih - 2):
        for x in range(2, 68):
            sx = 68 + (x * 7 + y * 13) % 22
            sy = 24 + (x * 5 + y * 3) % 28
            ipix[y * iw + x] = ipix[sy * iw + sx]
    sw, sh = nw * 4 // 5, nh * 4 // 5
    ox, oy = 4, ih - sh - 3
    for y in range(sh):
        for x in range(sw):
            p = pix[(y * 5 // 4) * nw + (x * 5 // 4)]
            if p != TRANSPARENT:
                ipix[(oy + y) * iw + ox + x] = p

    # TURRET label (same pixel font as make_turret_icon.py)
    sys.path.insert(0, "tools")
    from make_turret_icon import FONT, TEXT_COLOR
    x0, y0 = 66, 9
    for ci, ch in enumerate("TURRET"):
        for ry, row in enumerate(FONT[ch]):
            for rx, bit in enumerate(row):
                if bit == "1":
                    ipix[(y0 + ry) * iw + x0 + ci * 5 + rx] = TEXT_COLOR

    out = struct.pack("<hh", iw, ih) + compress(ipix)
    open(buttons_dir + "/99990-F-11.bin", "wb").write(out)
    print("icon rebuilt")


if __name__ == "__main__":
    main()
