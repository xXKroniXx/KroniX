/* KroniX Engine — Renderer: shadery świata (światła per-piksel, mgła, mokra nawierzchnia, emisja), niebo,
 * deszcz, poświaty, HDR + MSAA + bloom + tonemapping (ACES) + korekcja barw, render do tekstur. */
#include "render.h"
#include "glapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RCfg g_cfg = {4, 1, 100, 16, 1, 75};
int g_winW = 1280, g_winH = 720;
float g_time;
M4 g_view, g_proj, g_vp;
V3 g_camPos;

/* ---------------------------------------------------------------- shadery */
static const char *VS_WORLD =
  "#version 330 core\n"
  "layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNrm; layout(location=2) in vec3 aUV;\n"
  "layout(location=3) in vec4 aCol; layout(location=4) in vec2 aBF;\n"
  "uniform mat4 uVP; uniform mat4 uModel; uniform mat4 uBones[24]; uniform float uTime;\n"
  "out vec3 vPos; out vec3 vNrm; out vec3 vUV; out vec4 vCol; flat out int vFlags;\n"
  "void main(){\n"
  "  mat4 M = uModel * uBones[int(aBF.x)];\n"
  "  vec3 p = aPos; int fl = int(aBF.y);\n"
  "  if ((fl & 16) != 0) p.x += sin(uTime*1.3 + aPos.y*2.0 + aPos.z) * 0.02 * aPos.y;\n"
  "  vec4 wp = M * vec4(p, 1.0);\n"
  "  vPos = wp.xyz; vNrm = mat3(M) * aNrm; vUV = aUV;\n"
  "  vCol = vec4(pow(aCol.rgb, vec3(2.2)), aCol.a); vFlags = fl;\n"
  "  gl_Position = uVP * wp;\n"
  "}\n";

static const char *FS_WORLD =
  "#version 330 core\n"
  "in vec3 vPos; in vec3 vNrm; in vec3 vUV; in vec4 vCol; flat in int vFlags;\n"
  "uniform sampler2DArray uTex; uniform vec3 uCam; uniform int uNL;\n"
  "uniform vec4 uLPos[16]; uniform vec4 uLCol[16];\n"
  "uniform vec3 uAmbSky, uAmbGround, uSunDir, uSunCol, uFogCol; uniform float uFogDens, uWet, uTime, uExposure;\n"
  "uniform vec4 uTint; uniform int uViewModel;\n"
  "out vec4 frag;\n"
  "float hash(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233)))*43758.5453); }\n"
  "void main(){\n"
  "  vec3 uv = vUV;\n"
  "  if ((vFlags & 8) != 0) uv.xy += vec2(uTime*0.03, uTime*0.017);\n"
  "  vec4 t = texture(uTex, uv);\n"
  "  if ((vFlags & 1) != 0 && t.a < 0.5) discard;\n"
  "  vec3 alb = pow(t.rgb, vec3(2.2)) * vCol.rgb * uTint.rgb;\n"
  "  float gloss = ((vFlags & 1) != 0) ? 0.15 : t.a;\n"
  "  vec3 N = normalize(vNrm); if (!gl_FrontFacing) N = -N;\n"
  "  vec3 V = normalize(uCam - vPos);\n"
  "  if ((vFlags & 2) != 0) { frag = vec4(alb * (1.0 + vCol.a*6.0), 1.0); return; }\n"
  "  if ((vFlags & 32) != 0) { float f = 0.75 + 0.25*sin(uTime*13.0+vPos.x*5.0)*sin(uTime*7.3+vPos.z*3.0); frag = vec4(alb*4.0*f, 1.0); return; }\n"
  "  float wet = uWet * smoothstep(0.6, 0.95, N.y) * (uViewModel==1 ? 0.0 : 1.0);\n"
  "  alb *= mix(1.0, 0.55, wet);\n"
  "  gloss = mix(gloss, max(gloss, 0.85), wet * (0.55 + 0.45*t.a));\n"
  "  if ((vFlags & 8) != 0) gloss = 0.95;\n"
  "  float shin = 8.0 + gloss*gloss*250.0;\n"
  "  float specAmt = gloss*gloss*1.6 + 0.02;\n"
  "  vec3 amb = mix(uAmbGround, uAmbSky, N.y*0.5+0.5);\n"
  "  vec3 col = alb * amb;\n"
  "  float nd = max(dot(N, uSunDir), 0.0);\n"
  "  vec3 H = normalize(uSunDir + V);\n"
  "  col += uSunCol * (alb*nd + specAmt*pow(max(dot(N,H),0.0), shin)*nd);\n"
  "  for (int i=0;i<16;i++){ if (i>=uNL) break;\n"
  "    vec3 L = uLPos[i].xyz - vPos; float d = length(L); float r = uLPos[i].w;\n"
  "    if (d >= r) continue;\n"
  "    L /= d; float a = 1.0 - d/r; a *= a; a /= (1.0 + d*d*0.09);\n"
  "    float ln = max(dot(N, L), 0.0);\n"
  "    vec3 h = normalize(L + V);\n"
  "    col += uLCol[i].rgb * a * (alb*ln + specAmt*pow(max(dot(N,h),0.0), shin)*(ln>0.0?1.0:0.0) * 2.0);\n"
  "  }\n"
  "  float fres = pow(1.0 - max(dot(N, V), 0.0), 3.0);\n"
  "  col += uAmbSky * fres * 0.9 * (0.4 + alb);\n"
  "  col += alb * vCol.a * 4.0;\n"
  "  if ((vFlags & 4) == 0) { float dist = length(uCam - vPos); float f = 1.0 - exp(-pow(dist*uFogDens, 1.4)); col = mix(col, uFogCol, f); }\n"
  "  else { float dist = length(uCam - vPos); float f = 1.0 - exp(-dist*uFogDens*0.35); col = mix(col, uFogCol, f*0.85); }\n"
  "  if (uViewModel == 2) col = pow(col / (col + 0.6) * 1.5, vec3(1.0/2.2));\n"
  "  frag = vec4(col, 1.0);\n"
  "}\n";

