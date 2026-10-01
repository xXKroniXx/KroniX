/* UI 2D: wsadowe rysowanie w wirtualnej przestrzeni 1280x720 — zaokrąglone panele (SDF),
 * miękkie cienie, gradienty, obrazy i tekst SDF (ostry w każdej rozdzielczości). */
#include "render.h"
#include "glapi.h"
#include "font_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float g_uiScale = 1, g_uiOffX, g_uiOffY;

typedef struct { float x, y, u, v; uint8_t c[4]; float hw, hh, rad, feather; float mode, th; } UV2;

static const char *VS_UI =
  "#version 330 core\n"
  "layout(location=0) in vec2 aPos; layout(location=1) in vec2 aUV; layout(location=2) in vec4 aCol;\n"
  "layout(location=3) in vec4 aPar; layout(location=4) in vec2 aMode;\n"
  "uniform vec2 uScreen; uniform float uScale; uniform vec2 uOff;\n"
  "out vec2 vUV; out vec4 vCol; out vec4 vPar; out vec2 vMode;\n"
  "void main(){ vec2 p = aPos*uScale + uOff; vUV = aUV; vCol = aCol; vPar = aPar; vMode = aMode;\n"
  "  gl_Position = vec4(p.x/uScreen.x*2.0-1.0, 1.0-p.y/uScreen.y*2.0, 0.0, 1.0); }\n";
static const char *FS_UI =
  "#version 330 core\n"
  "in vec2 vUV; in vec4 vCol; in vec4 vPar; in vec2 vMode;\n"
  "uniform sampler2D uImg; uniform sampler2D uFont; uniform float uScale;\n"
  "out vec4 frag;\n"
  "float sdRound(vec2 p, vec2 b, float r){ vec2 q = abs(p) - b + r; return length(max(q,0.0)) + min(max(q.x,q.y),0.0) - r; }\n"
  "void main(){\n"
  "  int m = int(vMode.x + 0.5);\n"
  "  if (m == 0) { frag = texture(uImg, vUV) * vCol; return; }\n"
  "  if (m == 1) { float d = texture(uFont, vUV).r; float w = max(fwidth(d)*0.75, 0.004); float soft = vPar.x;\n"
  "    float a = smoothstep(0.5 - w - soft, 0.5 + w, d); frag = vec4(vCol.rgb, vCol.a*a); return; }\n"
  "  float d = sdRound(vUV, vPar.xy, vPar.z);\n"
  "  float f = max(vPar.w, 0.6/uScale);\n"
  "  if (m == 2) { float a = 1.0 - smoothstep(-f, f, d); frag = vec4(vCol.rgb, vCol.a*a); return; }\n"
  "  if (m == 3) { float t = vMode.y*0.5; float a = 1.0 - smoothstep(t - f, t + f, abs(d + t)); frag = vec4(vCol.rgb, vCol.a*a); return; }\n"
  "  if (m == 4) { float a = 1.0 - smoothstep(-vPar.w, vPar.w, d); frag = vec4(vCol.rgb, vCol.a*a*a); return; }\n"
  "  frag = vCol;\n"
  "}\n";

#define MAXV 60000
static UV2 vb[MAXV];
static int nv;
static unsigned prog, vao, vbo, fontTex, whiteTex, curImg;
static int uScreen, uScale, uOff;
static int inUI;

static unsigned compile2(const char *vs, const char *fs) {
  unsigned p = glCreateProgram();
  const char *src[2] = {vs, fs};
  GLenum ty[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
  for (int i = 0; i < 2; i++) {
    unsigned s = glCreateShader(ty[i]);
    glShaderSource(s, 1, &src[i], NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(s, 1024, NULL, log); fprintf(stderr, "ui shader: %s\n", log); }
    glAttachShader(p, s);
    glDeleteShader(s);
  }
  glLinkProgram(p);
  return p;
}

/* ---------------------------------------------------------------- czcionki */
static const FontGlyph *gl_index[3][0x2700];
static float fscaleU, fscaleV;

void d2_tex_init(void) {
  prog = compile2(VS_UI, FS_UI);
  uScreen = glGetUniformLocation(prog, "uScreen");
  uScale = glGetUniformLocation(prog, "uScale");
  uOff = glGetUniformLocation(prog, "uOff");
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof vb, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UV2), (void *)0);
  glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(UV2), (void *)8);
  glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(UV2), (void *)16);
  glEnableVertexAttribArray(3); glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(UV2), (void *)20);
  glEnableVertexAttribArray(4); glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(UV2), (void *)36);
  glBindVertexArray(0);
  /* atlas czcionek z RLE */
  int n = FONT_ATLAS_W * FONT_ATLAS_H;
  uint8_t *a = (uint8_t *)malloc(n);
  int o = 0;
  for (int i = 0; i + 1 < FONT_RLE_LEN && o < n; i += 2)
    for (int k = 0; k < FONT_RLE[i + 1] && o < n; k++) a[o++] = FONT_RLE[i];
  glGenTextures(1, &fontTex);
  glBindTexture(GL_TEXTURE_2D, fontTex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, FONT_ATLAS_W, FONT_ATLAS_H, 0, GL_RED, GL_UNSIGNED_BYTE, a);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  free(a);
  fscaleU = 1.0f / FONT_ATLAS_W; fscaleV = 1.0f / FONT_ATLAS_H;
  for (int i = 0; i < FONT_NGLYPHS; i++) {
    const FontGlyph *g = &FONT_GLYPHS[i];
    if (g->font < 3 && g->cp < 0x2700) gl_index[g->font][g->cp] = g;
  }
  uint8_t w4[16];
  memset(w4, 255, 16);
  whiteTex = r_tex2d(2, 2, w4, 0);
}

