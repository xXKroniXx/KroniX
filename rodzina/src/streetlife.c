/* Życie ulicy w otwartym mieście: przechodnie spacerujący chodnikami i kolejka nadziemna („the L”)
 * kursująca po estakadzie nad Loopem. Wszystko dynamiczne, tylko w zasięgu gracza. */
#include "engine.h"
#include "assets.h"

int city_tile(int x, int z);
int city_rail(float *x0, float *x1, float *zc);

/* ---------------------------------------------------------------- przechodnie */
#define MAXPED 44
typedef struct {
  float x, z, ang, speed, animT, pause;
  int dx, dz;      /* kierunek marszu (oś) */
  int type, alive, tx, tz; /* kafel docelowy */
  float lane;      /* przesunięcie w poprzek chodnika */
} Ped;
static Ped peds[MAXPED];
static int pedTypes[8], npedTypes, active;
static unsigned rs = 12345;
static int rnd(int n) { rs = rs * 1103515245u + 12345u; return (int)((rs >> 16) % (unsigned)n); }

static int walkable(int x, int z) { return city_tile(x, z) == ','; }

static int spawn_ped(Ped *p, float cx, float cz, float rmin, float rmax) {
  for (int tries = 0; tries < 40; tries++) {
    float a = rnd(6283) / 1000.0f, r = rmin + rnd(1000) / 1000.0f * (rmax - rmin);
    int x = (int)floorf(cx + cosf(a) * r), z = (int)floorf(cz + sinf(a) * r);
    if (!walkable(x, z)) continue;
    static const int D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    int k = rnd(4), ok = 0;
    for (int j = 0; j < 4; j++) { int kk = (k + j) % 4; if (walkable(x + D[kk][0], z + D[kk][1])) { k = kk; ok = 1; break; } }
    if (!ok) continue;
    memset(p, 0, sizeof *p);
    p->x = x + 0.5f; p->z = z + 0.5f;
    p->dx = D[k][0]; p->dz = D[k][1];
    p->tx = x + p->dx; p->tz = z + p->dz;
    p->lane = (rnd(100) - 50) / 250.0f;
    p->speed = 0.85f + rnd(100) / 250.0f;
    p->type = pedTypes[rnd(npedTypes)];
    p->animT = rnd(1000) / 100.0f;
    p->ang = atan2f((float)p->dx, (float)p->dz);
    p->alive = 1;
    return 1;
  }
  return 0;
}

void streetlife_init(void) {
  MapDef *m = &S.maps[W.map];
  active = !m->interior && m->w * m->h > 4000 && S.nregions > 0;
  memset(peds, 0, sizeof peds);
  if (!active) return;
  static const char *names[] = {"pan", "pani", "robotnik", "helena", "flapper", "janek", "marynarz", "agent"};
  npedTypes = 0;
  for (int i = 0; i < 8; i++) { int t = human_find(names[i]); if (t >= 0) pedTypes[npedTypes++] = t; }
  if (!npedTypes) { active = 0; return; }
  rs = (unsigned)(W.map * 977 + 31);
  int n = m->rain ? MAXPED * 2 / 3 : MAXPED;
  for (int i = 0; i < n; i++) spawn_ped(&peds[i], W.px, W.pz, 3, 40);
}

static void ped_update(Ped *p, float dt) {
  /* daleko od gracza: przenieś w nowe miejsce (poza zasięgiem wzroku) */
  float ddx = p->x - W.px, ddz = p->z - W.pz;
  if (ddx * ddx + ddz * ddz > 46 * 46) { spawn_ped(p, W.px, W.pz, 28, 42); return; }
  p->animT += dt;
  if (p->pause > 0) { p->pause -= dt; return; }
  /* zatrzymanie przed graczem (uprzejmość) */
  float fx = sinf(p->ang), fz = cosf(p->ang);
  if (ddx * ddx + ddz * ddz < 1.1f && -(ddx * fx + ddz * fz) > 0) return;
  float tx = p->tx + 0.5f + (p->dz ? p->lane : 0), tz = p->tz + 0.5f + (p->dx ? p->lane : 0);
  float vx = tx - p->x, vz = tz - p->z, d = sqrtf(vx * vx + vz * vz);
  float step = p->speed * dt;
  if (d <= step) {
    p->x = tx; p->z = tz;
    /* wybór dalszej drogi: prosto, czasem skręt, zawracanie tylko w ślepym zaułku */
    static const int D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    int cx = p->tx, cz = p->tz, opts[4], no = 0, straight = -1;
    for (int k = 0; k < 4; k++) {
      if (D[k][0] == -p->dx && D[k][1] == -p->dz) continue;
      if (!walkable(cx + D[k][0], cz + D[k][1])) continue;
      if (D[k][0] == p->dx && D[k][1] == p->dz) straight = k;
      opts[no++] = k;
    }
    int k;
    if (straight >= 0 && rnd(100) < 88) k = straight;
    else if (no) k = opts[rnd(no)];
    else { p->dx = -p->dx; p->dz = -p->dz; p->tx = cx + p->dx; p->tz = cz + p->dz; p->pause = 0.5f; return; }
    p->dx = D[k][0]; p->dz = D[k][1];
    p->tx = cx + p->dx; p->tz = cz + p->dz;
    if (rnd(100) < 3) p->pause = 1.5f + rnd(300) / 100.0f; /* przystanął: witryna, gazeta */
    return;
  }
  p->x += vx / d * step; p->z += vz / d * step;
  float want = atan2f(vx, vz), da = want - p->ang;
  while (da > PI_F) da -= 2 * PI_F;
  while (da < -PI_F) da += 2 * PI_F;
  p->ang += da * fminf(1, dt * 8);
}