static const char *VS_FULL =
  "#version 330 core\n"
  "out vec2 vUV;\n"
  "void main(){ vec2 p = vec2((gl_VertexID<<1)&2, gl_VertexID&2); vUV = p; gl_Position = vec4(p*2.0-1.0, 0.0, 1.0); }\n";

static const char *FS_SKY =
  "#version 330 core\n"
  "in vec2 vUV; uniform mat4 uInvVP; uniform vec3 uTop, uHor, uGlow, uMoonDir; uniform float uMoon, uStars, uTime;\n"
  "out vec4 frag;\n"
  "float hash(vec3 p){ return fract(sin(dot(p, vec3(127.1,311.7,74.7)))*43758.5453); }\n"
  "void main(){\n"
  "  vec4 a = uInvVP * vec4(vUV*2.0-1.0, 1.0, 1.0); vec3 d = normalize(a.xyz/a.w);\n"
  "  float h = clamp(d.y, -0.2, 1.0);\n"
  "  vec3 col = mix(uHor, uTop, pow(max(h,0.0), 0.55));\n"
  "  col += uGlow * exp(-max(h,0.0)*9.0) * 0.9;\n"
  "  if (h < 0.0) col = mix(uHor, uHor*0.6, -h*4.0);\n"
  "  vec3 s = floor(d*180.0); float st = hash(s);\n"
  "  if (st > 0.9965 && h > 0.05) col += vec3(0.9,0.9,1.0) * (0.5+0.5*sin(uTime*2.0+st*80.0)) * uStars * smoothstep(0.05,0.4,h);\n"
  "  float md = dot(d, uMoonDir);\n"
  "  col += vec3(1.0,0.97,0.88) * smoothstep(0.9993, 0.9996, md) * 3.0 * uMoon;\n"
  "  col += vec3(0.6,0.65,0.8) * pow(max(md,0.0), 400.0) * 0.8 * uMoon;\n"
  "  frag = vec4(col, 1.0);\n"
  "}\n";

static const char *VS_GLOW =
  "#version 330 core\n"
  "layout(location=0) in vec3 aPos; layout(location=1) in vec2 aCorner; layout(location=2) in vec4 aCol;\n"
  "uniform mat4 uVP; uniform vec3 uRight, uUp;\n"
  "out vec2 vC; out vec4 vCol;\n"
  "void main(){ vC = aCorner; vCol = aCol; vec3 p = aPos + (uRight*aCorner.x + uUp*aCorner.y) * aCol.a; gl_Position = uVP*vec4(p,1.0); }\n";
static const char *FS_GLOW =
  "#version 330 core\n"
  "in vec2 vC; in vec4 vCol; out vec4 frag;\n"
  "void main(){ float r = length(vC); float a = exp(-r*r*4.5) - 0.011; if (a <= 0.0) discard; frag = vec4(vCol.rgb * a, 1.0); }\n";

static const char *VS_RAIN =
  "#version 330 core\n"
  "layout(location=0) in vec4 aSeed;\n"
  "uniform mat4 uVP; uniform vec3 uCam; uniform float uTime;\n"
  "out float vA;\n"
  "void main(){\n"
  "  int corner = gl_VertexID % 2;\n"
  "  float box = 14.0;\n"
  "  vec3 p = aSeed.xyz * box;\n"
  "  float fall = uTime * (9.0 + aSeed.w*3.0);\n"
  "  p.y = mod(aSeed.y*box*0.5 - fall, 8.0) - 1.0;\n"
  "  p.x = uCam.x + mod(p.x - uCam.x + box*0.5, box) - box*0.5 + fall*0.06;\n"
  "  p.z = uCam.z + mod(p.z - uCam.z + box*0.5, box) - box*0.5;\n"
  "  p.y += uCam.y - 3.0;\n"
  "  if (corner == 1) { p.y += 0.22; p.x -= 0.014; }\n"
  "  vA = corner == 1 ? 0.0 : 1.0;\n"
  "  gl_Position = uVP * vec4(p, 1.0);\n"
  "}\n";
static const char *FS_RAIN =
  "#version 330 core\n"
  "in float vA; uniform float uInt; out vec4 frag;\n"
  "void main(){ frag = vec4(vec3(0.35,0.4,0.5) * (0.25 + vA*0.6) * uInt, 1.0); }\n";

static const char *FS_BRIGHT =
  "#version 330 core\n"
  "in vec2 vUV; uniform sampler2D uSrc; uniform float uExp; out vec4 frag;\n"
  "void main(){ vec2 px = 1.0/vec2(textureSize(uSrc,0));\n"
  "  vec3 c = (texture(uSrc, vUV+px*vec2(-0.5,-0.5)).rgb + texture(uSrc, vUV+px*vec2(0.5,-0.5)).rgb + texture(uSrc, vUV+px*vec2(-0.5,0.5)).rgb + texture(uSrc, vUV+px*vec2(0.5,0.5)).rgb)*0.25*uExp;\n"
  "  float l = max(c.r, max(c.g, c.b)); float k = max(l - 1.0, 0.0) / max(l, 1e-4);\n"
  "  frag = vec4(min(c * k, vec3(40.0)), 1.0); }\n";
