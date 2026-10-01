#ifndef FONT_DATA_H
#define FONT_DATA_H
typedef struct { int font, cp, x, y, w, h; float xoff, yoff, adv; } FontGlyph;
extern const int FONT_ATLAS_W, FONT_ATLAS_H, FONT_BASE, FONT_SPREAD;
extern const float FONT_METRICS[][2];
extern const int FONT_NGLYPHS;
extern const FontGlyph FONT_GLYPHS[];
extern const int FONT_RLE_LEN;
extern const unsigned char FONT_RLE[];
#endif