static void flush(void) {
  if (!nv) return;
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, nv * sizeof(UV2), vb);
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, nv);
  nv = 0;
}

void d2_begin(void) {
  inUI = 1;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, g_winW, g_winH);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glUseProgram(prog);
  glUniform2f(uScreen, (float)g_winW, (float)g_winH);
  glUniform1f(uScale, g_uiScale);
  glUniform2f(uOff, g_uiOffX, g_uiOffY);
  glUniform1i(glGetUniformLocation(prog, "uImg"), 0);
  glUniform1i(glGetUniformLocation(prog, "uFont"), 1);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, fontTex);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, whiteTex);
  curImg = whiteTex;
  nv = 0;
}

void d2_end(void) {
  flush();
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_BLEND);
  inUI = 0;
}

static UV2 *push6(void) {
  if (nv + 6 > MAXV) flush();
  UV2 *v = &vb[nv];
  nv += 6;
  return v;
}

static void put(UV2 *v, float x, float y, float u, float vv, uint32_t col, float hw, float hh, float r, float f, float mode, float th) {
  v->x = x; v->y = y; v->u = u; v->v = vv;
  v->c[0] = (col >> 16) & 255; v->c[1] = (col >> 8) & 255; v->c[2] = col & 255; v->c[3] = (col >> 24) & 255;
  v->hw = hw; v->hh = hh; v->rad = r; v->feather = f; v->mode = mode; v->th = th;
}

/* prostokąt z parametrami SDF; u/v = lokalne współrzędne względem środka */
static void quad_sdf(float x, float y, float w, float h, float pad, uint32_t c0, uint32_t c1, float r, float f, float mode, float th) {
  UV2 *v = push6();
  float hw = w * 0.5f, hh = h * 0.5f;
  float x0 = x - pad, y0 = y - pad, x1 = x + w + pad, y1 = y + h + pad;
  float lx0 = -hw - pad, ly0 = -hh - pad, lx1 = hw + pad, ly1 = hh + pad;
  put(v + 0, x0, y0, lx0, ly0, c0, hw, hh, r, f, mode, th);
  put(v + 1, x1, y0, lx1, ly0, c0, hw, hh, r, f, mode, th);
  put(v + 2, x1, y1, lx1, ly1, c1, hw, hh, r, f, mode, th);
  put(v + 3, x0, y0, lx0, ly0, c0, hw, hh, r, f, mode, th);
  put(v + 4, x1, y1, lx1, ly1, c1, hw, hh, r, f, mode, th);
  put(v + 5, x0, y1, lx0, ly1, c1, hw, hh, r, f, mode, th);
}

void d2_rect(float x, float y, float w, float h, uint32_t col) { quad_sdf(x, y, w, h, 0, col, col, 0, 0, 2, 0); }
void d2_grad(float x, float y, float w, float h, uint32_t top, uint32_t bot) { quad_sdf(x, y, w, h, 0, top, bot, 0, 0, 2, 0); }
void d2_hgrad(float x, float y, float w, float h, uint32_t l, uint32_t r) {
  UV2 *v = push6();
  put(v + 0, x, y, 0, 0, l, w * 0.5f, h * 0.5f, 0, 0, 2, 0);
  put(v + 1, x + w, y, 0, 0, r, w * 0.5f, h * 0.5f, 0, 0, 2, 0);
  put(v + 2, x + w, y + h, 0, 0, r, w * 0.5f, h * 0.5f, 0, 0, 2, 0);
  put(v + 3, x, y, 0, 0, l, w * 0.5f, h * 0.5f, 0, 0, 2, 0);
  put(v + 4, x + w, y + h, 0, 0, r, w * 0.5f, h * 0.5f, 0, 0, 2, 0);
  put(v + 5, x, y + h, 0, 0, l, w * 0.5f, h * 0.5f, 0, 0, 2, 0);
}
void d2_rrect(float x, float y, float w, float h, float r, uint32_t col) { quad_sdf(x, y, w, h, 1, col, col, r, 0, 2, 0); }
void d2_rrect_line(float x, float y, float w, float h, float r, float th, uint32_t col) { quad_sdf(x, y, w, h, 1, col, col, r, 0, 3, th); }
void d2_shadow(float x, float y, float w, float h, float r, float soft, uint32_t col) { quad_sdf(x, y, w, h, soft * 1.5f, col, col, r, soft, 4, 0); }
void d2_circle(float cx, float cy, float r, uint32_t col) { quad_sdf(cx - r, cy - r, r * 2, r * 2, 1, col, col, r, 0, 2, 0); }