static const char *FS_BLUR =
  "#version 330 core\n"
  "in vec2 vUV; uniform sampler2D uSrc; uniform vec2 uDir; out vec4 frag;\n"
  "void main(){ vec2 px = uDir/vec2(textureSize(uSrc,0));\n"
  "  vec3 c = texture(uSrc, vUV).rgb*0.2270270;\n"
  "  c += (texture(uSrc, vUV+px*1.3846153).rgb + texture(uSrc, vUV-px*1.3846153).rgb)*0.3162162;\n"
  "  c += (texture(uSrc, vUV+px*3.2307692).rgb + texture(uSrc, vUV-px*3.2307692).rgb)*0.0702702;\n"
  "  frag = vec4(c, 1.0); }\n";
static const char *FS_DOWN =
  "#version 330 core\n"
  "in vec2 vUV; uniform sampler2D uSrc; out vec4 frag;\n"
  "void main(){ frag = vec4(texture(uSrc, vUV).rgb, 1.0); }\n";
static const char *FS_FINAL =
  "#version 330 core\n"
  "in vec2 vUV; uniform sampler2D uSrc; uniform sampler2D uB1; uniform sampler2D uB2; uniform sampler2D uB3;\n"
  "uniform float uExp, uBloom, uTime, uSat, uWarm, uVig, uFlash; uniform vec2 uRes;\n"
  "out vec4 frag;\n"
  "vec3 aces(vec3 x){ return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14), 0.0, 1.0); }\n"
  "float hash(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233)))*43758.5453); }\n"
  "void main(){\n"
  "  vec3 c = texture(uSrc, vUV).rgb * uExp;\n"
  "  vec3 b = texture(uB1, vUV).rgb*0.5 + texture(uB2, vUV).rgb*0.35 + texture(uB3, vUV).rgb*0.3;\n"
  "  c += b * uBloom;\n"
  "  c = aces(c);\n"
  "  float l = dot(c, vec3(0.299,0.587,0.114));\n"
  "  c = mix(vec3(l), c, uSat);\n"
  "  c *= mix(vec3(1.0), vec3(1.06,1.0,0.88), uWarm);\n"
  "  c = mix(c, c*vec3(0.92,0.97,1.08), (1.0-l)*0.35*uWarm);\n"
  "  vec2 q = vUV - 0.5; float v = 1.0 - dot(q,q)*uVig*1.6; c *= clamp(v, 0.0, 1.0);\n"
  "  c += uFlash;\n"
  "  c = pow(clamp(c,0.0,1.0), vec3(1.0/2.2));\n"
  "  c += (hash(vUV*uRes + fract(uTime)*91.0) - 0.5) * 0.028;\n"
  "  frag = vec4(c, 1.0);\n"
  "}\n";

char g_glError[1024];
static void gl_err(const char *what, const char *name, const char *log) {
  fprintf(stderr, "%s %s: %s\n", what, name, log);
  if (!g_glError[0]) snprintf(g_glError, sizeof g_glError, "%s %s:\n%s", what, name, log);
}

static unsigned compile(const char *vs, const char *fs, const char *name) {
  unsigned p = glCreateProgram();
  const char *src[2] = {vs, fs};
  GLenum ty[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
  for (int i = 0; i < 2; i++) {
    unsigned s = glCreateShader(ty[i]);
    glShaderSource(s, 1, &src[i], NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
      char log[2048];
      glGetShaderInfoLog(s, sizeof log, NULL, log);
      gl_err(i ? "shader fs" : "shader vs", name, log);
    }
    glAttachShader(p, s);
    glDeleteShader(s);
  }
  glLinkProgram(p);
  GLint ok = 0;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[2048];
    glGetProgramInfoLog(p, sizeof log, NULL, log);
    gl_err("link", name, log);
  }
  return p;
}

static struct {
  unsigned world, sky, glow, rain, bright, blur, down, final;
  /* uniformy świata */
  int wVP, wModel, wBones, wTime, wTex, wCam, wNL, wLPos, wLCol, wAmbSky, wAmbGround, wSunDir, wSunCol, wFogCol, wFogDens, wWet, wExp, wTint, wVM;
  unsigned worldTex;
  /* bufory */
  unsigned msFbo, msColor, msDepth;
  unsigned hdrFbo, hdrTex, hdrDepth;
  unsigned bFbo[6], bTex[6];
  int rw, rh, bw[3], bh[3];
  unsigned emptyVao;
  /* poświaty */
  unsigned glowVao, glowVbo;
  float glowBuf[6 * 9 * 512];
  int nglow;
  /* deszcz */
  unsigned rainVao, rainVbo;
  int ready;
  REnv env;
  float grade[4];
  int msaaActive;
} R;

typedef struct { float x, y, z, r, cr, cg, cb, d2; } LightRec;
static LightRec lightsAll[512];
static int nLightsAll;

static int uloc(unsigned p, const char *n) { return glGetUniformLocation(p, n); }

static void destroy_targets(void) {
  if (R.msFbo) { glDeleteFramebuffers(1, &R.msFbo); glDeleteRenderbuffers(1, &R.msColor); glDeleteRenderbuffers(1, &R.msDepth); R.msFbo = 0; }
  if (R.hdrFbo) { glDeleteFramebuffers(1, &R.hdrFbo); glDeleteTextures(1, &R.hdrTex); glDeleteRenderbuffers(1, &R.hdrDepth); R.hdrFbo = 0; }
  for (int i = 0; i < 6; i++) if (R.bFbo[i]) { glDeleteFramebuffers(1, &R.bFbo[i]); glDeleteTextures(1, &R.bTex[i]); R.bFbo[i] = 0; }
}

