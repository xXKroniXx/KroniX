"""Generuje res/kronix.ico (fedora ze złotą opaską) — PNG w kontenerze ICO."""
import struct, zlib

ART = [
    "................",
    "....oooooooo....",
    "...oRRRRRRRRo...",
    "..oRRRRRRRRRRo..",
    "..oRRRRRRRRRRo..",
    "..oRRRRRRRRRRo..",
    "..oRRRRRRRRRRo..",
    "..oGGGGGGGGGGo..",
    "..oGGGGGGGGGGo..",
    "ooRRRRRRRRRRRRoo",
    "oRRRRRRRRRRRRRRo",
    ".oooooooooooooo.",
    "................",
    "....YY.YY.YY....",
    "................",
    "................",
]
COL = {'o': (20, 16, 28, 255), 'R': (40, 38, 52, 255), 'G': (232, 176, 48, 255), 'Y': (177, 62, 83, 255)}

def image(size):
    px = []
    for y in range(size):
        row = []
        for x in range(size):
            # tło: bordowe kółko
            cx = cy = (size - 1) / 2
            r = size / 2
            d = ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
            bg = (122, 42, 58, 255) if d < r - size / 32 else ((20, 16, 28, 255) if d < r else (0, 0, 0, 0))
            ax = int((x - size * 0.12) / (size * 0.76) * 16)
            ay = int((y - size * 0.2) / (size * 0.76) * 16)
            c = bg
            if 0 <= ax < 16 and 0 <= ay < 16 and ART[ay][ax] in COL and ART[ay][ax] != 'Y':
                c = COL[ART[ay][ax]]
            row.append(c)
        px.append(row)
    return px

def png(size):
    px = image(size)
    raw = b''.join(b'\x00' + b''.join(bytes(c) for c in row) for row in px)
    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', size, size, 8, 6, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))

sizes = [16, 32, 48, 256]
imgs = [png(s) for s in sizes]
out = struct.pack('<HHH', 0, 1, len(sizes))
off = 6 + 16 * len(sizes)
for s, d in zip(sizes, imgs):
    out += struct.pack('<BBBBHHII', s % 256, s % 256, 0, 0, 1, 32, len(d), off)
    off += len(d)
open('res/kronix.ico', 'wb').write(out + b''.join(imgs))
open('/tmp/claude-0/-home-user-KroniX/52a5f711-9a48-5800-bbb7-fda38f66b6b3/scratchpad/icon256.png', 'wb').write(imgs[-1])
print('ok')
