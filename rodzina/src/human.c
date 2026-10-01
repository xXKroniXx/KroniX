/* Postacie 3D: low-poly ludzie z lat 30. budowani z parametrów (stroje, kapelusze, twarze),
 * sztywny szkielet 13 kości, animacje proceduralne, portrety renderowane do tekstur. */
#include "assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { HAT_NONE, HAT_FEDORA, HAT_CAP, HAT_POLICE, HAT_TOQUE, HAT_CLOCHE, HAT_TOP, HAT_SCARF };
enum { TOP_SUIT, TOP_VEST, TOP_SHIRT, TOP_COAT, TOP_DRESS, TOP_APRON, TOP_UNIFORM, TOP_CASSOCK, TOP_BARE };

typedef struct {
  const char *name;
  uint32_t skin, hair; int hat; uint32_t hatc, band;
  int top; uint32_t topc, top2, tie, shirt;
  uint32_t pants, shoes;
  int mustache, beard, build, longhair, child;
} CharParam;

#define C(r, g, b) (0xFF000000u | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define SK C(0xf0, 0xc8, 0xa0)
#define SK2 C(0xc8, 0x92, 0x68)
#define SK3 C(0xe0, 0xb8, 0x88)
#define BLK C(0x1c, 0x1a, 0x22)