static unsigned make_tex(int w, int h, GLenum ifmt, GLenum fmt, GLenum type, int filter) {
  unsigned t;
  glGenTextures(1, &t);
  glBindTexture(GL_TEXTURE_2D, t);
  glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, fmt, type, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return t;
}

static void create_targets(void) {
  destroy_targets();
  int sc = g_cfg.scale < 40 ? 40 : g_cfg.scale > 100 ? 100 : g_cfg.scale;
  R.rw = g_winW * sc / 100; R.rh = g_winH * sc / 100;
  if (R.rw < 64) R.rw = 64;
  if (R.rh < 64) R.rh = 64;
  /* HDR (rozwiązany) */
  glGenFramebuffers(1, &R.hdrFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, R.hdrFbo);
  R.hdrTex = make_tex(R.rw, R.rh, GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, R.hdrTex, 0);
  glGenRenderbuffers(1, &R.hdrDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, R.hdrDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, R.rw, R.rh);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, R.hdrDepth);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) fprintf(stderr, "HDR FBO niekompletny\n");
  /* MSAA */
  R.msaaActive = 0;
  if (g_cfg.msaa > 1) {
    GLint maxs = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxs);
    int s = g_cfg.msaa > maxs ? maxs : g_cfg.msaa;
    if (s > 1) {
      glGenFramebuffers(1, &R.msFbo);
      glBindFramebuffer(GL_FRAMEBUFFER, R.msFbo);
      glGenRenderbuffers(1, &R.msColor);
      glBindRenderbuffer(GL_RENDERBUFFER, R.msColor);
      glRenderbufferStorageMultisample(GL_RENDERBUFFER, s, GL_RGBA16F, R.rw, R.rh);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, R.msColor);
      glGenRenderbuffers(1, &R.msDepth);
      glBindRenderbuffer(GL_RENDERBUFFER, R.msDepth);
      glRenderbufferStorageMultisample(GL_RENDERBUFFER, s, GL_DEPTH_COMPONENT24, R.rw, R.rh);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, R.msDepth);
      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) R.msaaActive = 1;
      else { glDeleteFramebuffers(1, &R.msFbo); R.msFbo = 0; }
    }
  }
  /* bloom: 3 poziomy x 2 (ping-pong) */
  int w = R.rw / 2, h = R.rh / 2;
  for (int l = 0; l < 3; l++) {
    if (w < 8) w = 8;
    if (h < 8) h = 8;
    R.bw[l] = w; R.bh[l] = h;
    for (int k = 0; k < 2; k++) {
      int i = l * 2 + k;
      glGenFramebuffers(1, &R.bFbo[i]);
      glBindFramebuffer(GL_FRAMEBUFFER, R.bFbo[i]);
      R.bTex[i] = make_tex(w, h, GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, GL_LINEAR);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, R.bTex[i], 0);
    }
    w /= 2; h /= 2;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void r_resize(int w, int h) {
  if (w < 64) w = 64;
  if (h < 64) h = 64;
  g_winW = w; g_winH = h;
  float sx = (float)w / UI_W, sy = (float)h / UI_H;
  g_uiScale = sx < sy ? sx : sy;
  g_uiOffX = (w - UI_W * g_uiScale) * 0.5f;
  g_uiOffY = (h - UI_H * g_uiScale) * 0.5f;
  if (R.ready) create_targets();
}

int r_init(void) {
  R.world = compile(VS_WORLD, FS_WORLD, "world");
  R.sky = compile(VS_FULL, FS_SKY, "sky");
  R.glow = compile(VS_GLOW, FS_GLOW, "glow");
  R.rain = compile(VS_RAIN, FS_RAIN, "rain");
  R.bright = compile(VS_FULL, FS_BRIGHT, "bright");
  R.blur = compile(VS_FULL, FS_BLUR, "blur");
  R.down = compile(VS_FULL, FS_DOWN, "down");
  R.final = compile(VS_FULL, FS_FINAL, "final");
  unsigned w = R.world;
  R.wVP = uloc(w, "uVP"); R.wModel = uloc(w, "uModel"); R.wBones = uloc(w, "uBones"); R.wTime = uloc(w, "uTime");
  R.wTex = uloc(w, "uTex"); R.wCam = uloc(w, "uCam"); R.wNL = uloc(w, "uNL"); R.wLPos = uloc(w, "uLPos"); R.wLCol = uloc(w, "uLCol");
  R.wAmbSky = uloc(w, "uAmbSky"); R.wAmbGround = uloc(w, "uAmbGround"); R.wSunDir = uloc(w, "uSunDir"); R.wSunCol = uloc(w, "uSunCol");
  R.wFogCol = uloc(w, "uFogCol"); R.wFogDens = uloc(w, "uFogDens"); R.wWet = uloc(w, "uWet"); R.wExp = uloc(w, "uExposure");
  R.wTint = uloc(w, "uTint"); R.wVM = uloc(w, "uViewModel");
  glGenVertexArrays(1, &R.emptyVao);
  /* poświaty */
  glGenVertexArrays(1, &R.glowVao);
  glGenBuffers(1, &R.glowVbo);
  glBindVertexArray(R.glowVao);
  glBindBuffer(GL_ARRAY_BUFFER, R.glowVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof R.glowBuf, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * 4, (void *)0);
  glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 9 * 4, (void *)12);
  glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 9 * 4, (void *)20);
  /* deszcz: 2 wierzchołki na kroplę */
  enum { NDROP = 2400 };
  static float seeds[NDROP * 2 * 4];
  unsigned s = 12345;
  for (int i = 0; i < NDROP; i++) {
    float v[4];
    for (int k = 0; k < 4; k++) { s = s * 1664525u + 1013904223u; v[k] = (s >> 8) / 16777216.0f; }
    for (int c = 0; c < 2; c++) memcpy(&seeds[(i * 2 + c) * 4], v, sizeof v);
  }
  glGenVertexArrays(1, &R.rainVao);
  glGenBuffers(1, &R.rainVbo);
  glBindVertexArray(R.rainVao);
  glBindBuffer(GL_ARRAY_BUFFER, R.rainVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof seeds, seeds, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0); glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 16, (void *)0);
  glBindVertexArray(0);
  R.ready = 1;
  R.grade[0] = 0.92f; R.grade[1] = 1.0f; R.grade[2] = 1.0f; R.grade[3] = 0;
  create_targets();
  d2_tex_init();
  return 0;
}

