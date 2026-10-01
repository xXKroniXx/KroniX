"""Generuje atlas czcionek SDF (signed distance field) -> src/font_data.c

Czcionki: Liberation Sans (Regular, Bold) i Liberation Serif (Bold) — licencja SIL OFL 1.1,
symbole z DejaVu Sans (licencja Bitstream Vera / public domain).
Wymaga: Pillow, numpy.  Uruchom: python3 tools/make_fonts.py
"""
import os
import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, '..', 'src', 'font_data.c')
FD = '/usr/share/fonts/truetype/'
FONTS = [
    ('sans', FD + 'liberation/LiberationSans-Regular.ttf'),
    ('sans_bold', FD + 'liberation/LiberationSans-Bold.ttf'),
    ('serif', FD + 'liberation/LiberationSerif-Bold.ttf'),
]
FALLBACK = FD + 'dejavu/DejaVuSans.ttf'
SIZE = 40          # rozmiar bazowy w atlasie (px)
SPREAD = 5         # zasięg pola odległości (px)
SS = 4             # nadpróbkowanie przy rasteryzacji
ATLAS_W, ATLAS_H = 1024, 1024

chars = [chr(c) for c in range(32, 127)]
chars += list('ąćęłńóśźżĄĆĘŁŃÓŚŹŻ„”“–—…•·°€éèàòùìáíúüöäßçÉÈÀ’‘«»×')
chars += ['▶', '▼', '★', '♥', '←', '→', '↑', '↓', '✓', '✖', '●', '⌂']


_notdef = {}


def has_glyph(font, ch):
    """Brak glifu = Pillow rysuje prostokąt .notdef — porównujemy z glifem prywatnego obszaru."""
    try:
        key = id(font)
        if key not in _notdef:
            m = font.getmask('\ue000')
            _notdef[key] = (m.size, bytes(m))
        mask = font.getmask(ch)
        if ch != ' ' and (mask.size, bytes(mask)) == _notdef[key]:
            return False
        return mask.getbbox() is not None or ch == ' '
    except Exception:
        return False


def sdf_glyph(font, ch):
    """Zwraca (img uint8 HxW, xoff, yoff, advance) w skali SIZE."""
    asc, desc = font.getmetrics()
    adv = font.getlength(ch)
    bbox = font.getbbox(ch)
    if ch == ' ' or bbox is None or bbox[2] <= bbox[0]:
        return None, 0, 0, adv / SS
    pad = SPREAD * SS + 2
    w = bbox[2] - bbox[0] + 2 * pad
    h = bbox[3] - bbox[1] + 2 * pad
    img = Image.new('L', (w, h), 0)
    ImageDraw.Draw(img).text((pad - bbox[0], pad - bbox[1]), ch, font=font, fill=255)
    a = np.asarray(img, dtype=np.float32) / 255.0
    inside = a > 0.5
    # punkty brzegowe (piksele, których sąsiad jest po drugiej stronie)
    edge = np.zeros_like(inside)
    edge[:-1, :] |= inside[:-1, :] != inside[1:, :]
    edge[1:, :] |= inside[:-1, :] != inside[1:, :]
    edge[:, :-1] |= inside[:, :-1] != inside[:, 1:]
    edge[:, 1:] |= inside[:, :-1] != inside[:, 1:]
    ey, ex = np.nonzero(edge)
    ow, oh = (w + SS - 1) // SS, (h + SS - 1) // SS
    gy, gx = np.mgrid[0:oh, 0:ow]
    sx = (gx.ravel() * SS + SS / 2.0)
    sy = (gy.ravel() * SS + SS / 2.0)
    if len(ex) == 0:
        return None, 0, 0, adv / SS
    best = np.full(sx.shape, 1e9, dtype=np.float32)
    exf, eyf = ex.astype(np.float32) + 0.5, ey.astype(np.float32) + 0.5
    for i in range(0, len(exf), 512):
        dx = sx[:, None] - exf[None, i:i + 512]
        dy = sy[:, None] - eyf[None, i:i + 512]
        best = np.minimum(best, np.min(dx * dx + dy * dy, axis=1))
    dist = np.sqrt(best) / SS
    ix = np.clip(sx.astype(int), 0, w - 1)
    iy = np.clip(sy.astype(int), 0, h - 1)
    sign = np.where(inside[iy, ix], 1.0, -1.0)
    sd = (sign * dist).reshape(oh, ow)
    v = np.clip(128 + sd * (127.0 / SPREAD), 0, 255).astype(np.uint8)
    xoff = (bbox[0] - pad) / SS
    yoff = (bbox[1] - pad) / SS - asc / SS   # względem linii bazowej
    return v, xoff, yoff, adv / SS