static const CharParam CP[] = {
  /* name     skin hair         hat         hatc              band            top          topc              top2              tie               shirt             pants             shoes  must beard build long child */
  {"tomek",   SK, C(0x6a,0x44,0x2a), HAT_CAP,    C(0x56,0x5c,0x68), C(0x3a,0x3e,0x48), TOP_VEST, C(0x4a,0x42,0x3a), C(0x6a,0x46,0x2e), 0, C(0xe8,0xe4,0xda), C(0x3a,0x3c,0x48), BLK, 0,0,1,0,0},
  {"don",     SK3, C(0xc0,0xc0,0xc8), HAT_NONE, 0,0, TOP_SUIT, C(0x1e,0x1c,0x24), C(0x10,0x10,0x16), C(0x8a,0x1e,0x28), C(0xf0,0xf0,0xf0), C(0x1e,0x1c,0x24), BLK, 1,0,2,0,0},
  {"vito",    SK3, C(0x1a,0x16,0x14), HAT_FEDORA, C(0x3a,0x38,0x40), C(0x14,0x12,0x16), TOP_SUIT, C(0x6a,0x6a,0x72), C(0x4a,0x4a,0x52), C(0xd8,0xb0,0x30), C(0xf0,0xf0,0xf0), C(0x5a,0x5a,0x62), BLK, 0,0,1,0,0},
  {"lucia",   SK, C(0x16,0x12,0x14), HAT_NONE, 0,0, TOP_DRESS, C(0xa8,0x1e,0x30), C(0x7a,0x14,0x22), 0, 0, C(0xa8,0x1e,0x30), BLK, 0,0,0,1,0},
  {"sean",    SK, C(0xc8,0x5a,0x2a), HAT_FEDORA, C(0x1e,0x40,0x2c), C(0x0e,0x20,0x16), TOP_SUIT, C(0x1e,0x44,0x2e), C(0x14,0x30,0x20), C(0xe8,0xb0,0x30), C(0xf0,0xf0,0xf0), C(0x1e,0x44,0x2e), BLK, 0,0,1,0,0},
  {"paddy",   SK, C(0xc8,0x5a,0x2a), HAT_CAP,    C(0x2a,0x4a,0x30), C(0x1a,0x30,0x20), TOP_VEST, C(0x7a,0x7a,0x82), C(0x2a,0x2a,0x30), 0, C(0xd8,0xd0,0xc0), C(0x4a,0x4a,0x52), BLK, 1,0,2,0,0},
  {"doyle",   SK, C(0x5a,0x3a,0x28), HAT_POLICE, C(0x1c,0x24,0x4a), C(0xe8,0xb0,0x30), TOP_UNIFORM, C(0x22,0x2c,0x5a), C(0x16,0x1e,0x40), 0, C(0xe8,0xb0,0x30), C(0x16,0x1e,0x40), BLK, 1,0,2,0,0},
  {"walsh",   SK, C(0x5a,0x3a,0x28), HAT_FEDORA, C(0x6a,0x52,0x3a), C(0x2a,0x1e,0x14), TOP_COAT, C(0xb0,0x94,0x6a), C(0x8a,0x70,0x4e), C(0x5a,0x2a,0x2a), C(0xe0,0xe0,0xe0), C(0x4a,0x40,0x38), BLK, 0,0,1,0,0},
  {"gliniarz",SK, C(0x5a,0x3a,0x28), HAT_POLICE, C(0x1c,0x24,0x4a), C(0xb8,0xb8,0xc0), TOP_UNIFORM, C(0x26,0x32,0x66), C(0x1a,0x24,0x4c), 0, C(0xb8,0xb8,0xc0), C(0x1a,0x24,0x4c), BLK, 0,0,1,0,0},
  {"zbir",    SK, C(0x1a,0x16,0x14), HAT_FEDORA, C(0x1a,0x18,0x20), C(0x0a,0x0a,0x0e), TOP_SUIT, C(0x2a,0x28,0x32), C(0x1c,0x1a,0x22), C(0x1c,0x1a,0x22), C(0xd8,0xd8,0xd8), C(0x2a,0x28,0x32), BLK, 0,0,1,0,0},
  {"zbir2",   SK, C(0xb8,0x6a,0x3a), HAT_CAP,    C(0x2a,0x4a,0x30), C(0x1a,0x30,0x20), TOP_VEST, C(0x8a,0x8a,0x92), C(0x3a,0x2e,0x24), 0, C(0xd8,0xd0,0xc0), C(0x4a,0x44,0x3e), BLK, 0,0,1,0,0},
  {"dokowiec",SK2, C(0x4a,0x30,0x20), HAT_CAP,   C(0x2a,0x3a,0x6a), C(0x1a,0x24,0x4a), TOP_SHIRT, C(0x9a,0x5a,0x3a), C(0x7a,0x44,0x2a), 0, 0, C(0x2a,0x32,0x5a), BLK, 0,1,2,0,0},
  {"bokser",  SK, C(0xc8,0x5a,0x2a), HAT_NONE, 0,0, TOP_BARE, SK, C(0xd8,0xa8,0x80), 0, 0, C(0x1e,0x6a,0x3a), BLK, 0,0,2,0,0},
  {"piekarz", SK, C(0x6a,0x44,0x2a), HAT_TOQUE,  C(0xf0,0xf0,0xf0), C(0xc8,0xc8,0xc8), TOP_APRON, C(0xf0,0xf0,0xf0), C(0xd8,0xd8,0xd8), 0, C(0xf0,0xf0,0xf0), C(0x4a,0x44,0x3e), BLK, 1,0,2,0,0},
  {"feliks",  SK3, C(0xb0,0xb0,0xb8), HAT_NONE, 0,0, TOP_VEST, C(0x5a,0x3a,0x28), C(0x3a,0x26,0x18), C(0x2a,0x36,0x6a), C(0xe8,0xe4,0xda), C(0x3a,0x3a,0x44), BLK, 1,0,0,0,0},
  {"stefek",  SK, C(0x6a,0x44,0x2a), HAT_CAP,    C(0x6a,0x6a,0x72), C(0x4a,0x4a,0x52), TOP_SHIRT, C(0x3a,0x5a,0xa0), C(0x2a,0x40,0x7a), 0, 0, C(0x5a,0x3a,0x28), BLK, 0,0,0,0,1},
  {"mickey",  SK, C(0xb0,0xb0,0xb8), HAT_CAP,    C(0x8a,0x1e,0x28), C(0x5a,0x14,0x1a), TOP_SHIRT, C(0x8a,0x8a,0x92), C(0x6a,0x6a,0x72), 0, 0, C(0x3a,0x3a,0x44), BLK, 1,0,1,0,0},
  {"kelner",  SK3, C(0x1a,0x16,0x14), HAT_NONE, 0,0, TOP_VEST, C(0xf0,0xf0,0xf0), C(0x1c,0x1a,0x22), C(0x1c,0x1a,0x22), C(0xf0,0xf0,0xf0), C(0x1c,0x1a,0x22), BLK, 0,0,0,0,0},
  {"helena",  SK, C(0xd8,0xd8,0xe0), HAT_SCARF,  C(0x6a,0x2a,0x6a), C(0x4a,0x1a,0x4a), TOP_DRESS, C(0x4a,0x3a,0x5a), C(0x36,0x28,0x44), 0, 0, C(0x4a,0x3a,0x5a), BLK, 0,0,0,0,0},
  {"pan",     SK, C(0x5a,0x3a,0x28), HAT_FEDORA, C(0x6a,0x4a,0x30), C(0x3a,0x28,0x18), TOP_SUIT, C(0x7a,0x5a,0x3a), C(0x5a,0x42,0x2a), C(0x8a,0x1e,0x28), C(0xe8,0xe4,0xda), C(0x5a,0x42,0x2a), BLK, 0,0,1,0,0},
  {"pani",    SK, C(0x5a,0x3a,0x28), HAT_CLOCHE, C(0x3a,0x5a,0x9a), C(0xe8,0xe0,0xd8), TOP_DRESS, C(0x4a,0x7a,0xc0), C(0x36,0x5a,0x96), 0, 0, C(0x4a,0x7a,0xc0), BLK, 0,0,0,1,0},
  {"robotnik",SK2, C(0x1a,0x16,0x14), HAT_CAP,   C(0x5a,0x44,0x30), C(0x3a,0x2a,0x1e), TOP_SHIRT, C(0x3a,0x4a,0x6a), C(0x2a,0x36,0x4e), 0, 0, C(0x5a,0x44,0x30), BLK, 0,0,1,0,0},
  {"menel",   SK2, C(0x7a,0x7a,0x82), HAT_NONE, 0,0, TOP_COAT, C(0x5a,0x4a,0x3a), C(0x42,0x36,0x2a), 0, C(0x8a,0x8a,0x8a), C(0x3a,0x34,0x2e), BLK, 0,1,0,0,0},
  {"mafioso", SK3, C(0x1a,0x16,0x14), HAT_FEDORA, C(0x4a,0x4a,0x52), C(0x1a,0x1a,0x1e), TOP_SUIT, C(0x16,0x14,0x1a), C(0x0e,0x0c,0x12), C(0xf0,0xf0,0xf0), C(0x2a,0x2a,0x30), C(0x16,0x14,0x1a), BLK, 0,0,1,0,0},
  {"lucky",   SK3, C(0x1a,0x16,0x14), HAT_FEDORA, C(0x4a,0x2a,0x5a), C(0x1a,0x10,0x20), TOP_SUIT, C(0x5a,0x2e,0x6a), C(0x40,0x20,0x4c), C(0xe8,0xb0,0x30), C(0xf0,0xf0,0xf0), C(0x40,0x20,0x4c), BLK, 1,0,1,0,0},
  {"bronek",  SK, C(0xb0,0xb0,0xb8), HAT_CAP,    C(0x6a,0x4a,0x30), C(0x4a,0x32,0x20), TOP_VEST, C(0x8a,0x6a,0x4a), C(0x5a,0x42,0x2a), 0, C(0xe8,0xe4,0xda), C(0x4a,0x40,0x38), BLK, 1,0,2,0,0},
  {"zoska",   SK, C(0xe8,0xc0,0x60), HAT_NONE, 0,0, TOP_APRON, C(0xa8,0x2a,0x36), C(0xf0,0xf0,0xf0), 0, 0, C(0xa8,0x2a,0x36), BLK, 0,0,1,1,0},
  {"ksiadz",  SK, C(0x5a,0x5a,0x62), HAT_NONE, 0,0, TOP_CASSOCK, C(0x16,0x14,0x1a), C(0x0e,0x0c,0x12), 0, C(0xf0,0xf0,0xf0), C(0x16,0x14,0x1a), BLK, 0,0,1,0,0},
  {"mama",    SK, C(0xb0,0xb0,0xb8), HAT_SCARF,  C(0x2a,0x46,0x8a), C(0x6a,0x8a,0xc8), TOP_DRESS, C(0x2a,0x2c,0x44), C(0x1c,0x1e,0x30), 0, 0, C(0x2a,0x2c,0x44), BLK, 0,0,1,0,0},
  {"janek",   SK, C(0xe8,0xc0,0x60), HAT_CAP,    C(0x8a,0x6a,0x4a), C(0x5a,0x42,0x2a), TOP_SHIRT, C(0x6a,0xa8,0xe8), C(0x4a,0x80,0xc0), 0, 0, C(0x5a,0x44,0x30), BLK, 0,0,0,0,0},
  {"lee",     C(0xe8,0xc8,0x98), C(0x10,0x10,0x14), HAT_NONE, 0,0, TOP_COAT, C(0x1a,0x2a,0x3a), C(0xc8,0xa0,0x30), 0, C(0x10,0x10,0x14), C(0x1a,0x1a,0x22), BLK, 1,0,1,0,0},
  {"triada",  C(0xe8,0xc8,0x98), C(0x10,0x10,0x14), HAT_CAP, C(0x1a,0x1a,0x22), C(0x0a,0x0a,0x0e), TOP_SHIRT, C(0x1a,0x1a,0x22), C(0x10,0x10,0x16), 0, 0, C(0x1a,0x1a,0x22), BLK, 0,0,1,0,0},
  {"kane",    SK, C(0xd8,0xd8,0xe0), HAT_TOP,    C(0x10,0x10,0x14), C(0x5a,0x14,0x1a), TOP_SUIT, C(0xe8,0xe4,0xd8), C(0xc0,0xbc,0xb0), C(0x10,0x10,0x14), C(0xf0,0xf0,0xf0), C(0xe8,0xe4,0xd8), C(0x3a,0x1e,0x14), 1,0,2,0,0},
  {"ochroniarz", SK, C(0x3a,0x2a,0x20), HAT_FEDORA, C(0x5a,0x5a,0x62), C(0x2a,0x2a,0x30), TOP_COAT, C(0x3a,0x3a,0x44), C(0x2a,0x2a,0x32), C(0x1c,0x1a,0x22), C(0xd8,0xd8,0xd8), C(0x2a,0x2a,0x32), BLK, 0,0,2,0,0},
  {"russo",   SK3, C(0x2a,0x1a,0x10), HAT_FEDORA, C(0x5a,0x3a,0x22), C(0x2a,0x1a,0x10), TOP_SUIT, C(0x6a,0x46,0x2a), C(0x4a,0x30,0x1c), C(0xb0,0x30,0x30), C(0xe0,0xd8,0xc8), C(0x4a,0x30,0x1c), BLK, 0,0,1,0,0},
  {"agent",   SK, C(0x3a,0x2a,0x20), HAT_FEDORA, C(0x4a,0x4e,0x56), C(0x1a,0x1a,0x1e), TOP_SUIT, C(0x5a,0x60,0x6a), C(0x42,0x48,0x52), C(0x1c,0x24,0x4a), C(0xf0,0xf0,0xf0), C(0x42,0x48,0x52), BLK, 0,0,1,0,0},
  {"gino",    SK3, C(0x1a,0x16,0x14), HAT_CAP,   C(0x3a,0x3a,0x44), C(0x1a,0x1a,0x22), TOP_VEST, C(0xe0,0xd8,0xc8), C(0x3a,0x3a,0x44), 0, C(0xe0,0xd8,0xc8), C(0x3a,0x3a,0x44), BLK, 1,0,1,0,0},
  {"enzo",    SK3, C(0x8a,0x8a,0x92), HAT_NONE, 0,0, TOP_SUIT, C(0x2a,0x2a,0x3a), C(0x1a,0x1a,0x28), C(0x3a,0x5a,0x3a), C(0xf0,0xf0,0xf0), C(0x2a,0x2a,0x3a), BLK, 0,0,0,0,0},
  {"sal",     SK3, C(0x1a,0x16,0x14), HAT_FEDORA, C(0x2a,0x2a,0x30), C(0x8a,0x1e,0x28), TOP_SUIT, C(0x3a,0x2a,0x22), C(0x2a,0x1e,0x18), C(0x8a,0x1e,0x28), C(0xf0,0xf0,0xf0), C(0x3a,0x2a,0x22), BLK, 1,0,2,0,0},
  {"dolores", SK, C(0x14,0x10,0x10), HAT_NONE, 0,0, TOP_DRESS, C(0x6a,0x10,0x1a), C(0x4a,0x0a,0x12), 0, 0, C(0x6a,0x10,0x1a), BLK, 0,0,0,1,0},
  {"flapper", SK, C(0x2a,0x1a,0x10), HAT_CLOCHE, C(0x1a,0x6a,0x6a), C(0xe8,0xd0,0x60), TOP_DRESS, C(0xd8,0xb8,0x50), C(0xa8,0x88,0x30), 0, 0, C(0xd8,0xb8,0x50), BLK, 0,0,0,0,0},
  {"muzyk",   SK2, C(0x10,0x10,0x10), HAT_NONE, 0,0, TOP_SUIT, C(0x16,0x16,0x1c), C(0x0c,0x0c,0x10), C(0xe8,0xe8,0xe8), C(0xf0,0xf0,0xf0), C(0x16,0x16,0x1c), BLK, 1,0,1,0,0},
  {"barman",  SK, C(0x3a,0x2a,0x1a), HAT_NONE, 0,0, TOP_VEST, C(0x1c,0x1a,0x22), C(0x14,0x12,0x18), C(0x8a,0x1e,0x28), C(0xf0,0xf0,0xf0), C(0x1c,0x1a,0x22), BLK, 1,0,1,0,0},
  {"kessler", SK, C(0x4a,0x3a,0x2a), HAT_FEDORA, C(0x3a,0x3e,0x46), C(0x1a,0x1a,0x1e), TOP_COAT, C(0x4e,0x52,0x5a), C(0x3a,0x3e,0x46), C(0x1a,0x2a,0x4a), C(0xf0,0xf0,0xf0), C(0x2a,0x2e,0x36), BLK, 0,0,1,0,0},
  {"bokser2", SK2, C(0x1a,0x16,0x14), HAT_NONE, 0,0, TOP_BARE, SK2, C(0xb0,0x80,0x58), 0, 0, C(0x8a,0x1a,0x1a), BLK, 0,0,2,0,0},
  {"marynarz",SK, C(0x5a,0x3a,0x28), HAT_CAP, C(0x1a,0x22,0x3a), C(0x10,0x14,0x22), TOP_COAT, C(0x1a,0x22,0x3a), C(0x10,0x14,0x22), 0, C(0xe8,0xe8,0xe8), C(0x1a,0x1a,0x22), BLK, 0,1,2,0,0},
};
#define NCP ((int)(sizeof(CP) / sizeof(CP[0])))