void r_set_world_textures(unsigned arr) { R.worldTex = arr; }

unsigned r_texarray(int layers, const uint8_t *rgba) {
  unsigned t;
  glGenTextures(1, &t);
  glBindTexture(GL_TEXTURE_2D_ARRAY, t);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_SRGB8_ALPHA8, TEXSZ, TEXSZ, layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  float an = 0;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &an);
  glGetError();
  if (an > 1) { glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY, an > 8 ? 8 : an); glGetError(); }
  return t;
}

unsigned r_tex2d(int w, int h, const uint8_t *rgba, int filter) {
  unsigned t;
  glGenTextures(1, &t);
  glBindTexture(GL_TEXTURE_2D, t);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return t;
}

/* ---------------------------------------------------------------- klatka */
static M4 g_invVP;
static M4 g_camToWorld;
M4 r_cam_to_world(void) { return g_camToWorld; }
static float camRight[3], camUp[3];
static int vmActive;

static M4 m4_inverse(M4 m) {
  float *a = m.m, inv[16];
  inv[0] = a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15] + a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
  inv[4] = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15] - a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
  inv[8] = a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15] + a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
  inv[12] = -a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14] - a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
  inv[1] = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15] - a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
  inv[5] = a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15] + a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
  inv[9] = -a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15] - a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
  inv[13] = a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14] + a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
  inv[2] = a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15] + a[5] * a[3] * a[14] + a[13] * a[2] * a[7] - a[13] * a[3] * a[6];
  inv[6] = -a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15] - a[4] * a[3] * a[14] - a[12] * a[2] * a[7] + a[12] * a[3] * a[6];
  inv[10] = a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15] + a[4] * a[3] * a[13] + a[12] * a[1] * a[7] - a[12] * a[3] * a[5];
  inv[14] = -a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14] - a[4] * a[2] * a[13] - a[12] * a[1] * a[6] + a[12] * a[2] * a[5];
  inv[3] = -a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11] - a[5] * a[3] * a[10] - a[9] * a[2] * a[7] + a[9] * a[3] * a[6];
  inv[7] = a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11] + a[4] * a[3] * a[10] + a[8] * a[2] * a[7] - a[8] * a[3] * a[6];
  inv[11] = -a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11] - a[4] * a[3] * a[9] - a[8] * a[1] * a[7] + a[8] * a[3] * a[5];
  inv[15] = a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10] + a[4] * a[2] * a[9] + a[8] * a[1] * a[6] - a[8] * a[2] * a[5];
  float det = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
  M4 r;
  if (fabsf(det) < 1e-12f) return m4_identity();
  for (int i = 0; i < 16; i++) r.m[i] = inv[i] / det;
  return r;
}

static void world_uniforms_env(void) {
  REnv *e = &R.env;
  glUniform3f(R.wAmbSky, e->ambSky[0], e->ambSky[1], e->ambSky[2]);
  glUniform3f(R.wAmbGround, e->ambGround[0], e->ambGround[1], e->ambGround[2]);
  V3 sd = v3norm(v3(e->sunDir[0], e->sunDir[1], e->sunDir[2]));
  glUniform3f(R.wSunDir, sd.x, sd.y, sd.z);
  glUniform3f(R.wSunCol, e->sunCol[0], e->sunCol[1], e->sunCol[2]);
  glUniform3f(R.wFogCol, e->fogCol[0], e->fogCol[1], e->fogCol[2]);
  glUniform1f(R.wFogDens, e->fogDensity);
  glUniform1f(R.wWet, e->wet);
  glUniform1f(R.wTime, g_time);
  glUniform1f(R.wExp, e->exposure);
}