void d2_line(float x0, float y0, float x1, float y1, float th, uint32_t col) {
  float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
  if (l < 0.001f) return;
  float nx = -dy / l * th * 0.5f, ny = dx / l * th * 0.5f;
  UV2 *v = push6();
  float hw = 1, hh = 1;
  put(v + 0, x0 + nx, y0 + ny, 0, 0, col, hw, hh, 0, 0, 5, 0);
  put(v + 1, x1 + nx, y1 + ny, 0, 0, col, hw, hh, 0, 0, 5, 0);
  put(v + 2, x1 - nx, y1 - ny, 0, 0, col, hw, hh, 0, 0, 5, 0);
  put(v + 3, x0 + nx, y0 + ny, 0, 0, col, hw, hh, 0, 0, 5, 0);
  put(v + 4, x1 - nx, y1 - ny, 0, 0, col, hw, hh, 0, 0, 5, 0);
  put(v + 5, x0 - nx, y0 - ny, 0, 0, col, hw, hh, 0, 0, 5, 0);
}

void d2_image(unsigned tex, float x, float y, float w, float h, float u0, float v0, float u1, float v1, uint32_t col) {
  if (tex != curImg) { flush(); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex); curImg = tex; }
  UV2 *v = push6();
  put(v + 0, x, y, u0, v0, col, 0, 0, 0, 0, 0, 0);
  put(v + 1, x + w, y, u1, v0, col, 0, 0, 0, 0, 0, 0);
  put(v + 2, x + w, y + h, u1, v1, col, 0, 0, 0, 0, 0, 0);
  put(v + 3, x, y, u0, v0, col, 0, 0, 0, 0, 0, 0);
  put(v + 4, x + w, y + h, u1, v1, col, 0, 0, 0, 0, 0, 0);
  put(v + 5, x, y + h, u0, v1, col, 0, 0, 0, 0, 0, 0);
}

void d2_clip(float x, float y, float w, float h) {
  flush();
  glEnable(GL_SCISSOR_TEST);
  int sx = (int)(x * g_uiScale + g_uiOffX), sy = (int)(y * g_uiScale + g_uiOffY);
  int sw = (int)(w * g_uiScale + 0.5f), sh = (int)(h * g_uiScale + 0.5f);
  glScissor(sx, g_winH - sy - sh, sw, sh);
}
void d2_noclip(void) { flush(); glDisable(GL_SCISSOR_TEST); }

/* ---------------------------------------------------------------- tekst */
static int utf8(const char **p) {
  const unsigned char *s = (const unsigned char *)*p;
  int c = *s;
  if (c < 0x80) { *p += 1; return c; }
  if ((c & 0xE0) == 0xC0 && s[1]) { *p += 2; return ((c & 0x1F) << 6) | (s[1] & 0x3F); }
  if ((c & 0xF0) == 0xE0 && s[1] && s[2]) { *p += 3; return ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); }
  *p += 1;
  return '?';
}

static const FontGlyph *glyph(int font, int cp) {
  if (font < 0 || font > 2) font = 0;
  if (cp >= 0 && cp < 0x2700 && gl_index[font][cp]) return gl_index[font][cp];
  return gl_index[font]['?'];
}

float d2_line_h(int font, float size) { (void)font; return size * 1.28f; }

/* kody koloru {y} {r} itd. jak w dawnym silniku */
static uint32_t code_color(char c, uint32_t def) {
  uint32_t a = def & 0xFF000000u;
  switch (c) {
    case 'y': return a | 0xF2C46Bu;
    case 'r': return a | 0xE0605Au;
    case 'g': return a | 0x7CC97Au;
    case 's': return a | 0xA9B4C2u;
    case 'b': return a | 0x7FB2F0u;
    case 'o': return a | 0xF09A5Au;
    case 'w': return a | 0xF4F0E6u;
    case 'z': return a | 0xE8B84Au;
    default: return def;
  }
}