/* ---------------------------------------------------------------- kolejka nadziemna */
#define TRAIN_CARS 2
#define CAR_HL 1.9f   /* połowa długości wagonu */
#define CAR_HW 0.55f
typedef struct { float x, v, wait; int dir, track; float clackT; } Train;
static Train trains[2];
static GMesh trainMesh;
static int trainBuilt;
static float rX0, rX1, rZc;
static int hasRail;

static void build_train_car(MB *b) {
  float L = CAR_HL, Wd = CAR_HW, y0 = 0.0f, y1 = 1.35f;
  /* pudło wagonu: ciemna zieleń z kremowym pasem pod oknami */
  mb_paint(0xFF2E4A36, L_PAINT);
  mb_rbox(b, v3(-L, y0 + 0.15f, -Wd), v3(L, y0 + 0.62f, Wd), 0.03f);
  mb_rbox(b, v3(-L, y0 + 1.02f, -Wd), v3(L, y1, Wd), 0.03f);
  /* filary między oknami */
  for (float x = -L; x <= L - 0.3f; x += 0.55f) mb_box(b, v3(x, y0 + 0.62f, -Wd), v3(x + 0.12f, y0 + 1.02f, Wd));
  mb_box(b, v3(L - 0.12f, y0 + 0.62f, -Wd), v3(L, y0 + 1.02f, Wd));
  mb_paint(0xFFD8C8A0, L_PAINT);
  mb_box(b, v3(-L - 0.005f, y0 + 0.55f, -Wd - 0.005f), v3(L + 0.005f, y0 + 0.6f, Wd + 0.005f));
  /* okna (podświetlone wnętrze) */
  mb_paint(0xFFFFE0A8, L_WINDOW); P_.emis = 0.45f;
  mb_box(b, v3(-L + 0.02f, y0 + 0.62f, -Wd + 0.03f), v3(L - 0.02f, y0 + 1.02f, Wd - 0.03f));
  /* dach z wywietrznikiem (clerestory) */
  mb_paint(0xFF2A2A2C, L_METAL);
  mb_rbox(b, v3(-L + 0.05f, y1, -Wd + 0.02f), v3(L - 0.05f, y1 + 0.1f, Wd - 0.02f), 0.04f);
  mb_box(b, v3(-L + 0.4f, y1 + 0.1f, -0.25f), v3(L - 0.4f, y1 + 0.22f, 0.25f));
  /* czoła: drzwi przejściowe i reflektory */
  mb_paint(0xFF22382A, L_PAINT);
  for (int s = -1; s <= 1; s += 2) mb_box(b, v3(s > 0 ? L : -L - 0.02f, y0 + 0.15f, -0.2f), v3(s > 0 ? L + 0.02f : -L, y0 + 1.1f, 0.2f));
  /* wózki (bogies) */
  mb_paint(0xFF1A1A1C, L_METAL);
  for (int s = -1; s <= 1; s += 2) {
    mb_box(b, v3(s * (L - 0.55f) - 0.38f, y0 - 0.02f, -0.36f), v3(s * (L - 0.55f) + 0.38f, y0 + 0.16f, 0.36f));
    for (int w = -1; w <= 1; w += 2) mb_cyl_z(b, v3(s * (L - 0.55f) + w * 0.24f, y0 + 0.02f, 0), 0.11f, 0.62f, 10, 3);
  }
}

void streetlife_build(void) {
  if (trainBuilt) return;
  MB b;
  mb_init(&b);
  build_train_car(&b);
  trainMesh = gm_upload(&b);
  mb_free(&b);
  trainBuilt = 1;
}

static void trains_init(void) {
  hasRail = active && city_rail(&rX0, &rX1, &rZc);
  if (!hasRail) return;
  float span = TRAIN_CARS * CAR_HL * 2 + 0.1f;
  trains[0] = (Train){rX0 + span * 0.5f + 0.3f, 0, 2.0f, 1, 0, 0};
  trains[1] = (Train){rX1 - span * 0.5f - 0.3f, 0, 9.0f, -1, 1, 0};
}