void r_frame_begin(const RCam *cam, const REnv *env) {
  R.env = *env;
  if (R.env.exposure <= 0) R.env.exposure = 1;
  g_camPos = cam->pos;
  float aspect = (float)R.rw / R.rh;
  g_proj = m4_perspective(cam->fov * 3.14159265f / 180.0f, aspect, 0.03f, 260.0f);
  /* kamera: przód f=(sin yaw, 0, cos yaw), pitch w górę dodatni */
  M4 rot = m4_mul(m4_rotx(-cam->pitch), m4_roty(3.14159265f - cam->yaw));
  g_view = m4_mul(rot, m4_translate(-cam->pos.x, -cam->pos.y, -cam->pos.z));
  g_vp = m4_mul(g_proj, g_view);
  g_invVP = m4_inverse(g_vp);
  g_camToWorld = m4_inverse(g_view);
  camRight[0] = g_view.m[0]; camRight[1] = g_view.m[4]; camRight[2] = g_view.m[8];
  camUp[0] = g_view.m[1]; camUp[1] = g_view.m[5]; camUp[2] = g_view.m[9];
  nLightsAll = 0;
  R.nglow = 0;
  vmActive = 0;

  glBindFramebuffer(GL_FRAMEBUFFER, R.msaaActive ? R.msFbo : R.hdrFbo);
  glViewport(0, 0, R.rw, R.rh);
  glDisable(GL_SCISSOR_TEST);
  glDepthMask(GL_TRUE);
  glClearColor(env->fogCol[0], env->fogCol[1], env->fogCol[2], 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_BLEND);
  if (!env->interior) {
    /* niebo */
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glUseProgram(R.sky);
    glUniformMatrix4fv(uloc(R.sky, "uInvVP"), 1, GL_FALSE, g_invVP.m);
    glUniform3f(uloc(R.sky, "uTop"), env->skyTop[0], env->skyTop[1], env->skyTop[2]);
    glUniform3f(uloc(R.sky, "uHor"), env->skyHorizon[0], env->skyHorizon[1], env->skyHorizon[2]);
    glUniform3f(uloc(R.sky, "uGlow"), env->skyGlow[0], env->skyGlow[1], env->skyGlow[2]);
    V3 md = v3norm(v3(env->sunDir[0], env->sunDir[1] + 0.25f, env->sunDir[2]));
    glUniform3f(uloc(R.sky, "uMoonDir"), md.x, md.y, md.z);
    glUniform1f(uloc(R.sky, "uMoon"), env->moon);
    glUniform1f(uloc(R.sky, "uStars"), env->stars);
    glUniform1f(uloc(R.sky, "uTime"), g_time);
    glBindVertexArray(R.emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDepthMask(GL_TRUE);
  }
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glUseProgram(R.world);
  glUniformMatrix4fv(R.wVP, 1, GL_FALSE, g_vp.m);
  glUniform3f(R.wCam, cam->pos.x, cam->pos.y, cam->pos.z);
  glUniform1i(R.wTex, 0);
  glUniform1i(R.wVM, 0);
  world_uniforms_env();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, R.worldTex);
  M4 id = m4_identity();
  glUniformMatrix4fv(R.wBones, 1, GL_FALSE, id.m);
  glUniform1i(R.wNL, 0);
}

void r_light(float x, float y, float z, float r, float g, float b, float radius) {
  if (nLightsAll >= 512) return;
  LightRec *l = &lightsAll[nLightsAll++];
  l->x = x; l->y = y; l->z = z; l->r = radius; l->cr = r; l->cg = g; l->cb = b;
  float dx = x - g_camPos.x, dy = y - g_camPos.y, dz = z - g_camPos.z;
  l->d2 = dx * dx + dy * dy + dz * dz;
}

static int light_cmp(const void *a, const void *b) {
  const LightRec *x = (const LightRec *)a, *y = (const LightRec *)b;
  /* ważność: odległość pomniejszona o zasięg i jasność */
  float wx = sqrtf(x->d2) - x->r * 1.5f, wy = sqrtf(y->d2) - y->r * 1.5f;
  return wx < wy ? -1 : wx > wy;
}

void r_lights_commit(void) {
  qsort(lightsAll, nLightsAll, sizeof(LightRec), light_cmp);
  int n = nLightsAll < g_cfg.maxLights ? nLightsAll : g_cfg.maxLights;
  if (n > 16) n = 16;
  float pos[64], col[64];
  for (int i = 0; i < n; i++) {
    pos[i * 4] = lightsAll[i].x; pos[i * 4 + 1] = lightsAll[i].y; pos[i * 4 + 2] = lightsAll[i].z; pos[i * 4 + 3] = lightsAll[i].r;
    col[i * 4] = lightsAll[i].cr; col[i * 4 + 1] = lightsAll[i].cg; col[i * 4 + 2] = lightsAll[i].cb; col[i * 4 + 3] = 1;
  }
  glUseProgram(R.world);
  glUniform1i(R.wNL, n);
  if (n) {
    glUniform4fv(R.wLPos, n, pos);
    glUniform4fv(R.wLCol, n, col);
  }
}

static void set_tint(uint32_t tint) {
  float a = ((tint >> 24) & 255) / 255.0f;
  if (tint == 0) { glUniform4f(R.wTint, 1, 1, 1, 1); return; }
  glUniform4f(R.wTint, ((tint >> 16) & 255) / 255.0f, ((tint >> 8) & 255) / 255.0f, (tint & 255) / 255.0f, a);
}

void r_draw(const GMesh *m, const M4 *model, uint32_t tint) {
  if (!m || !m->n) return;
  M4 id = m4_identity();
  glUniformMatrix4fv(R.wModel, 1, GL_FALSE, model ? model->m : id.m);
  set_tint(tint);
  glBindVertexArray(m->vao);
  glDrawArrays(GL_TRIANGLES, 0, m->n);
}

void r_draw_bones(const GMesh *m, const M4 *model, const M4 *bones, int nb, uint32_t tint) {
  if (!m || !m->n) return;
  glUniformMatrix4fv(R.wBones, nb > 24 ? 24 : nb, GL_FALSE, bones[0].m);
  r_draw(m, model, tint);
  M4 id = m4_identity();
  glUniformMatrix4fv(R.wBones, 1, GL_FALSE, id.m);
}

void r_glow(float x, float y, float z, float size, float r, float g, float b) {
  if (R.nglow >= 512) return;
  static const float C[6][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {1, 1}, {-1, 1}};
  float *v = &R.glowBuf[R.nglow * 6 * 9];
  for (int i = 0; i < 6; i++) {
    v[i * 9 + 0] = x; v[i * 9 + 1] = y; v[i * 9 + 2] = z;
    v[i * 9 + 3] = C[i][0]; v[i * 9 + 4] = C[i][1];
    v[i * 9 + 5] = r; v[i * 9 + 6] = g; v[i * 9 + 7] = b; v[i * 9 + 8] = size;
  }
  R.nglow++;
}