typedef struct { GMesh body; GMesh guns[4]; int face; float scale; unsigned portrait; } HType;
static HType HT[NCP];
static V3 PIV[NBONES]; /* punkty obrotu kości (pozycja spoczynkowa) */
static int PARENT[NBONES] = {-1, 0, 1, 2, 2, 4, 2, 6, 1, 8, 1, 10, 7};

int human_count(void) { return NCP; }
const char *human_name(int i) { return i >= 0 && i < NCP ? CP[i].name : ""; }
int human_find(const char *name) {
  for (int i = 0; i < NCP; i++) if (!strcmp(CP[i].name, name)) return i;
  return -1;
}

static uint32_t darker(uint32_t c, float k) {
  int r = (int)(((c >> 16) & 255) * k), g = (int)(((c >> 8) & 255) * k), b = (int)((c & 255) * k);
  return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static int face_for(const CharParam *p) {
  const char *n = p->name;
  if (p->child) return 14;
  if (!strcmp(n, "dolores") || !strcmp(n, "flapper")) return 11;
  if (p->top == TOP_DRESS || p->hat == HAT_CLOCHE || p->hat == HAT_SCARF || p->longhair) {
    uint32_t h = p->hair;
    return (((h >> 16) & 255) > 0xa0 && ((h >> 8) & 255) > 0xa0) ? 5 : 4;
  }
  if (!strcmp(n, "lee") || !strcmp(n, "triada")) return 7;
  if (!strcmp(n, "kane") || !strcmp(n, "lucky")) return 8;
  if (!strcmp(n, "russo")) return 10;
  if (!strcmp(n, "ksiadz")) return 12;
  if (!strncmp(n, "zbir", 4) || !strcmp(n, "sal") || !strcmp(n, "bokser")) return 9;
  int old = ((p->hair >> 16) & 255) > 0xa0 && ((p->hair >> 8) & 255) > 0xa0;
  if (p->mustache) return old ? 3 : 1;
  if (p->beard) return 2;
  if (!strcmp(n, "tomek") || !strcmp(n, "janek") || !strcmp(n, "stefek") || !strcmp(n, "mickey")) return 6;
  if (!strcmp(n, "enzo")) return 13;
  return old ? 3 : (n[0] + n[1]) % 2 ? 0 : 15;
}

static MB *B_;
static void bone(int b) { P_.bone = b; }
static void rb(float x0, float y0, float z0, float x1, float y1, float z1, float e) { mb_rbox(B_, v3(x0, y0, z0), v3(x1, y1, z1), e); }

static void build_type(int t) {
  const CharParam *p = &CP[t];
  MB m;
  mb_init(&m);
  B_ = &m;
  float sw = p->build == 0 ? 0.095f : p->build == 2 ? 0.125f : 0.11f;  /* pół szerokości barków */
  float dz = p->build == 2 ? 0.085f : 0.065f;
  int female = p->top == TOP_DRESS || p->longhair || p->hat == HAT_CLOCHE || p->hat == HAT_SCARF;
  if (female && p->top != TOP_DRESS) female = p->longhair || p->hat == HAT_CLOCHE || p->hat == HAT_SCARF;
  uint32_t sleeve = (p->top == TOP_VEST || p->top == TOP_APRON) ? (p->shirt ? p->shirt : 0xFFE8E4DA) : p->top == TOP_BARE ? p->skin : p->topc;
  int clothL = L_CLOTH;
  /* --- nogi */
  for (int s = -1; s <= 1; s += 2) {
    float lx = s * 0.055f;
    int thigh = s > 0 ? B_LEG_L : B_LEG_R, shin = s > 0 ? B_SHIN_L : B_SHIN_R;
    bone(thigh); mb_paint(p->pants, clothL); P_.bone = thigh;
    rb(lx - 0.037f, 0.26f, -0.04f, lx + 0.037f, 0.48f, 0.04f, 0.015f);
    mb_paint(p->pants, clothL); P_.bone = shin;
    rb(lx - 0.032f, 0.05f, -0.035f, lx + 0.032f, 0.27f, 0.035f, 0.014f);
    mb_paint(p->shoes, L_LEATHER); P_.bone = shin;
    rb(lx - 0.035f, 0.0f, -0.045f, lx + 0.035f, 0.055f, 0.075f, 0.018f);
  }
  /* --- miednica */
  mb_paint(p->pants, clothL); P_.bone = B_PELVIS;
  rb(-sw * 0.82f, 0.43f, -dz * 0.95f, sw * 0.82f, 0.53f, dz * 0.95f, 0.02f);
  /* długie ubrania (sztywne, na miednicy) */
  if (p->top == TOP_DRESS || p->top == TOP_CASSOCK || p->top == TOP_COAT) {
    float hem = p->top == TOP_COAT ? 0.27f : p->top == TOP_CASSOCK ? 0.04f : 0.14f;
    mb_paint(p->topc, clothL); P_.bone = B_PELVIS;
    mb_cyl(B_, v3(0, hem, 0), p->top == TOP_DRESS ? 0.13f : 0.11f, sw * 0.85f, 0.53f - hem, 10, 0);
  }
  /* --- tułów */
  mb_paint(p->top == TOP_BARE ? p->skin : p->topc, p->top == TOP_BARE ? L_SKIN : clothL); P_.bone = B_SPINE;
  float belly = p->build == 2 ? 0.02f : 0;
  rb(-sw, 0.5f, -dz, sw, 0.745f, dz + belly, 0.035f);
  float fz = dz + belly + 0.002f;
  if (p->top == TOP_SUIT || p->top == TOP_COAT || p->top == TOP_UNIFORM) {
    /* koszula w dekolcie, krawat, klapy */
    mb_paint(p->shirt ? p->shirt : 0xFFF0F0F0, L_CLOTH); P_.bone = B_SPINE;
    mb_quad(B_, v3(-0.03f, 0.63f, fz), v3(0.03f, 0.63f, fz), v3(0.045f, 0.742f, fz), v3(-0.045f, 0.742f, fz), 0, 0, 0.1f, 0.1f);
    if (p->tie) {
      mb_paint(p->tie, L_CLOTH); P_.bone = B_SPINE;
      mb_quad(B_, v3(-0.011f, 0.58f, fz + 0.003f), v3(0.011f, 0.58f, fz + 0.003f), v3(0.009f, 0.735f, fz + 0.003f), v3(-0.009f, 0.735f, fz + 0.003f), 0, 0, 0.1f, 0.1f);
    }
    mb_paint(darker(p->topc, 0.7f), L_CLOTH); P_.bone = B_SPINE;
    for (int s = -1; s <= 1; s += 2) {
      mb_tri(B_, v3(s * 0.03f, 0.63f, fz + 0.001f), v3(s * 0.075f, 0.735f, fz + 0.001f), v3(s * 0.045f, 0.742f, fz + 0.001f), (float[]){0, 0}, (float[]){0.1f, 0}, (float[]){0, 0.1f});
      mb_tri(B_, v3(s * 0.03f, 0.63f, fz + 0.001f), v3(s * 0.045f, 0.742f, fz + 0.001f), v3(s * 0.075f, 0.735f, fz + 0.001f), (float[]){0, 0}, (float[]){0.1f, 0}, (float[]){0, 0.1f});
    }
    mb_paint(0xFF1A1A1A, L_WHITE); P_.bone = B_SPINE;
    for (int k = 0; k < 2; k++) mb_sphere(B_, v3(0.0f, 0.55f + k * 0.04f, fz), 0.006f, 0.006f, 0.004f, 5);
    if (p->top == TOP_UNIFORM) {
      mb_paint(0xFF1A1A1A, L_LEATHER); P_.bone = B_SPINE;
      rb(-sw - 0.002f, 0.5f, -dz - 0.002f, sw + 0.002f, 0.525f, fz + 0.002f, 0.005f);
      mb_paint(0xFFD8B460, L_BRASS); P_.bone = B_SPINE;
      mb_sphere(B_, v3(-0.05f, 0.68f, fz), 0.012f, 0.014f, 0.004f, 6);
    }
  } else if (p->top == TOP_VEST) {
    mb_paint(p->shirt ? p->shirt : 0xFFE8E4DA, L_CLOTH); P_.bone = B_SPINE;
    rb(-sw * 0.55f, 0.66f, dz - 0.01f, sw * 0.55f, 0.745f, fz + 0.002f, 0.01f);
    if (p->tie) { mb_paint(p->tie, L_CLOTH); P_.bone = B_SPINE; rb(-0.01f, 0.64f, fz, 0.01f, 0.735f, fz + 0.006f, 0.003f); }
    mb_paint(darker(p->topc, 0.6f), L_WHITE); P_.bone = B_SPINE;
    for (int k = 0; k < 3; k++) mb_sphere(B_, v3(0.0f, 0.54f + k * 0.035f, fz), 0.005f, 0.005f, 0.004f, 5);
  } else if (p->top == TOP_APRON) {
    mb_paint(p->top2 == p->topc ? 0xFFF0F0F0 : p->top2, L_CLOTH); P_.bone = B_SPINE;
    rb(-sw * 0.8f, 0.5f, fz - 0.004f, sw * 0.8f, 0.7f, fz + 0.004f, 0.003f);
    P_.bone = B_PELVIS;
    rb(-sw * 0.85f, 0.25f, dz + 0.005f, sw * 0.85f, 0.53f, dz + 0.013f, 0.003f);
  } else if (p->top == TOP_CASSOCK) {
    mb_paint(0xFFF0F0F0, L_CLOTH); P_.bone = B_SPINE;
    rb(-0.015f, 0.715f, fz - 0.002f, 0.015f, 0.74f, fz + 0.004f, 0.002f);
  }
  /* --- ręce */
  for (int s = -1; s <= 1; s += 2) {
    float ax = s * (sw + 0.022f);
    int up = s > 0 ? B_ARM_L : B_ARM_R, fo = s > 0 ? B_FORE_L : B_FORE_R;
    mb_paint(sleeve, sleeve == p->skin ? L_SKIN : clothL); P_.bone = up;
    rb(ax - 0.027f, 0.52f, -0.03f, ax + 0.027f, 0.745f, 0.03f, 0.016f);
    mb_paint(sleeve, sleeve == p->skin ? L_SKIN : clothL); P_.bone = fo;
    rb(ax - 0.024f, 0.37f, -0.027f, ax + 0.024f, 0.535f, 0.027f, 0.014f);
    mb_paint(p->skin, L_SKIN); P_.bone = fo;
    rb(ax - 0.02f, 0.31f, -0.022f, ax + 0.02f, 0.375f, 0.024f, 0.012f);
  }
  /* --- szyja i głowa */
  mb_paint(p->skin, L_SKIN); P_.bone = B_HEAD;
  mb_cyl(B_, v3(0, 0.735f, 0), 0.028f, 0.026f, 0.05f, 8, 0);
  float hw = 0.062f, hy0 = 0.765f, hy1 = 0.905f, hz0 = -0.06f, hz1 = 0.066f;
  rb(-hw, hy0, hz0, hw, hy1, hz1, 0.022f);
  /* uszy, nos */
  for (int s = -1; s <= 1; s += 2) mb_sphere(B_, v3(s * (hw + 0.004f), 0.835f, 0.0f), 0.01f, 0.018f, 0.012f, 6);
  mb_paint(darker(p->skin, 0.95f), L_SKIN); P_.bone = B_HEAD;
  rb(-0.011f, 0.815f, hz1 - 0.004f, 0.011f, 0.85f, hz1 + 0.016f, 0.008f);
  /* twarz: czworokąt z atlasu */
  int fi = face_for(p);
  float fu0 = (fi % 4) / 4.0f, fv0 = (fi / 4) / 4.0f;
  mb_paint(p->skin, L_FACES); P_.bone = B_HEAD;
  mb_quad(B_, v3(-hw + 0.016f, hy0 + 0.012f, hz1 + 0.001f), v3(hw - 0.016f, hy0 + 0.012f, hz1 + 0.001f), v3(hw - 0.016f, hy1 - 0.016f, hz1 + 0.001f),
          v3(-hw + 0.016f, hy1 - 0.016f, hz1 + 0.001f), fu0 + 0.02f, fv0 + 0.03f, fu0 + 0.23f, fv0 + 0.24f);
  /* włosy */
  mb_paint(p->hair, L_CLOTH); P_.bone = B_HEAD;
  rb(-hw - 0.004f, 0.865f, hz0 - 0.006f, hw + 0.004f, hy1 + 0.008f, hz1 - 0.035f, 0.03f);
  rb(-hw - 0.005f, 0.79f, hz0 - 0.008f, hw + 0.005f, 0.88f, hz0 + 0.03f, 0.02f);
  if (female || p->longhair) {
    rb(-hw - 0.012f, 0.70f, hz0 - 0.012f, hw + 0.012f, 0.88f, hz0 + 0.05f, 0.03f);
    rb(-hw - 0.012f, 0.79f, hz0, -hw + 0.008f, 0.89f, hz1 - 0.02f, 0.01f);
    rb(hw - 0.008f, 0.79f, hz0, hw + 0.012f, 0.89f, hz1 - 0.02f, 0.01f);
  }
  if (p->mustache || p->beard) {
    mb_paint(p->hair, L_CLOTH); P_.bone = B_HEAD;
    if (p->beard) rb(-hw + 0.004f, hy0 - 0.012f, hz1 - 0.03f, hw - 0.004f, 0.8f, hz1 + 0.004f, 0.018f);
  }
  /* kapelusze */
  P_.bone = B_HEAD;
  switch (p->hat) {
    case HAT_FEDORA:
      mb_paint(p->hatc, L_CLOTH); P_.bone = B_HEAD;
      mb_cyl(B_, v3(0, hy1 - 0.012f, 0), 0.118f, 0.112f, 0.012f, 16, 3);
      mb_cyl(B_, v3(0, hy1, 0), 0.072f, 0.06f, 0.075f, 14, 2);
      mb_paint(p->band ? p->band : darker(p->hatc, 0.5f), L_CLOTH); P_.bone = B_HEAD;
      mb_cyl(B_, v3(0, hy1, 0), 0.0735f, 0.0715f, 0.02f, 14, 0);
      break;
    case HAT_CAP:
      mb_paint(p->hatc, L_CLOTH); P_.bone = B_HEAD;
      mb_sphere(B_, v3(0, hy1 - 0.008f, -0.005f), 0.075f, 0.035f, 0.08f, 12);
      rb(-0.06f, hy1 - 0.02f, 0.03f, 0.06f, hy1 - 0.008f, 0.115f, 0.006f);
      break;
    case HAT_POLICE:
      mb_paint(p->hatc, L_CLOTH); P_.bone = B_HEAD;
      mb_cyl(B_, v3(0, hy1 - 0.015f, 0), 0.07f, 0.082f, 0.06f, 14, 3);
      mb_paint(0xFF101010, L_LEATHER); P_.bone = B_HEAD;
      rb(-0.06f, hy1 - 0.02f, 0.03f, 0.06f, hy1 - 0.008f, 0.11f, 0.006f);
      mb_paint(p->band ? p->band : 0xFFD8B460, L_BRASS); P_.bone = B_HEAD;
      mb_sphere(B_, v3(0, hy1 + 0.015f, 0.075f), 0.014f, 0.016f, 0.005f, 6);
      break;
    case HAT_TOQUE:
      mb_paint(0xFFF2F2F2, L_CLOTH); P_.bone = B_HEAD;
      mb_cyl(B_, v3(0, hy1 - 0.012f, 0), 0.066f, 0.07f, 0.06f, 12, 0);
      mb_sphere(B_, v3(0, hy1 + 0.07f, 0), 0.085f, 0.05f, 0.085f, 12);
      break;
    case HAT_CLOCHE:
      mb_paint(p->hatc, L_CLOTH); P_.bone = B_HEAD;
      mb_sphere(B_, v3(0, hy1 - 0.02f, -0.005f), 0.08f, 0.06f, 0.082f, 14);
      mb_cyl(B_, v3(0, hy1 - 0.045f, -0.005f), 0.088f, 0.082f, 0.012f, 14, 3);
      if (p->band) { mb_paint(p->band, L_CLOTH); P_.bone = B_HEAD; mb_cyl(B_, v3(0, hy1 - 0.035f, -0.005f), 0.082f, 0.08f, 0.012f, 14, 0); }
      break;
    case HAT_TOP:
      mb_paint(p->hatc, L_CLOTH); P_.bone = B_HEAD;
      mb_cyl(B_, v3(0, hy1 - 0.012f, 0), 0.1f, 0.1f, 0.01f, 16, 3);
      mb_cyl(B_, v3(0, hy1, 0), 0.062f, 0.066f, 0.13f, 14, 2);
      if (p->band) { mb_paint(p->band, L_CLOTH); P_.bone = B_HEAD; mb_cyl(B_, v3(0, hy1, 0), 0.064f, 0.064f, 0.02f, 14, 0); }
      break;
    case HAT_SCARF:
      mb_paint(p->hatc, L_CLOTH); P_.bone = B_HEAD;
      mb_sphere(B_, v3(0, 0.865f, -0.008f), 0.077f, 0.068f, 0.075f, 12);
      mb_sphere(B_, v3(0, 0.77f, -0.01f), 0.03f, 0.02f, 0.03f, 6);
      break;
  }
  HT[t].body = gm_upload(&m);
  mb_free(&m);
  HT[t].face = fi;
  HT[t].scale = p->child ? 0.75f : 1.0f;
  /* broń w prawej dłoni (w spoczynku skierowana w dół, wzdłuż przedramienia) */
  float hx = -(sw + 0.022f);
  for (int w = 1; w < 4; w++) {
    MB g;
    mb_init(&g);
    B_ = &g;
    mb_paint(0xFF1E2024, L_METAL); P_.bone = B_FORE_R;
    if (w == 1) {
      rb(hx - 0.012f, 0.3f, 0.0f, hx + 0.012f, 0.33f, 0.04f, 0.004f);
      rb(hx - 0.01f, 0.2f, 0.022f, hx + 0.01f, 0.33f, 0.045f, 0.004f);
    } else {
      float len = w == 2 ? 0.34f : 0.28f;
      mb_paint(0xFF4A2E1A, L_WOOD); P_.bone = B_FORE_R;
      rb(hx - 0.014f, 0.3f, -0.07f, hx + 0.014f, 0.36f, 0.03f, 0.006f);
      mb_paint(0xFF1E2024, L_METAL); P_.bone = B_FORE_R;
      rb(hx - 0.016f, 0.33f - len, 0.02f, hx + 0.016f, 0.34f, 0.05f, 0.005f);
      mb_cyl(B_, v3(hx, 0.33f - len - 0.02f, 0.035f), 0.009f, 0.009f, len * 0.6f, 6, 0);
      if (w == 3) { mb_cyl_x(B_, v3(hx, 0.24f, 0.07f), 0.045f, 0.035f, 12, 3); }
      else { mb_paint(0xFF4A2E1A, L_WOOD); P_.bone = B_FORE_R; rb(hx - 0.015f, 0.12f, 0.0f, hx + 0.015f, 0.2f, 0.03f, 0.006f); }
    }
    HT[t].guns[w] = gm_upload(&g);
    mb_free(&g);
  }
}

void humans_build(void) {
  PIV[B_ROOT] = v3(0, 0, 0);
  PIV[B_PELVIS] = v3(0, 0.48f, 0);
  PIV[B_SPINE] = v3(0, 0.52f, 0);
  PIV[B_HEAD] = v3(0, 0.75f, 0);
  PIV[B_ARM_L] = v3(0.12f, 0.725f, 0);
  PIV[B_FORE_L] = v3(0.13f, 0.53f, 0);
  PIV[B_ARM_R] = v3(-0.12f, 0.725f, 0);
  PIV[B_FORE_R] = v3(-0.13f, 0.53f, 0);
  PIV[B_LEG_L] = v3(0.055f, 0.47f, 0);
  PIV[B_SHIN_L] = v3(0.055f, 0.265f, 0);
  PIV[B_LEG_R] = v3(-0.055f, 0.47f, 0);
  PIV[B_SHIN_R] = v3(-0.055f, 0.265f, 0);
  PIV[B_GUN] = v3(-0.13f, 0.53f, 0);
  for (int t = 0; t < NCP; t++) build_type(t);
  mb_paint(0xFFFFFFFF, L_WHITE);
}

/* ---------------------------------------------------------------- animacja */
typedef struct { float rx[NBONES], ry[NBONES], rz[NBONES]; float lift, lean, fall; } Rot;

static void pose_rot(const HumanPose *p, Rot *r) {
  memset(r, 0, sizeof *r);
  float t = p->t;
  float armOut = 0.08f;
  r->rz[B_ARM_L] = armOut; r->rz[B_ARM_R] = -armOut;
  r->rx[B_SPINE] = 0.03f * sinf(t * 1.7f);
  switch (p->anim) {
    case AN_WALK: case AN_RUN: {
      float run = p->anim == AN_RUN;
      float sp = run ? 9.5f : 6.2f, amp = run ? 0.75f : 0.45f;
      float s = sinf(t * sp);
      r->rx[B_LEG_L] = -s * amp; r->rx[B_LEG_R] = s * amp;
      r->rx[B_SHIN_L] = fmaxf(0, s) * amp * 1.2f; r->rx[B_SHIN_R] = fmaxf(0, -s) * amp * 1.2f;
      r->rx[B_ARM_L] = s * amp * 0.8f; r->rx[B_ARM_R] = -s * amp * 0.8f;
      r->rx[B_FORE_L] = -0.25f - run * 0.6f; r->rx[B_FORE_R] = -0.25f - run * 0.6f;
      r->lift = fabsf(cosf(t * sp)) * (run ? 0.025f : 0.012f);
      r->rx[B_SPINE] = run ? 0.18f : 0.04f;
      r->ry[B_SPINE] = s * 0.08f;
      break;
    }
    case AN_AIM: case AN_SHOOT: {
      float kick = p->anim == AN_SHOOT ? 0.18f : 0;
      float ap = p->aimPitch;
      if (p->weapon <= 1) {
        r->rx[B_ARM_R] = -1.5f - ap - kick; r->rz[B_ARM_R] = 0.15f;
        r->rx[B_ARM_L] = -1.3f - ap; r->rz[B_ARM_L] = -0.45f; r->rx[B_FORE_L] = -0.25f;
        if (p->weapon == 0) { r->rx[B_FORE_R] = -1.2f; r->rx[B_FORE_L] = -1.2f; r->rx[B_ARM_R] = -0.6f; r->rx[B_ARM_L] = -0.7f; r->rz[B_ARM_L] = 0.1f; }
      } else {
        r->rx[B_ARM_R] = -1.15f - ap * 0.8f - kick * 0.5f; r->rz[B_ARM_R] = 0.25f; r->rx[B_FORE_R] = -0.45f;
        r->rx[B_ARM_L] = -1.35f - ap; r->rz[B_ARM_L] = -0.55f; r->rx[B_FORE_L] = -0.4f;
        r->ry[B_SPINE] = 0.2f;
      }
      r->rx[B_HEAD] = -ap * 0.5f;
      r->rx[B_SPINE] += kick * -0.2f;
      r->rx[B_LEG_L] = -0.12f; r->rx[B_LEG_R] = 0.1f; r->rx[B_SHIN_L] = 0.15f;
      break;
    }
    case AN_TALK: {
      r->rx[B_FORE_R] = -0.9f + sinf(t * 3.1f) * 0.35f; r->rx[B_ARM_R] = -0.2f + sinf(t * 2.3f) * 0.12f;
      r->rx[B_FORE_L] = -0.5f + sinf(t * 2.6f + 1) * 0.25f;
      r->rx[B_HEAD] = sinf(t * 2.0f) * 0.06f; r->ry[B_HEAD] = sinf(t * 1.3f) * 0.12f;
      break;
    }
    case AN_SIT: {
      r->rx[B_LEG_L] = r->rx[B_LEG_R] = -1.5f;
      r->rx[B_SHIN_L] = r->rx[B_SHIN_R] = 1.5f;
      r->lift = -0.215f;
      r->rx[B_ARM_L] = r->rx[B_ARM_R] = -0.35f;
      r->rx[B_FORE_L] = r->rx[B_FORE_R] = -0.9f;
      r->rx[B_HEAD] = sinf(t * 0.7f) * 0.05f;
      break;
    }
    case AN_DANCE: {
      float s = sinf(t * 7.0f);
      r->rz[B_PELVIS] = s * 0.12f;
      r->rx[B_LEG_L] = -fmaxf(0, s) * 0.6f; r->rx[B_SHIN_L] = fmaxf(0, s) * 0.9f;
      r->rx[B_LEG_R] = -fmaxf(0, -s) * 0.6f; r->rx[B_SHIN_R] = fmaxf(0, -s) * 0.9f;
      float c = cosf(t * 3.5f);
      r->rx[B_ARM_L] = -0.5f + c * 0.35f; r->rx[B_ARM_R] = -0.5f - c * 0.35f;
      r->rz[B_ARM_L] = 0.25f; r->rz[B_ARM_R] = -0.25f;
      r->rx[B_FORE_L] = -1.3f; r->rx[B_FORE_R] = -1.3f;
      r->ry[B_SPINE] = c * 0.25f;
      r->rx[B_HEAD] = s * 0.06f;
      r->lift = fabsf(s) * 0.02f;
      break;
    }
    case AN_PLAY: {
      r->rx[B_ARM_L] = -0.9f; r->rx[B_ARM_R] = -0.9f;
      r->rx[B_FORE_L] = -0.6f + sinf(t * 8) * 0.08f; r->rx[B_FORE_R] = -0.6f + sinf(t * 8 + 2) * 0.08f;
      r->rz[B_ARM_L] = -0.3f; r->rz[B_ARM_R] = 0.3f;
      r->rx[B_HEAD] = 0.15f + sinf(t * 4) * 0.05f;
      break;
    }
    case AN_HIT: {
      r->rx[B_SPINE] = -0.35f; r->rx[B_HEAD] = -0.3f;
      r->rx[B_ARM_L] = -0.6f; r->rx[B_ARM_R] = -0.4f;
      break;
    }
    case AN_DEAD: {
      float d = p->deadT < 0 ? 0 : p->deadT > 1 ? 1 : p->deadT;
      float e = 1 - (1 - d) * (1 - d);
      r->fall = e;
      r->rx[B_SHIN_L] = 0.5f * e; r->rx[B_LEG_L] = -0.3f * e;
      r->rz[B_ARM_L] = 1.2f * e; r->rz[B_ARM_R] = -1.0f * e;
      r->rx[B_HEAD] = -0.4f * e; r->ry[B_HEAD] = 0.6f * e;
      break;
    }
    default: break;
  }
  if (p->hitT > 0 && p->anim != AN_DEAD) { r->rx[B_SPINE] -= p->hitT * 0.4f; r->rx[B_HEAD] -= p->hitT * 0.3f; }
}

static void compute_bones(const HumanPose *p, M4 *out) {
  Rot r;
  pose_rot(p, &r);
  M4 local[NBONES];
  for (int b = 0; b < NBONES; b++) {
    V3 pv = PIV[b];
    M4 R = m4_mul(m4_roty(r.ry[b]), m4_mul(m4_rotx(r.rx[b]), m4_rotz(r.rz[b])));
    local[b] = m4_mul(m4_translate(pv.x, pv.y, pv.z), m4_mul(R, m4_translate(-pv.x, -pv.y, -pv.z)));
  }
  /* korzeń: podskok i upadek na plecy (obrót wokół stóp) */
  M4 root = m4_translate(0, r.lift, 0);
  if (r.fall > 0) root = m4_mul(m4_translate(0, 0.04f * r.fall, -0.05f * r.fall), m4_rotx(-1.5f * r.fall));
  local[B_ROOT] = root;
  for (int b = 0; b < NBONES; b++) {
    if (b > B_GUN) { out[b] = m4_identity(); continue; }
    out[b] = PARENT[b] < 0 ? local[b] : m4_mul(out[PARENT[b]], local[b]);
  }
}

void human_draw(int type, float x, float y, float z, float yaw, const HumanPose *pose, uint32_t tint) {
  if (type < 0 || type >= NCP) return;
  M4 bones[NBONES];
  compute_bones(pose, bones);
  float s = HT[type].scale;
  M4 model = m4_mul(m4_translate(x, y, z), m4_mul(m4_roty(yaw), m4_scale(s, s, s)));
  r_draw_bones(&HT[type].body, &model, bones, NBONES, tint);
  if (pose->weapon > 0 && pose->weapon < 4 && (pose->anim == AN_AIM || pose->anim == AN_SHOOT || pose->anim == AN_WALK || pose->anim == AN_RUN || pose->anim == AN_IDLE))
    r_draw_bones(&HT[type].guns[pose->weapon], &model, bones, NBONES, tint);
}

/* ---------------------------------------------------------------- portrety */
void humans_portraits_build(void) {
  for (int t = 0; t < NCP; t++) {
    RTarget rt = rt_create(256, 256);
    HumanPose p;
    memset(&p, 0, sizeof p);
    p.anim = AN_IDLE;
    r_portrait_begin(&rt, v3(0.07f, 0.86f, 0.5f), v3(0, 0.825f, 0), 26.0f);
    human_draw(t, 0, 0, 0, 0.18f, &p, 0);
    r_portrait_end();
    HT[t].portrait = rt.tex;
  }
}
unsigned human_portrait(int type) { return type >= 0 && type < NCP ? HT[type].portrait : 0; }