static void train_update(Train *t, float dt) {
  float half = TRAIN_CARS * CAR_HL + 0.1f, a = 1.6f, vmax = 6.0f;
  float xa = rX0 + half + 0.3f, xb = rX1 - half - 0.3f, st = (rX0 + rX1) * 0.5f;
  if (t->wait > 0) {
    t->wait -= dt;
    if (t->wait <= 0 && ((t->dir > 0 && t->x >= xb - 0.05f) || (t->dir < 0 && t->x <= xa + 0.05f))) t->dir = -t->dir;
    return;
  }
  /* najbliższy przystanek przed pociągiem: stacja albo koniec toru */
  float stop = t->dir > 0 ? (t->x < st - 0.05f ? st : xb) : (t->x > st + 0.05f ? st : xa);
  float dist = (stop - t->x) * t->dir;
  if (dist <= 0.02f) {
    t->x = stop; t->v = 0;
    t->wait = fabsf(stop - st) < 0.1f ? 5.0f : 3.5f;
    return;
  }
  float vstop = sqrtf(2 * a * dist);
  float target = fminf(vmax, vstop);
  if (t->v < target) t->v = fminf(target, t->v + a * dt);
  else t->v = target;
  t->x += t->dir * t->v * dt;
  /* dźwięk przejazdu przy graczu */
  t->clackT -= dt * t->v;
  if (t->clackT <= 0 && t->v > 1.0f) {
    t->clackT = 6.0f;
    float dx = t->x - W.px, dz = rZc - W.pz, d = sqrtf(dx * dx + dz * dz);
    if (d < 34) {
      float g = fminf(1, 0.25f + t->v / vmax) * (1 - d / 34) * (1 - d / 34);
      float pan = 0;
      float rx = cosf(W.yaw), rz = -sinf(W.yaw); /* prawa strona kamery */
      if (d > 0.5f) pan = (dx * rx + dz * rz) / d;
      sfx_play_pan(SFX_TRAIN, g, pan * 0.5f);
    }
  }
}

/* ---------------------------------------------------------------- API */
void streetlife_map(void) {
  streetlife_init();
  trains_init();
}

void streetlife_update(void) {
  if (!active) return;
  float dt = 1.0f / 60;
  for (int i = 0; i < MAXPED; i++) if (peds[i].alive) ped_update(&peds[i], dt);
  if (hasRail) for (int i = 0; i < 2; i++) train_update(&trains[i], dt);
}

void streetlife_lights(int night) {
  if (!hasRail) return;
  for (int i = 0; i < 2; i++) {
    Train *t = &trains[i];
    float dx = t->x - W.px;
    if (fabsf(dx) > 30 || fabsf(rZc - W.pz) > 30) continue;
    float z = rZc + (t->track ? 0.8f : -0.8f);
    float k = night ? 1.0f : 0.4f;
    r_light(t->x, 4.3f, z, 1.6f * k, 1.3f * k, 0.8f * k, 4.5f);
  }
}

void streetlife_draw(void) {
  if (!active) return;
  V3 c = g_camPos;
  float fx = sinf(W.yaw), fz = cosf(W.yaw);
  for (int i = 0; i < MAXPED; i++) {
    Ped *p = &peds[i];
    if (!p->alive) continue;
    float dx = p->x - c.x, dz = p->z - c.z, d2 = dx * dx + dz * dz;
    if (d2 > 42 * 42) continue;
    if (d2 > 4 && dx * fx + dz * fz < -2) continue; /* za plecami */
    HumanPose hp;
    memset(&hp, 0, sizeof hp);
    hp.anim = p->pause > 0 ? AN_IDLE : AN_WALK;
    hp.t = p->animT * p->speed * 1.1f;
    human_draw(p->type, p->x, world_ground(p->x, p->z), p->z, p->ang, &hp, 0);
  }
  if (!hasRail || !trainBuilt) return;
  for (int i = 0; i < 2; i++) {
    Train *t = &trains[i];
    float z = rZc + (t->track ? 0.8f : -0.8f);
    float sway = sinf(g_time * 5.3f + i) * 0.006f * fminf(1, t->v);
    for (int k = 0; k < TRAIN_CARS; k++) {
      float x = t->x + (k - (TRAIN_CARS - 1) * 0.5f) * (CAR_HL * 2 + 0.1f);
      float ddx = x - c.x, ddz = z - c.z;
      if (ddx * ddx + ddz * ddz > 80 * 80) continue;
      M4 m = m4_mul(m4_translate(x, 3.62f, z), m4_rotx(sway));
      r_draw(&trainMesh, &m, 0);
    }
    /* reflektor czołowy i czerwone światła końcowe */
    float head = t->x + t->dir * (TRAIN_CARS * (CAR_HL + 0.05f) + 0.03f), tail = t->x - t->dir * (TRAIN_CARS * (CAR_HL + 0.05f) + 0.03f);
    r_glow(head, 4.05f, z, 0.45f, 1.2f, 1.05f, 0.75f);
    r_glow(tail, 4.05f, z - 0.3f, 0.18f, 1.0f, 0.1f, 0.05f);
    r_glow(tail, 4.05f, z + 0.3f, 0.18f, 1.0f, 0.1f, 0.05f);
    /* iskry z szyny przy hamowaniu */
    if (t->v > 0.5f && t->v < 3.0f && fmodf(g_time * 13 + i, 1.0f) < 0.3f)
      r_glow(t->x - t->dir * (CAR_HL - 0.5f), 3.62f, z + 0.25f, 0.12f, 1.6f, 1.1f, 0.5f);
  }
}