void r_glow_flush(void) {
  if (!R.nglow) return;
  glUseProgram(R.glow);
  glUniformMatrix4fv(uloc(R.glow, "uVP"), 1, GL_FALSE, g_vp.m);
  glUniform3f(uloc(R.glow, "uRight"), camRight[0], camRight[1], camRight[2]);
  glUniform3f(uloc(R.glow, "uUp"), camUp[0], camUp[1], camUp[2]);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glDepthMask(GL_FALSE);
  glDisable(GL_CULL_FACE);
  glBindVertexArray(R.glowVao);
  glBindBuffer(GL_ARRAY_BUFFER, R.glowVbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, R.nglow * 6 * 9 * 4, R.glowBuf);
  glDrawArrays(GL_TRIANGLES, 0, R.nglow * 6);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  R.nglow = 0;
  glUseProgram(R.world);
}

void r_rain(float intensity) {
  if (intensity <= 0) return;
  glUseProgram(R.rain);
  glUniformMatrix4fv(uloc(R.rain, "uVP"), 1, GL_FALSE, g_vp.m);
  glUniform3f(uloc(R.rain, "uCam"), g_camPos.x, g_camPos.y, g_camPos.z);
  glUniform1f(uloc(R.rain, "uTime"), g_time);
  glUniform1f(uloc(R.rain, "uInt"), intensity);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glDepthMask(GL_FALSE);
  glBindVertexArray(R.rainVao);
  glDrawArrays(GL_LINES, 0, (int)(2400 * 2 * (intensity > 1 ? 1 : intensity)) & ~1);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  glUseProgram(R.world);
}

void r_viewmodel_begin(float fov) {
  r_glow_flush();
  glClear(GL_DEPTH_BUFFER_BIT);
  float aspect = (float)R.rw / R.rh;
  M4 proj = m4_perspective(fov * 3.14159265f / 180.0f, aspect, 0.01f, 10.0f);
  glUseProgram(R.world);
  /* broń w przestrzeni świata (model = kamera->świat * lokalna), osobna projekcja */
  M4 vp = m4_mul(proj, g_view);
  glUniformMatrix4fv(R.wVP, 1, GL_FALSE, vp.m);
  vmActive = 1;
  glUniform1i(R.wVM, 1);
}

void r_viewmodel_end(void) {
  glUseProgram(R.world);
  glUniformMatrix4fv(R.wVP, 1, GL_FALSE, g_vp.m);
  glUniform1i(R.wVM, 0);
  vmActive = 0;
}

static void fullscreen(void) {
  glBindVertexArray(R.emptyVao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void r_grade(float sat, float warm, float vignette, float flash) {
  R.grade[0] = sat; R.grade[1] = warm; R.grade[2] = vignette; R.grade[3] = flash;
}

void r_frame_end(void) {
  r_glow_flush();
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  if (R.msaaActive) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, R.msFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, R.hdrFbo);
    glBlitFramebuffer(0, 0, R.rw, R.rh, 0, 0, R.rw, R.rh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  }
  glActiveTexture(GL_TEXTURE0);
  if (g_cfg.bloom) {
    /* jasne fragmenty -> poziom 0 */
    glBindFramebuffer(GL_FRAMEBUFFER, R.bFbo[0]);
    glViewport(0, 0, R.bw[0], R.bh[0]);
    glUseProgram(R.bright);
    glUniform1i(uloc(R.bright, "uSrc"), 0);
    glUniform1f(uloc(R.bright, "uExp"), R.env.exposure);
    glBindTexture(GL_TEXTURE_2D, R.hdrTex);
    fullscreen();
    for (int l = 0; l < 3; l++) {
      if (l > 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, R.bFbo[l * 2]);
        glViewport(0, 0, R.bw[l], R.bh[l]);
        glUseProgram(R.down);
        glUniform1i(uloc(R.down, "uSrc"), 0);
        glBindTexture(GL_TEXTURE_2D, R.bTex[(l - 1) * 2]);
        fullscreen();
      }
      glUseProgram(R.blur);
      glUniform1i(uloc(R.blur, "uSrc"), 0);
      glViewport(0, 0, R.bw[l], R.bh[l]);
      for (int it = 0; it < 2; it++) {
        glBindFramebuffer(GL_FRAMEBUFFER, R.bFbo[l * 2 + 1]);
        glUniform2f(uloc(R.blur, "uDir"), 1, 0);
        glBindTexture(GL_TEXTURE_2D, R.bTex[l * 2]);
        fullscreen();
        glBindFramebuffer(GL_FRAMEBUFFER, R.bFbo[l * 2]);
        glUniform2f(uloc(R.blur, "uDir"), 0, 1);
        glBindTexture(GL_TEXTURE_2D, R.bTex[l * 2 + 1]);
        fullscreen();
      }
    }
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, g_winW, g_winH);
  glUseProgram(R.final);
  glUniform1i(uloc(R.final, "uSrc"), 0);
  glUniform1i(uloc(R.final, "uB1"), 1);
  glUniform1i(uloc(R.final, "uB2"), 2);
  glUniform1i(uloc(R.final, "uB3"), 3);
  glUniform1f(uloc(R.final, "uExp"), R.env.exposure);
  glUniform1f(uloc(R.final, "uBloom"), g_cfg.bloom ? 0.55f : 0.0f);
  glUniform1f(uloc(R.final, "uTime"), g_time);
  glUniform1f(uloc(R.final, "uSat"), R.grade[0]);
  glUniform1f(uloc(R.final, "uWarm"), R.grade[1]);
  glUniform1f(uloc(R.final, "uVig"), R.grade[2]);
  glUniform1f(uloc(R.final, "uFlash"), R.grade[3]);
  glUniform2f(uloc(R.final, "uRes"), (float)g_winW, (float)g_winH);
  glBindTexture(GL_TEXTURE_2D, R.hdrTex);
  for (int i = 1; i <= 3; i++) {
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, R.bTex[(i - 1) * 2]);
  }
  glActiveTexture(GL_TEXTURE0);
  fullscreen();
}