static float draw_impl(int font, float size, float x, float y, const char *s, uint32_t col, int maxc, int draw, float soft) {
  float sc = size / FONT_BASE;
  float base = y + FONT_METRICS[font][0] * sc;
  float cx = x;
  uint32_t c = col;
  int count = 0;
  while (*s) {
    if (s[0] == '{' && s[1] && s[2] == '}') { c = s[1] == '-' ? col : code_color(s[1], col); s += 3; continue; }
    if (*s == '\n') break;
    if (maxc >= 0 && count >= maxc) break;
    int cp = utf8(&s);
    count++;
    const FontGlyph *g = glyph(font, cp);
    if (!g) continue;
    if (draw && g->w > 0) {
      float gx = cx + g->xoff * sc, gy = base + g->yoff * sc;
      float gw = g->w * sc, gh = g->h * sc;
      float u0 = g->x * fscaleU, v0 = g->y * fscaleV, u1 = (g->x + g->w) * fscaleU, v1 = (g->y + g->h) * fscaleV;
      UV2 *v = push6();
      put(v + 0, gx, gy, u0, v0, c, soft, 0, 0, 0, 1, 0);
      put(v + 1, gx + gw, gy, u1, v0, c, soft, 0, 0, 0, 1, 0);
      put(v + 2, gx + gw, gy + gh, u1, v1, c, soft, 0, 0, 0, 1, 0);
      put(v + 3, gx, gy, u0, v0, c, soft, 0, 0, 0, 1, 0);
      put(v + 4, gx + gw, gy + gh, u1, v1, c, soft, 0, 0, 0, 1, 0);
      put(v + 5, gx, gy + gh, u0, v1, c, soft, 0, 0, 0, 1, 0);
    }
    cx += g->adv * sc;
  }
  return cx - x;
}

float d2_text(int font, float size, float x, float y, const char *s, uint32_t col) { return draw_impl(font, size, x, y, s, col, -1, 1, 0); }
float d2_text_w(int font, float size, const char *s) { return draw_impl(font, size, 0, 0, s, 0, -1, 0, 0); }
float d2_text_sh(int font, float size, float x, float y, const char *s, uint32_t col) {
  uint32_t a = (col >> 24) & 255;
  draw_impl(font, size, x + size * 0.04f, y + size * 0.07f, s, ARGB(a * 3 / 4, 0, 0, 0), -1, 1, 0.12f);
  return draw_impl(font, size, x, y, s, col, -1, 1, 0);
}
void d2_text_n(int font, float size, float x, float y, const char *s, uint32_t col, int maxc) {
  uint32_t a = (col >> 24) & 255;
  draw_impl(font, size, x + size * 0.04f, y + size * 0.07f, s, ARGB(a * 3 / 4, 0, 0, 0), maxc, 1, 0.12f);
  draw_impl(font, size, x, y, s, col, maxc, 1, 0);
}
void d2_text_c(int font, float size, float cx, float y, const char *s, uint32_t col) { d2_text_sh(font, size, cx - d2_text_w(font, size, s) * 0.5f, y, s, col); }
void d2_text_r(int font, float size, float xr, float y, const char *s, uint32_t col) { d2_text_sh(font, size, xr - d2_text_w(font, size, s), y, s, col); }

/* zawijanie tekstu; zachowuje aktywny kod koloru na początku kolejnych linii */
int d2_wrap(int font, float size, const char *s, float maxw, char out[][256], int maxl) {
  int n = 0;
  char line[256] = "", act[4] = "";
  const char *p = s;
  while (*p && n < maxl) {
    if (*p == '\n') { snprintf(out[n++], 256, "%s", line); snprintf(line, sizeof line, "%s", act); p++; continue; }
    const char *q = p;
    while (*q && *q != ' ' && *q != '\n') q++;
    char word[256];
    int wl = (int)(q - p) < 255 ? (int)(q - p) : 255;
    memcpy(word, p, wl);
    word[wl] = 0;
    for (const char *t = word; *t; t++)
      if (t[0] == '{' && t[1] && t[2] == '}') { if (t[1] == '-') act[0] = 0; else { act[0] = '{'; act[1] = t[1]; act[2] = '}'; act[3] = 0; } }
    char cand[512];
    snprintf(cand, sizeof cand, "%s%s%s", line, (line[0] && strcmp(line, act)) ? " " : "", word);
    if (d2_text_w(font, size, cand) > maxw && line[0] && strcmp(line, act)) {
      snprintf(out[n++], 256, "%s", line);
      snprintf(line, sizeof line, "%s%s", act, word);
    } else snprintf(line, sizeof line, "%s", cand);
    p = q;
    while (*p == ' ') p++;
  }
  if (line[0] && n < maxl && strcmp(line, act)) snprintf(out[n++], 256, "%s", line);
  return n;
}