def main():
    atlas = np.zeros((ATLAS_H, ATLAS_W), dtype=np.uint8)
    cx = cy = 0
    rowh = 0
    glyphs = []
    metrics = []
    fb = ImageFont.truetype(FALLBACK, SIZE * SS)
    for fi, (name, path) in enumerate(FONTS):
        font = ImageFont.truetype(path, SIZE * SS)
        asc, desc = font.getmetrics()
        metrics.append((asc / SS, desc / SS))
        for ch in chars:
            f = font if has_glyph(font, ch) else fb
            if not has_glyph(f, ch):
                continue
            img, xo, yo, adv = sdf_glyph(f, ch)
            if img is None:
                glyphs.append((fi, ord(ch), 0, 0, 0, 0, 0.0, 0.0, adv))
                continue
            gh, gw = img.shape
            if cx + gw > ATLAS_W:
                cx = 0
                cy += rowh + 1
                rowh = 0
            if cy + gh > ATLAS_H:
                raise SystemExit('atlas za mały')
            atlas[cy:cy + gh, cx:cx + gw] = img
            glyphs.append((fi, ord(ch), cx, cy, gw, gh, xo, yo, adv))
            cx += gw + 1
            rowh = max(rowh, gh)
    used_h = cy + rowh + 1
    print('glifów: %d, atlas %dx%d (użyte %d wierszy)' % (len(glyphs), ATLAS_W, ATLAS_H, used_h))
    with open(OUT, 'w') as f:
        f.write('/* Wygenerowane przez tools/make_fonts.py — nie edytować.\n')
        f.write(' * Liberation Sans/Serif: SIL Open Font License 1.1. DejaVu Sans: licencja Bitstream Vera. */\n')
        f.write('#include "font_data.h"\n')
        f.write('const int FONT_ATLAS_W = %d, FONT_ATLAS_H = %d, FONT_BASE = %d, FONT_SPREAD = %d;\n' % (ATLAS_W, used_h, SIZE, SPREAD))
        f.write('const float FONT_METRICS[%d][2] = {%s};\n' % (len(FONTS), ', '.join('{%.2ff, %.2ff}' % m for m in metrics)))
        f.write('const int FONT_NGLYPHS = %d;\n' % len(glyphs))
        f.write('const FontGlyph FONT_GLYPHS[] = {\n')
        for g in glyphs:
            f.write('  {%d, %d, %d, %d, %d, %d, %.2ff, %.2ff, %.2ff},\n' % g)
        f.write('};\n')
        # atlas: proste RLE (bajt wartości, bajt długości) — dużo zer i 255 w tle
        data = atlas[:used_h].ravel()
        rle = []
        i = 0
        n = len(data)
        while i < n:
            v = data[i]
            j = i + 1
            while j < n and data[j] == v and j - i < 255:
                j += 1
            rle.append(int(v)); rle.append(j - i)
            i = j
        f.write('const int FONT_RLE_LEN = %d;\n' % len(rle))
        f.write('const unsigned char FONT_RLE[] = {\n')
        for k in range(0, len(rle), 32):
            f.write(','.join(str(x) for x in rle[k:k + 32]) + ',\n')
        f.write('};\n')
    Image.fromarray(atlas[:used_h]).save(os.path.join(HERE, '..', 'build', 'font_atlas.png')) if os.path.isdir(os.path.join(HERE, '..', 'build')) else None


if __name__ == '__main__':
    main()