int r_project(float x, float y, float z, float *sx, float *sy) {
  float v[4] = {x, y, z, 1}, o[4];
  for (int r = 0; r < 4; r++) o[r] = g_vp.m[r] * v[0] + g_vp.m[4 + r] * v[1] + g_vp.m[8 + r] * v[2] + g_vp.m[12 + r];
  if (o[3] < 0.05f) return 0;
  float nx = o[0] / o[3], ny = o[1] / o[3];
  float px = (nx * 0.5f + 0.5f) * g_winW, py = (1 - (ny * 0.5f + 0.5f)) * g_winH;
  *sx = (px - g_uiOffX) / g_uiScale;
  *sy = (py - g_uiOffY) / g_uiScale;
  return 1;
}

/* ---------------------------------------------------------------- render do tekstury */
RTarget rt_create(int w, int h) {
  RTarget t;
  t.w = w; t.h = h;
  glGenFramebuffers(1, &t.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
  t.tex = make_tex(w, h, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.tex, 0);
  glGenRenderbuffers(1, &t.depth);
  glBindRenderbuffer(GL_RENDERBUFFER, t.depth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, t.depth);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return t;
}

/* portret: prosta scena ze światłem, bez postprocesu (ton mapowany w shaderze przez ekspozycję) */
void rt_begin(RTarget *t, uint32_t clear) {
  glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
  glViewport(0, 0, t->w, t->h);
  glClearColor(((clear >> 16) & 255) / 255.0f, ((clear >> 8) & 255) / 255.0f, (clear & 255) / 255.0f, ((clear >> 24) & 255) / 255.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glUseProgram(R.world);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, R.worldTex);
  M4 id = m4_identity();
  glUniformMatrix4fv(R.wBones, 1, GL_FALSE, id.m);
}
/* portret: kamera patrzy z eye na target, oświetlenie studyjne (klucz + kontra), bez mgły */
void r_portrait_begin(RTarget *t, V3 eye, V3 target, float fov) {
  rt_begin(t, 0x00000000);
  M4 proj = m4_perspective(fov * 3.14159265f / 180.0f, (float)t->w / t->h, 0.05f, 20.0f);
  V3 f = v3norm(v3sub(target, eye));
  float yaw = atan2f(f.x, f.z), pitch = asinf(f.y);
  M4 rot = m4_mul(m4_rotx(-pitch), m4_roty(3.14159265f - yaw));
  M4 view = m4_mul(rot, m4_translate(-eye.x, -eye.y, -eye.z));
  M4 vp = m4_mul(proj, view);
  glUniformMatrix4fv(R.wVP, 1, GL_FALSE, vp.m);
  glUniform3f(R.wCam, eye.x, eye.y, eye.z);
  glUniform1i(R.wTex, 0);
  glUniform1i(R.wVM, 2);
  glUniform3f(R.wAmbSky, 0.32f, 0.3f, 0.3f);
  glUniform3f(R.wAmbGround, 0.12f, 0.1f, 0.1f);
  glUniform3f(R.wSunDir, 0.6f, 0.5f, 0.7f);
  glUniform3f(R.wSunCol, 1.1f, 0.95f, 0.8f);
  glUniform3f(R.wFogCol, 0, 0, 0);
  glUniform1f(R.wFogDens, 0.0f);
  glUniform1f(R.wWet, 0);
  glUniform1f(R.wTime, g_time);
  float pos[4] = {-0.6f, 1.1f, -0.5f, 3.0f}, col[4] = {0.5f, 0.6f, 0.9f, 1};
  glUniform4fv(R.wLPos, 1, pos);
  glUniform4fv(R.wLCol, 1, col);
  glUniform1i(R.wNL, 1);
}
void r_portrait_end(void) {
  glUniform1i(R.wVM, 0);
  rt_end();
}

void rt_end(void) {
  glBindFramebuffer(GL_FRAMEBUFFER, R.msaaActive ? R.msFbo : R.hdrFbo);
  glViewport(0, 0, R.rw, R.rh);
}

/* ---------------------------------------------------------------- zrzut ekranu (BMP 24-bit) */
int r_screenshot(const char *path) {
  int w = g_winW, h = g_winH;
  uint8_t *px = (uint8_t *)malloc((size_t)w * h * 4);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px);
  FILE *f = fopen(path, "wb");
  if (!f) { free(px); return 0; }
  int row = (w * 3 + 3) & ~3;
  int size = 54 + row * h;
  uint8_t hd[54] = {'B', 'M'};
  hd[2] = size; hd[3] = size >> 8; hd[4] = size >> 16; hd[5] = size >> 24;
  hd[10] = 54; hd[14] = 40;
  hd[18] = w; hd[19] = w >> 8; hd[22] = h; hd[23] = h >> 8;
  hd[26] = 1; hd[28] = 24;
  fwrite(hd, 1, 54, f);
  uint8_t *line = (uint8_t *)calloc(row, 1);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      uint8_t *p = px + ((size_t)y * w + x) * 4;
      line[x * 3] = p[2]; line[x * 3 + 1] = p[1]; line[x * 3 + 2] = p[0];
    }
    fwrite(line, 1, row, f);
  }
  free(line);
  fclose(f);
  free(px);
  return 1;
}
