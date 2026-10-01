/* Tycoon mafii: „Księga Rodziny”.
 * Po przejęciu rodziny gracz zarządza imperium dzień po dniu: dzielnice Chicago,
 * cztery rywalizujące organizacje, biznesy, ludzie, akcje (także prowadzone osobiście w 3D),
 * dyplomacja, policja i federalne śledztwo podatkowe. */
#include "engine.h"
#include <stdarg.h>
#include <ctype.h>

/* ================================================================ dane stałe */
#define ND 8          /* dzielnice */
#define NF 5          /* 0 = gracz, 1..4 rywale */
#define MAXBIZ 40
#define MAXMEN 12
#define MAXREC 4
#define MAXOPS 8
#define NLOG 90
#define NTODAY 24
#define MAXEV 8

enum { B_MELINA, B_KASYNO, B_BUKMACHER, B_KLUB, B_BROWAR, B_BIMBER, B_PORT, B_PRALNIA, B_RESTAUR, B_LOMBARD, B_BOKS, B_N };
typedef struct { const char *name, *desc; int cost, income, heat, boozeUse, boozeMake, dockOnly; } BizType;
static const BizType BT[B_N] = {
  {"Melina", "Nielegalny bar. Zużywa 2 beczki alkoholu dziennie.", 800, 130, 1, 2, 0, 0},
  {"Kasyno", "Ruletka i karty na zapleczu. Duży zysk, sporo hałasu.", 2200, 280, 2, 1, 0, 0},
  {"Bukmacher", "Zakłady na wyścigi i walki. Pewny grosz.", 1200, 160, 1, 0, 0, 0},
  {"Klub jazzowy", "Elegancki lokal dla śmietanki. Zużywa 3 beczki.", 3200, 380, 1, 3, 0, 0},
  {"Browar", "Warzy piwo: +8 beczek dziennie.", 2400, 30, 2, 0, 8, 0},
  {"Bimbrownia", "Pędzi whisky w piwnicy: +5 beczek dziennie.", 1300, 20, 1, 0, 5, 0},
  {"Port przemytniczy", "Tylko w Dokach. Kanadyjski towar: +12 beczek.", 3600, 120, 2, 0, 12, 1},
  {"Pralnia", "Pierze brudne pieniądze. Zmniejsza gorączkę i śledztwo.", 1800, 40, -3, 0, 0, 0},
  {"Restauracja", "Legalna przykrywka. Zmniejsza gorączkę, daje szacunek.", 1000, 70, -2, 0, 0, 0},
  {"Lombard", "Skupuje fanty bez zadawania pytań.", 900, 95, 0, 0, 0, 0},
  {"Hala bokserska", "Walki i zakłady. Przyciąga twardszych rekrutów.", 1700, 150, 1, 0, 0, 0},
};
static const float LVLMULT[4] = {0, 1.0f, 1.6f, 2.3f};

typedef struct { const char *name; int x, y, w, h, wealth, slots, police, dock; int infl[NF]; } DistTpl;
static const DistTpl DT[ND] = {
  {"Mała Italia", 70, 94, 88, 56, 3, 4, 2, 0, {60, 0, 15, 0, 0}},
  {"Polonia", 70, 152, 88, 94, 2, 4, 1, 0, {25, 15, 0, 0, 0}},
  {"Doki", 200, 44, 50, 48, 3, 3, 1, 1, {0, 20, 45, 0, 0}},
  {"Śródmieście", 160, 94, 90, 46, 5, 5, 4, 0, {0, 15, 15, 0, 35}},
  {"Chinatown", 160, 142, 90, 34, 3, 3, 1, 0, {0, 0, 0, 70, 0}},
  {"North Side", 70, 44, 128, 48, 4, 4, 2, 0, {0, 70, 0, 0, 0}},
  {"Cicero", 10, 44, 58, 156, 3, 4, 1, 0, {0, 0, 0, 0, 60}},
  {"Levee", 160, 178, 90, 68, 2, 4, 2, 0, {0, 0, 60, 10, 0}},
};

typedef struct { const char *name, *boss, *sprite, *id, *adj; int soldiers, cash, aggr, hq; char col; } FamTpl;
static const FamTpl FT[NF] = {
  {"Rodzina", "Ty", "tomek", "ty", "Twoja", 0, 0, 0, 0, 'z'},
  {"Irlandczycy", "Liam O'Donnell", "sean", "irl", "irlandzka", 30, 3000, 50, 5, 'g'},
  {"Rodzina Russo", "Carmine Russo", "russo", "rus", "Russo", 40, 4000, 65, 7, 'r'},
  {"Triada", "Lee Wong", "lee", "tri", "chińska", 25, 2500, 30, 4, 'o'},
  {"Syndykat Kane'a", "Victor Kane", "kane", "kan", "Kane'a", 20, 8000, 25, 6, 'c'},
};
enum { F_NEUTRAL, F_WAR, F_TRUCE, F_ALLY, F_VASSAL, F_ABSORBED, F_DEAD };
static const char *FSTATE[] = {"neutralność", "WOJNA", "rozejm", "sojusz", "lennik", "przyłączeni", "rozbici"};

enum { OP_HARACZ, OP_PRZEMYT, OP_NAPAD, OP_SABOTAZ, OP_ATAK, OP_LAPOWKA, OP_ZAMACH, OP_WERBUNEK, OPT_N };
enum { TG_NONE, TG_DIST, TG_FAM, TG_HEIST };
typedef struct { const char *name, *desc; int days, tgt, fightOp, minTeam; const char *map; } OpType;
static const OpType OT[OPT_N] = {
  {"Haracz", "Wymuś opłaty za „ochronę” w dzielnicy. +wpływy, +kasa.", 1, TG_DIST, 1, 1, "op_ulica"},
  {"Przemyt", "Przerzut kanadyjskiej whisky. +beczki alkoholu.", 2, TG_NONE, 0, 1, NULL},
  {"Napad", "Ciężarówka, pociąg pocztowy albo bank. Duża kasa, duży hałas.", 2, TG_HEIST, 1, 2, "op_bank"},
  {"Sabotaż", "Spal magazyn rywala. Osłabia rodzinę, psuje stosunki.", 1, TG_FAM, 1, 1, "op_magazyn"},
  {"Atak na dzielnicę", "Wyrzuć rywala siłą. Duży zysk wpływów, oznacza wojnę.", 1, TG_DIST, 1, 2, "op_ulica"},
  {"Łapówki", "Koperty dla policji i radnych ($600). Mniej gorączki i śledztwa.", 1, TG_NONE, 0, 1, NULL},
  {"Zamach na bossa", "Zabij szefa rodziny. Rodzina może się rozpaść.", 2, TG_FAM, 1, 2, "op_hq"},
  {"Werbunek", "Przeczesz bary i hale. Nowi żołnierze za darmo.", 1, TG_NONE, 0, 1, NULL},
};
static const char *HEIST[3] = {"Ciężarówka z forsą", "Pociąg pocztowy", "First National Bank"};
static const int HEIST_DIFF[3] = {60, 110, 170}, HEIST_MIN[3] = {800, 2000, 4000}, HEIST_MAX[3] = {1400, 3200, 7000}, HEIST_HEAT[3] = {10, 18, 30};

static const char *FIRST[] = {"Tony", "Sal", "Vinnie", "Paulie", "Gino", "Frankie", "Joey", "Nico", "Marek", "Staszek", "Wojtek", "Józek",
                              "Franek", "Mickey", "Danny", "Patrick", "Sean", "Lou", "Carlo", "Benny", "Rocco", "Leon", "Edek", "Kazik"};
static const char *LAST[] = {"Esposito", "Romano", "Bianchi", "Greco", "Moretti", "Conti", "Kowalski", "Nowak", "Wiśniewski", "Lewandowski",
                             "Mazur", "O'Brien", "Murphy", "Kelly", "Ricci", "Marino", "Costa", "Zieliński", "Dudek", "Barzini", "Lucca", "Fontana"};
static const char *NICK[] = {"Młot", "Cichy", "Książę", "Brzytwa", "Profesor", "Szczęściarz", "Byk", "Kulawy", "Lis", "Pięść", "Okularnik",
                             "Rzeźnik", "Śliski", "Kaznodzieja", "Dzieciak", "Grabarz", "Wilk", "Lalka", "Sęp", "Iskra"};
static const char *MSPR[] = {"mafioso", "zbir", "zbir2", "dokowiec", "bokser", "gino", "enzo", "sal", "mickey", "stefek", "robotnik", "lucky"};
#define NELEM(a) ((int)(sizeof(a) / sizeof((a)[0])))

static const char *MONTHS[12] = {"stycznia", "lutego", "marca", "kwietnia", "maja", "czerwca", "lipca", "sierpnia", "września", "października", "listopada", "grudnia"};
static const int MDAYS[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#define REPEAL_DAY 187 /* 5 grudnia 1933: koniec prohibicji */

/* ================================================================ stan */
enum { MS_OK, MS_HURT, MS_JAIL, MS_OP };
typedef struct {
  char name[28], nick[16], sprite[16];
  int fight, brains, loyal, wage, xp, lvl, status, statusT, assign, capo, ops, kills;
} Man;
typedef struct { int type, dist, lvl, manager, closed; long earned; } Biz;
typedef struct { int infl[NF], fort, owner; } Dist;
typedef struct { int soldiers, cash, rel, state, stateT, aggr, leaderless, vassalDays, bossDead; } Fam;
typedef struct { int type, target, days, team[4], nteam, soldiers, active; } Op;
typedef struct { int id, a, b; } Ev;

static struct {
  int active, day, cash_unused, heat, invest, respect, booze, soldiers, debtDays, politician;
  int lastIncome, lastCosts, lastBooze, repealed, ended, legalBrew;
  Dist d[ND];
  Fam f[NF];
  Biz biz[MAXBIZ]; int nbiz;
  Man men[MAXMEN]; int nmen;
  Man rec[MAXREC]; int nrec;
  Op ops[MAXOPS];
  char log[NLOG][100]; int nlog;
  char today[NTODAY][100]; int ntoday;
  Ev ev[MAXEV]; int nev;
  int warnInvest;
  /* v2: kronika */
  int beats, visitDay, newsWeek, lastWar;
} T;

/* akcja prowadzona osobiście w 3D */
static struct { int active, op, endT, n; int ent[8], man[8]; } MS;

/* ================================================================ narzędzia */
static int rnd(int n) { return n > 0 ? rand() % n : 0; }
static int rr(int a, int b) { return a + rnd(b - a + 1); }

static char tlogHeader[100]; /* nagłówek dnia — wpisywany dopiero przy pierwszym zdarzeniu */
static void tlog_raw(const char *b) {
  if (T.nlog >= NLOG) { memmove(T.log[0], T.log[1], sizeof(T.log[0]) * (NLOG - 1)); T.nlog = NLOG - 1; }
  snprintf(T.log[T.nlog++], 100, "%s", b);
}
static void tlog(const char *fmt, ...) {
  if (tlogHeader[0]) { char h[100]; snprintf(h, sizeof h, "%s", tlogHeader); tlogHeader[0] = 0; tlog_raw(h); }
  char b[100];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(b, sizeof b, fmt, ap);
  va_end(ap);
  if (T.nlog >= NLOG) { memmove(T.log[0], T.log[1], sizeof(T.log[0]) * (NLOG - 1)); T.nlog = NLOG - 1; }
  snprintf(T.log[T.nlog++], 100, "%s", b);
  if (T.ntoday < NTODAY) snprintf(T.today[T.ntoday++], 100, "%s", b);
}

static void push_ev(int id, int a, int b) {
  if (T.nev >= MAXEV) return;
  for (int i = 0; i < T.nev; i++) if (T.ev[i].id == id && T.ev[i].a == a) return;
  T.ev[T.nev++] = (Ev){id, a, b};
}

static void date_str(int day, char *out, int n) {
  int y = 1933, m = 5, d = 1 + day;
  while (d > MDAYS[m]) { d -= MDAYS[m]; if (++m >= 12) { m = 0; y++; } }
  snprintf(out, n, "%d %s %d", d, MONTHS[m], y);
}

static int fam_alive(int f) { return f > 0 && T.f[f].state < F_ABSORBED; }
static int dist_count(int f) { int n = 0; for (int d = 0; d < ND; d++) if (T.d[d].owner == f) n++; return n; }
static int men_free(void) { int n = 0; for (int i = 0; i < T.nmen; i++) if (T.men[i].status == MS_OK && T.men[i].assign < 0) n++; return n; }
static int biz_in(int d) { int n = 0; for (int i = 0; i < T.nbiz; i++) if (T.biz[i].dist == d) n++; return n; }
static int max_soldiers(void) { return 20 + dist_count(0) * 6 + T.respect / 4; }
static int player_power(void) {
  int p = T.soldiers * 2 + T.respect / 4 + dist_count(0) * 10;
  for (int i = 0; i < T.nmen; i++) if (T.men[i].status != MS_JAIL) p += T.men[i].fight * 3 + T.men[i].lvl * 2 + (T.men[i].capo ? 5 : 0);
  return p;
}
static int fam_power(int f) {
  int p = T.f[f].soldiers * 2 + (T.f[f].cash > 0 ? T.f[f].cash / 200 : 0) + dist_count(f) * 10 + (T.f[f].bossDead ? 0 : 15);
  return p;
}

static void recompute_owners(int verbose) {
  for (int d = 0; d < ND; d++) {
    int best = -1, bv = 0;
    for (int f = 0; f < NF; f++) if (T.d[d].infl[f] > bv) { bv = T.d[d].infl[f]; best = f; }
    int owner = bv >= 40 ? best : -1;
    if (owner != T.d[d].owner && verbose) {
      if (owner == 0) tlog("%s jest teraz pod kontrolą Rodziny!", DT[d].name);
      else if (T.d[d].owner == 0) tlog("Straciliśmy kontrolę nad dzielnicą %s.", DT[d].name);
      else if (owner > 0) tlog("%s przejmują dzielnicę %s.", FT[owner].name, DT[d].name);
    }
    T.d[d].owner = owner;
  }
}

static int infl_free(int d) {
  int s = 0;
  for (int f = 0; f < NF; f++) s += T.d[d].infl[f];
  return 100 - s;
}

/* przenosi wpływy do rodziny `to`: najpierw z puli neutralnej, potem od rywali (`from` < 0 = od wszystkich) */
static int infl_take(int d, int to, int amount, int from) {
  int got = 0;
  if (from < 0) {
    int fr = infl_free(d);
    int a = fr < amount ? fr : amount;
    T.d[d].infl[to] += a; got += a; amount -= a;
    while (amount > 0) {
      int best = -1, bv = 0;
      for (int f = 0; f < NF; f++) if (f != to && T.d[d].infl[f] > bv) { bv = T.d[d].infl[f]; best = f; }
      if (best < 0) break;
      int k = bv < amount ? bv : amount;
      T.d[d].infl[best] -= k; T.d[d].infl[to] += k; got += k; amount -= k;
    }
  } else {
    int k = T.d[d].infl[from] < amount ? T.d[d].infl[from] : amount;
    T.d[d].infl[from] -= k; T.d[d].infl[to] += k; got = k;
  }
  return got;
}

static int strongest_rival_in(int d) {
  int best = -1, bv = 0;
  for (int f = 1; f < NF; f++) if (fam_alive(f) && T.d[d].infl[f] > bv) { bv = T.d[d].infl[f]; best = f; }
  return best;
}

static int dist_defense(int d) {
  int n = 0;
  for (int k = 0; k < ND; k++) if (T.d[k].infl[0] >= 25) n++;
  int def = 8 + T.d[d].fort * 15 + (n ? T.soldiers / n : 0) * 4;
  for (int i = 0; i < T.nmen; i++)
    if (T.men[i].assign == 1000 + d && T.men[i].status == MS_OK) def += T.men[i].fight * 5 + T.men[i].lvl * 3;
  return def;
}

static void make_man(Man *m, int quality) {
  memset(m, 0, sizeof *m);
  snprintf(m->name, sizeof m->name, "%s %s", FIRST[rnd(NELEM(FIRST))], LAST[rnd(NELEM(LAST))]);
  snprintf(m->nick, sizeof m->nick, "%s", NICK[rnd(NELEM(NICK))]);
  snprintf(m->sprite, sizeof m->sprite, "%s", MSPR[rnd(NELEM(MSPR))]);
  m->fight = CLAMP(rr(1, 4) + quality / 2 + rnd(2), 1, 10);
  m->brains = CLAMP(rr(1, 4) + quality / 2 + rnd(2), 1, 10);
  m->loyal = rr(45, 75);
  m->lvl = 1;
  m->wage = 10 + (m->fight + m->brains) * 3;
  m->assign = -1;
}
static int hire_cost(const Man *m) { return (m->fight + m->brains) * 45 + m->lvl * 60; }

static void refresh_recruits(void) {
  int q = T.respect / 25;
  for (int i = 0; i < T.nbiz; i++) if (T.biz[i].type == B_BOKS) q++;
  T.nrec = MAXREC;
  for (int i = 0; i < MAXREC; i++) {
    make_man(&T.rec[i], q);
    for (int b = 0; b < T.nbiz; b++) if (T.biz[b].type == B_BOKS) { T.rec[i].fight = CLAMP(T.rec[i].fight + 1, 1, 10); break; }
  }
}

static void remove_man(int i) {
  for (int b = 0; b < T.nbiz; b++) {
    if (T.biz[b].manager == i) T.biz[b].manager = -1;
    else if (T.biz[b].manager > i) T.biz[b].manager--;
  }
  for (int o = 0; o < MAXOPS; o++)
    for (int k = 0; k < T.ops[o].nteam; k++) if (T.ops[o].team[k] > i) T.ops[o].team[k]--;
  for (int k = i; k < T.nmen - 1; k++) T.men[k] = T.men[k + 1];
  T.nmen--;
}

static void unassign(int i) {
  Man *m = &T.men[i];
  if (m->assign >= 0 && m->assign < 1000) T.biz[m->assign].manager = -1;
  m->assign = -1;
}

/* ================================================================ dochód */
static int biz_income(int i, int boozeOk) {
  Biz *b = &T.biz[i];
  if (b->closed > 0) return 0;
  const BizType *t = &BT[b->type];
  float inc = t->income * 0.7f * LVLMULT[b->lvl] * (0.6f + 0.2f * DT[b->dist].wealth);
  if (b->manager >= 0) inc *= 1.0f + T.men[b->manager].brains * 0.06f;
  if (t->boozeUse && !boozeOk) inc *= 0.4f;
  int owner = T.d[b->dist].owner;
  if (owner > 0) inc *= 0.7f;
  if (T.repealed) {
    if (b->type == B_MELINA) inc *= 0.5f;
    if (b->type == B_KLUB) inc *= 0.8f;
    if ((b->type == B_BROWAR || b->type == B_BIMBER) && T.legalBrew) inc += 150 * LVLMULT[b->lvl];
  }
  return (int)inc;
}

static int biz_heat(int i) {
  Biz *b = &T.biz[i];
  if (b->closed > 0) return 0;
  int h = BT[b->type].heat;
  if (T.repealed && T.legalBrew && (b->type == B_BROWAR || b->type == B_BIMBER)) h = 0;
  return h;
}

static int fam_income(int f) {
  int inc = 160;
  for (int d = 0; d < ND; d++) inc += T.d[d].infl[f] * DT[d].wealth * 90 / 100;
  return inc;
}

/* ================================================================ akcje gracza (wspólne dla UI i symulacji) */
enum { R_OK, R_NOCASH, R_NOSLOT, R_NOINFL, R_INVALID, R_FULL, R_REFUSED };

static int build_cost(int type) { return BT[type].cost; }
static int can_build(int d, int type) {
  if (BT[type].dockOnly && !DT[d].dock) return R_INVALID;
  if (T.d[d].infl[0] < 25) return R_NOINFL;
  if (biz_in(d) >= DT[d].slots) return R_NOSLOT;
  if (T.nbiz >= MAXBIZ) return R_FULL;
  if (H.gold < build_cost(type)) return R_NOCASH;
  return R_OK;
}
static int t_build(int d, int type) {
  int r = can_build(d, type);
  if (r) return r;
  H.gold -= build_cost(type);
  Biz *b = &T.biz[T.nbiz++];
  memset(b, 0, sizeof *b);
  b->type = type; b->dist = d; b->lvl = 1; b->manager = -1;
  tlog("Otwarto: %s (%s).", BT[type].name, DT[d].name);
  infl_take(d, 0, 4, -1);
  return R_OK;
}
static int upgrade_cost(int i) { return (int)(BT[T.biz[i].type].cost * (T.biz[i].lvl == 1 ? 0.8f : 1.3f)); }
static int t_upgrade(int i) {
  if (T.biz[i].lvl >= 3) return R_INVALID;
  if (H.gold < upgrade_cost(i)) return R_NOCASH;
  H.gold -= upgrade_cost(i);
  T.biz[i].lvl++;
  tlog("%s (%s) rozbudowany do poziomu %d.", BT[T.biz[i].type].name, DT[T.biz[i].dist].name, T.biz[i].lvl);
  return R_OK;
}
static int sell_value(int i) { return BT[T.biz[i].type].cost * T.biz[i].lvl / 2; }
static void biz_remove(int i) {
  if (T.biz[i].manager >= 0) T.men[T.biz[i].manager].assign = -1;
  for (int m = 0; m < T.nmen; m++) if (T.men[m].assign > i && T.men[m].assign < 1000) T.men[m].assign--;
  for (int k = i; k < T.nbiz - 1; k++) T.biz[k] = T.biz[k + 1];
  T.nbiz--;
}
static void t_sell(int i) {
  H.gold += sell_value(i);
  tlog("Sprzedano: %s (%s).", BT[T.biz[i].type].name, DT[T.biz[i].dist].name);
  biz_remove(i);
}
static void t_manager(int man, int bi) {
  unassign(man);
  if (T.biz[bi].manager >= 0) T.men[T.biz[bi].manager].assign = -1;
  T.biz[bi].manager = man;
  T.men[man].assign = bi;
}
static void t_guard(int man, int d) { unassign(man); T.men[man].assign = 1000 + d; }
static int fort_cost(int d) { return 700 * (T.d[d].fort + 1); }
static int t_fort(int d) {
  if (T.d[d].fort >= 3) return R_INVALID;
  if (H.gold < fort_cost(d)) return R_NOCASH;
  H.gold -= fort_cost(d);
  T.d[d].fort++;
  tlog("Wzmocniono ochronę w dzielnicy %s (poziom %d).", DT[d].name, T.d[d].fort);
  return R_OK;
}
static int t_hire(int r) {
  if (r < 0 || r >= T.nrec) return R_INVALID;
  if (T.nmen >= MAXMEN) return R_FULL;
  if (H.gold < hire_cost(&T.rec[r])) return R_NOCASH;
  H.gold -= hire_cost(&T.rec[r]);
  T.men[T.nmen++] = T.rec[r];
  tlog("%s „%s” dołącza do Rodziny.", T.rec[r].name, T.rec[r].nick);
  for (int k = r; k < T.nrec - 1; k++) T.rec[k] = T.rec[k + 1];
  T.nrec--;
  return R_OK;
}
#define SOLDIER_PACK 5
#define SOLDIER_COST 600
static int t_soldiers(void) {
  if (T.soldiers + SOLDIER_PACK > max_soldiers()) return R_FULL;
  if (H.gold < SOLDIER_COST) return R_NOCASH;
  H.gold -= SOLDIER_COST;
  T.soldiers += SOLDIER_PACK;
  return R_OK;
}

static int op_diff(int type, int target) {
  switch (type) {
    case OP_HARACZ: {
      int r = 0;
      for (int f = 1; f < NF; f++) if (fam_alive(f)) r += T.d[target].infl[f];
      return 18 + r * 6 / 10 + DT[target].police * 4;
    }
    case OP_PRZEMYT: return 30 + T.heat / 2;
    case OP_NAPAD: return HEIST_DIFF[target];
    case OP_SABOTAZ: return 30 + T.f[target].soldiers * 8 / 10;
    case OP_ATAK: {
      int f = strongest_rival_in(target);
      return 25 + (f > 0 ? T.d[target].infl[f] * T.f[f].soldiers / 35 : 0) + T.d[target].fort * 10;
    }
    case OP_LAPOWKA: return 30 + T.invest / 3;
    case OP_ZAMACH: return 80 + T.f[target].soldiers * 2;
    case OP_WERBUNEK: return 20;
  }
  return 50;
}
static int team_power(int type, const int *team, int n, int soldiers) {
  int p = soldiers * 4;
  for (int i = 0; i < n; i++) {
    const Man *m = &T.men[team[i]];
    if (OT[type].fightOp) p += m->fight * 6 + m->brains * 2 + m->lvl * 3 + (m->capo ? 8 : 0);
    else p += m->brains * 7 + m->fight * 2 + m->lvl * 3 + (m->capo ? 8 : 0);
  }
  return p;
}
static int op_chance(int type, int target, const int *team, int n, int soldiers) {
  int p = team_power(type, team, n, soldiers), d = op_diff(type, target);
  int c = p * 100 / (p + d);
  return CLAMP(c, 5, 95);
}
static int op_target_valid(int type, int t) {
  switch (OT[type].tgt) {
    case TG_DIST:
      if (t < 0 || t >= ND) return 0;
      if (type == OP_HARACZ) return T.d[t].infl[0] < 90;
      return strongest_rival_in(t) > 0;
    case TG_FAM:
      if (!fam_alive(t)) return 0;
      if (type == OP_ZAMACH && T.f[t].bossDead) return 0;
      return T.f[t].state != F_ALLY && T.f[t].state != F_VASSAL;
    case TG_HEIST: return t >= 0 && t < 3;
    default: return t == 0;
  }
}
static int ops_active(void) { int n = 0; for (int o = 0; o < MAXOPS; o++) if (T.ops[o].active) n++; return n; }

static int t_launch(int type, int target, const int *team, int n, int soldiers) {
  if (n < OT[type].minTeam && soldiers < 3) return R_INVALID;
  if (!op_target_valid(type, target)) return R_INVALID;
  if (type == OP_LAPOWKA && H.gold < 600) return R_NOCASH;
  int o;
  for (o = 0; o < MAXOPS; o++) if (!T.ops[o].active) break;
  if (o >= MAXOPS) return R_FULL;
  if (soldiers > T.soldiers) soldiers = T.soldiers;
  if (type == OP_LAPOWKA) H.gold -= 600;
  Op *op = &T.ops[o];
  memset(op, 0, sizeof *op);
  op->type = type; op->target = target; op->days = OT[type].days; op->soldiers = soldiers; op->active = 1;
  for (int i = 0; i < n && i < 4; i++) { op->team[op->nteam++] = team[i]; unassign(team[i]); T.men[team[i]].status = MS_OP; }
  T.soldiers -= soldiers;
  return R_OK;
}

static void rel_add(int f, int v) { T.f[f].rel = CLAMP(T.f[f].rel + v, -100, 100); }
static void go_war(int f) {
  if (T.f[f].state == F_WAR || !fam_alive(f)) return;
  T.f[f].state = F_WAR;
  tlog("%s wypowiadają nam wojnę!", FT[f].name);
}

static void family_collapse(int f, int byPlayer) {
  T.f[f].state = F_DEAD;
  tlog("%s przestają istnieć!", FT[f].name);
  for (int d = 0; d < ND; d++) {
    int v = T.d[d].infl[f];
    T.d[d].infl[f] = 0;
    if (byPlayer) T.d[d].infl[0] += v / 2;
  }
  T.respect = CLAMP(T.respect + 15, 0, 100);
  char sc[40];
  snprintf(sc, sizeof sc, "tyc_koniec_%s", FT[f].id);
  if (script_find(sc) >= 0) push_ev(100, f, 0);
}

/* wynik akcji: success 0/1; personal = prowadzona osobiście (ludzie bezpieczniejsi) */
static void op_resolve(Op *op, int success, int personal) {
  int type = op->type, t = op->target;
  char tn[48] = "";
  if (OT[type].tgt == TG_DIST) snprintf(tn, sizeof tn, " (%s)", DT[t].name);
  else if (OT[type].tgt == TG_FAM) snprintf(tn, sizeof tn, " (%s)", FT[t].name);
  else if (OT[type].tgt == TG_HEIST) snprintf(tn, sizeof tn, " (%s)", HEIST[t]);
  if (success) {
    if (type != OP_HARACZ || rnd(2)) T.respect = CLAMP(T.respect + 1, 0, 100);
    switch (type) {
      case OP_HARACZ: {
        int f = strongest_rival_in(t);
        int g = infl_take(t, 0, rr(5, 11), -1);
        int c = 40 * DT[t].wealth + rnd(80);
        H.gold += c; T.heat += 3;
        if (f > 0) rel_add(f, -5);
        tlog("Haracz%s: +%d wpływów, +$%d.", tn, g, c);
        break;
      }
      case OP_PRZEMYT: { int b = rr(25, 45); T.booze += b; T.heat += 4; tlog("Przemyt udany: +%d beczek whisky.", b); break; }
      case OP_NAPAD: { int c = rr(HEIST_MIN[t], HEIST_MAX[t]); H.gold += c; T.heat += HEIST_HEAT[t]; T.respect = CLAMP(T.respect + 3, 0, 100); tlog("Napad%s: łup $%d!", tn, c); break; }
      case OP_SABOTAZ: {
        int s = rr(3, 6), c = T.f[t].cash / 5;
        T.f[t].soldiers -= s; T.f[t].cash -= c; rel_add(t, -15); T.heat += 6;
        tlog("Sabotaż%s: spłonął ich magazyn, -%d ludzi, -$%d.", tn, s, c);
        if (T.f[t].rel < -40) go_war(t);
        break;
      }
      case OP_ATAK: {
        int f = strongest_rival_in(t);
        int g = f > 0 ? infl_take(t, 0, rr(20, 35), f) : infl_take(t, 0, 20, -1);
        if (f > 0) { int s = rr(3, 6); T.f[f].soldiers -= s; rel_add(f, -25); go_war(f); }
        T.heat += 8;
        tlog("Atak%s: przejęto %d wpływów!", tn, g);
        break;
      }
      case OP_LAPOWKA: T.heat -= 25; T.invest -= 8; tlog("Łapówki trafiły do właściwych kieszeni. Policja patrzy w inną stronę."); break;
      case OP_ZAMACH: {
        T.f[t].bossDead = 1; T.f[t].leaderless = 10; T.f[t].soldiers /= 2; T.heat += 20;
        T.respect = CLAMP(T.respect + 10, 0, 100);
        for (int f = 1; f < NF; f++) if (f != t && fam_alive(f)) rel_add(f, -10);
        tlog("Zamach udany: %s nie żyje! %s w rozsypce.", FT[t].boss, FT[t].name);
        go_war(t);
        if (T.f[t].soldiers < 10) family_collapse(t, 1);
        break;
      }
      case OP_WERBUNEK: { int s = rr(3, 6) + T.respect / 20; if (T.soldiers + op->soldiers + s > max_soldiers()) s = max_soldiers() - T.soldiers - op->soldiers; if (s < 0) s = 0; T.soldiers += s; tlog("Werbunek: %d nowych żołnierzy.", s); break; }
    }
  } else {
    T.respect = CLAMP(T.respect - 2, 0, 100);
    tlog("Akcja nieudana: %s%s.", OT[type].name, tn);
    if (type == OP_LAPOWKA) { T.heat += 10; tlog("Gliniarz okazał się uczciwy. Gorączka rośnie."); }
    if (type == OP_SABOTAZ || type == OP_ZAMACH || type == OP_ATAK) { rel_add(t >= 0 && OT[type].tgt == TG_FAM ? t : strongest_rival_in(t) > 0 ? strongest_rival_in(t) : 1, -15); }
    T.heat += OT[type].fightOp ? 5 : 3;
  }
  /* straty */
  int lost = success ? (personal ? 0 : op->soldiers * rnd(15) / 100) : op->soldiers * rr(20, 40) / 100;
  T.soldiers += op->soldiers - lost;
  if (lost) tlog("Zginęło %d żołnierzy.", lost);
  for (int k = op->nteam - 1; k >= 0; k--) {
    int mi = op->team[k];
    if (mi < 0 || mi >= T.nmen) continue;
    Man *m = &T.men[mi];
    m->status = MS_OK; m->ops++;
    int roll = rnd(100);
    int die = success ? (personal ? 0 : 2) : 8, hurt = success ? 10 : 30, jail = T.heat / 5;
    if (OT[type].fightOp && roll < die) {
      tlog("%s „%s” zginął.", m->name, m->nick);
      for (int j = 0; j < T.nmen; j++) if (j != mi) T.men[j].loyal = CLAMP(T.men[j].loyal - 3, 0, 100);
      remove_man(mi);
      continue;
    }
    if (OT[type].fightOp && roll < die + hurt) { m->status = MS_HURT; m->statusT = rr(2, 5); tlog("%s „%s” jest ranny.", m->name, m->nick); }
    else if (!success && rnd(100) < jail) { m->status = MS_JAIL; m->statusT = rr(3, 8); tlog("%s „%s” trafił do aresztu.", m->name, m->nick); }
    m->xp += success ? 2 : 1;
    m->loyal = CLAMP(m->loyal + (success ? 3 : -2), 0, 100);
    if (m->xp >= m->lvl * 5) {
      m->xp -= m->lvl * 5; m->lvl++;
      if (m->lvl % 2) m->brains = CLAMP(m->brains + 1, 1, 10); else m->fight = CLAMP(m->fight + 1, 1, 10);
      m->wage += 5;
      tlog("%s „%s” awansuje na poziom %d.", m->name, m->nick, m->lvl);
    }
  }
  op->active = 0;
  T.heat = CLAMP(T.heat, 0, 100);
  T.invest = CLAMP(T.invest, 0, 100);
}

/* dyplomacja */
enum { DP_GIFT, DP_TRUCE, DP_ALLY, DP_TRIBUTE, DP_MERGE, DP_WAR, DP_SELL, DP_N };
static const char *DPN[DP_N] = {"Wyślij prezent ($500)", "Zaproponuj rozejm", "Zaproponuj sojusz", "Zażądaj trybutu",
                                "Zaproponuj przyłączenie", "Wypowiedz wojnę", "Sprzedaj 20 beczek"};
static int dp_available(int f, int a) {
  Fam *F = &T.f[f];
  if (!fam_alive(f)) return 0;
  switch (a) {
    case DP_GIFT: return H.gold >= 500;
    case DP_TRUCE: return F->state == F_WAR;
    case DP_ALLY: return F->state != F_WAR && F->state != F_ALLY && F->state != F_VASSAL && F->rel >= 30;
    case DP_TRIBUTE: return F->state != F_VASSAL && F->state != F_ALLY && T.respect >= 35 && player_power() >= fam_power(f) * 2;
    case DP_MERGE: return (F->state == F_VASSAL && F->vassalDays >= 15 && F->rel >= 25) || (F->state == F_ALLY && F->rel >= 80 && T.respect >= 60) || player_power() >= fam_power(f) * 4;
    case DP_WAR: return F->state != F_WAR;
    case DP_SELL: return F->state == F_ALLY && T.booze >= 20;
  }
  return 0;
}
static const char *dp_hint(int f, int a) {
  static char b[120];
  Fam *F = &T.f[f];
  switch (a) {
    case DP_GIFT: return "Poprawia stosunki (+12).";
    case DP_TRUCE: snprintf(b, sizeof b, "Szansa zgody ok. %d%%. Rozejm na 14 dni.", CLAMP(30 + (F->rel + 50) / 2 + (player_power() - fam_power(f)) / 2, 5, 95)); return b;
    case DP_ALLY: return "Wymaga stosunków 30+. Sojusznicy nie atakują i pomagają.";
    case DP_TRIBUTE: return "Wymaga 2x ich siły i szacunku 35. Lennik płaci 15% dochodu.";
    case DP_MERGE: return "Lennik 15+ dni i stosunki 25+, sojusz 80+ (szacunek 60) albo 4x siły.";
    case DP_WAR: return "Otwarta wojna. Atakują częściej.";
    case DP_SELL: return "Sojusznik kupi 20 beczek po $25.";
  }
  return "";
}
static int t_diplo(int f, int a) {
  if (!dp_available(f, a)) return R_INVALID;
  Fam *F = &T.f[f];
  switch (a) {
    case DP_GIFT: H.gold -= 500; rel_add(f, F->rel > 60 ? 5 : 12); tlog("Prezent dla: %s. Stosunki %+d.", FT[f].boss, F->rel); return R_OK;
    case DP_TRUCE: {
      int c = CLAMP(30 + (F->rel + 50) / 2 + (player_power() - fam_power(f)) / 2, 5, 95);
      if (rnd(100) < c) { F->state = F_TRUCE; F->stateT = 14; rel_add(f, 10); tlog("%s zgadzają się na rozejm.", FT[f].name); return R_OK; }
      rel_add(f, -5); tlog("%s odrzucają rozejm.", FT[f].name); return R_REFUSED;
    }
    case DP_ALLY:
      if ((F->rel >= 50 && T.respect >= 30) || rnd(100) < F->rel / 2 + T.respect / 4) { F->state = F_ALLY; rel_add(f, 10); T.respect = CLAMP(T.respect + 5, 0, 100); tlog("Sojusz z: %s!", FT[f].name); return R_OK; }
      rel_add(f, -3); tlog("%s nie chcą sojuszu.", FT[f].name); return R_REFUSED;
    case DP_TRIBUTE: {
      int c = CLAMP(player_power() * 30 / (fam_power(f) + 1) - 30 + F->rel / 4, 5, 85);
      if (rnd(100) < c) { F->state = F_VASSAL; F->vassalDays = 0; rel_add(f, -10); T.respect = CLAMP(T.respect + 8, 0, 100); tlog("%s płacą nam trybut!", FT[f].name); return R_OK; }
      rel_add(f, -20); tlog("%s wyśmiali nasze żądanie.", FT[f].name);
      if (F->rel < -40) go_war(f);
      return R_REFUSED;
    }
    case DP_MERGE: {
      F->state = F_ABSORBED;
      int s = F->soldiers / 2;
      T.soldiers += s;
      for (int d = 0; d < ND; d++) { T.d[d].infl[0] += T.d[d].infl[f]; T.d[d].infl[f] = 0; }
      H.gold += F->cash / 2;
      if (T.nmen < MAXMEN && !F->bossDead) {
        Man *m = &T.men[T.nmen++];
        make_man(m, 6);
        snprintf(m->name, sizeof m->name, "%s", FT[f].boss);
        snprintf(m->nick, sizeof m->nick, "Capo");
        snprintf(m->sprite, sizeof m->sprite, "%s", FT[f].sprite);
        m->capo = 1; m->lvl = 3; m->loyal = 55;
      }
      T.respect = CLAMP(T.respect + 15, 0, 100);
      tlog("%s przyłączają się do Rodziny! +%d żołnierzy.", FT[f].name, s);
      char sc[40];
      snprintf(sc, sizeof sc, "tyc_koniec_%s", FT[f].id);
      if (script_find(sc) >= 0) push_ev(100, f, 0);
      return R_OK;
    }
    case DP_WAR: F->state = F_WAR; rel_add(f, -50); tlog("Wypowiedzieliśmy wojnę: %s.", FT[f].name); return R_OK;
    case DP_SELL: T.booze -= 20; H.gold += 500; rel_add(f, 3); tlog("Sprzedano 20 beczek dla: %s (+$500).", FT[f].name); return R_OK;
  }
  return R_INVALID;
}

/* ================================================================ wydarzenia */
enum { EV_RAISE = 1, EV_COPDEAL, EV_JOURNALIST, EV_ALLYOFFER, EV_TRUCEOFFER, EV_VASSALOFFER, EV_TRIBUTEDEMAND, EV_RAT, EV_WEDDING,
       EV_ELECTION, EV_REPEAL, EV_SHIPMENT, EV_WIDOW, EV_INFORMANT, EV_STORY = 100, EV_BEAT, EV_NEWS };

typedef struct { char title[48]; char text[7][100]; int ntext; char opt[4][60]; int nopt; } EvView;
static void beat_view(const Ev *e, EvView *v);
static void news_view(const Ev *e, EvView *v);
static void beat_apply(const Ev *e);
static void beats_check(void);
static int var_or(const char *n, int def);
static void var_set(const char *n, int v);

static void ev_view(const Ev *e, EvView *v) {
  memset(v, 0, sizeof *v);
  Man *m = (e->a >= 0 && e->a < T.nmen) ? &T.men[e->a] : NULL;
#define TXT(...) snprintf(v->text[v->ntext++], 100, __VA_ARGS__)
#define OPT(...) snprintf(v->opt[v->nopt++], 60, __VA_ARGS__)
  switch (e->id) {
    case EV_RAISE:
      snprintf(v->title, 48, "Prośba o podwyżkę");
      TXT("%s „%s” przychodzi z kapeluszem w dłoni.", m ? m->name : "?", m ? m->nick : "?");
      TXT("„Szefie, ryzykuję głowę za grosze. Należy mi się więcej.”");
      OPT("Daj podwyżkę (+$15 dziennie)"); OPT("Odmów"); OPT("Daj premię jednorazowo ($300)");
      break;
    case EV_COPDEAL:
      snprintf(v->title, 48, "Kapitan policji");
      TXT("Kapitan z komisariatu przy Maxwell Street proponuje układ.");
      TXT("Za $%d jego ludzie przestaną się nami interesować.", e->b);
      OPT("Zapłać ($%d)", e->b); OPT("Odmów");
      break;
    case EV_JOURNALIST:
      snprintf(v->title, 48, "Dziennikarz z Tribune");
      TXT("Reporter węszy wokół naszych interesów.");
      TXT("Jeśli artykuł się ukaże, zrobi się gorąco.");
      OPT("Przekup go ($400)"); OPT("Zastrasz go (ryzyko)"); OPT("Zignoruj");
      break;
    case EV_ALLYOFFER:
      snprintf(v->title, 48, "Propozycja sojuszu");
      TXT("%s przysyła posłańca z butelką wina.", FT[e->a].boss);
      TXT("%s proponują sojusz przeciw wspólnym wrogom.", FT[e->a].name);
      OPT("Przyjmij sojusz"); OPT("Odmów grzecznie");
      break;
    case EV_TRUCEOFFER:
      snprintf(v->title, 48, "Biała flaga");
      TXT("%s mają dość wojny.", FT[e->a].name);
      TXT("%s proponuje rozejm na 14 dni.", FT[e->a].boss);
      OPT("Przyjmij rozejm"); OPT("Walczymy dalej");
      break;
    case EV_VASSALOFFER:
      snprintf(v->title, 48, "Kapitulacja");
      TXT("%s są na kolanach.", FT[e->a].name);
      TXT("Oferują trybut w zamian za życie i spokój.");
      OPT("Przyjmij ich jako lenników"); OPT("Odrzuć — zniszczymy ich");
      break;
    case EV_TRIBUTEDEMAND:
      snprintf(v->title, 48, "Żądanie haraczu");
      TXT("%s żądają od nas $%d „za spokój”.", FT[e->a].name, e->b);
      TXT("Odmowa oznacza otwartą wojnę.");
      OPT("Zapłać ($%d)", e->b); OPT("Odmów");
      break;
    case EV_RAT:
      snprintf(v->title, 48, "Szczur w Rodzinie");
      TXT("Nasz człowiek w policji mówi, że ktoś sypie.");
      TXT("Wszystko wskazuje na: %s „%s”.", m ? m->name : "?", m ? m->nick : "?");
      OPT("Zlikwiduj go"); OPT("Wyślij go z miasta"); OPT("Zignoruj plotki");
      break;
    case EV_WEDDING:
      snprintf(v->title, 48, "Zaproszenie na wesele");
      TXT("Córka bossa %s wychodzi za mąż.", FT[e->a].boss);
      TXT("Przyjęcie zaproszenia z prezentem to wielki gest.");
      OPT("Idź z prezentem ($700)"); OPT("Wyślij życzenia"); OPT("Zignoruj");
      break;
    case EV_ELECTION:
      snprintf(v->title, 48, "Wybory radnych");
      TXT("Radny Kowalczyk startuje w wyborach i potrzebuje funduszy.");
      TXT("Swój człowiek w ratuszu codziennie studzi gorączkę.");
      OPT("Wesprzyj kampanię ($1500)"); OPT("Odmów");
      break;
    case EV_REPEAL:
      snprintf(v->title, 48, "Koniec prohibicji!");
      TXT("5 grudnia 1933: 21. poprawka znosi prohibicję.");
      TXT("Meliny tracą klientów, ale browary mogą działać legalnie.");
      OPT("Zalegalizuj browary ($1000)"); OPT("Działamy po staremu");
      break;
    case EV_SHIPMENT:
      snprintf(v->title, 48, "Transport z Kanady");
      TXT("Przemytnik z Windsor ma 50 beczek whisky na zbyciu.");
      TXT("Cena: $%d za całość.", e->b);
      OPT("Kup ($%d)", e->b); OPT("Nie dziś");
      break;
    case EV_WIDOW:
      snprintf(v->title, 48, "Wdowa po Donie");
      TXT("Donna Maria prosi o datek na nowy dzwon dla kościoła św. Rocha.");
      TXT("Cała dzielnica patrzy, co zrobisz.");
      OPT("Daj hojnie ($500)"); OPT("Odmów");
      break;
    case EV_INFORMANT:
      snprintf(v->title, 48, "Informator");
      TXT("Chłopak z %s zna rozkład ich ludzi.", FT[e->a].name);
      TXT("Za $300 sprzeda nam, gdzie trzymają broń.");
      OPT("Kup informacje ($300)"); OPT("Przegoń go");
      break;
    case EV_BEAT: beat_view(e, v); break;
    case EV_NEWS: news_view(e, v); break;
    case EV_STORY:
      snprintf(v->title, 48, "Wieści z miasta");
      TXT("Ktoś chce się z tobą widzieć w kwaterze.");
      OPT("Jedź na spotkanie");
      break;
  }
#undef TXT
#undef OPT
}

static void ev_apply(const Ev *e, int c) {
  Man *m = (e->a >= 0 && e->a < T.nmen) ? &T.men[e->a] : NULL;
  switch (e->id) {
    case EV_RAISE:
      if (!m) break;
      if (c == 0) { m->wage += 15; m->loyal = CLAMP(m->loyal + 20, 0, 100); tlog("%s dostaje podwyżkę.", m->name); }
      else if (c == 2 && H.gold >= 300) { H.gold -= 300; m->loyal = CLAMP(m->loyal + 12, 0, 100); tlog("%s dostaje premię.", m->name); }
      else { m->loyal = CLAMP(m->loyal - 15, 0, 100); tlog("%s odchodzi naburmuszony.", m->name); }
      break;
    case EV_COPDEAL:
      if (c == 0 && H.gold >= e->b) { H.gold -= e->b; T.heat = CLAMP(T.heat - 30, 0, 100); tlog("Kapitan wziął kopertę. Gorączka spada."); }
      else { T.heat = CLAMP(T.heat + 5, 0, 100); tlog("Odrzuciliśmy ofertę kapitana."); }
      break;
    case EV_JOURNALIST:
      if (c == 0 && H.gold >= 400) { H.gold -= 400; tlog("Reporter zapomniał o artykule."); }
      else if (c == 1) {
        if (rnd(100) < 60) tlog("Reporter zmienił zdanie po rozmowie z naszymi chłopcami.");
        else { T.heat = CLAMP(T.heat + 15, 0, 100); T.invest = CLAMP(T.invest + 5, 0, 100); tlog("Tribune: „Gangsterzy grożą prasie!” Gorączka rośnie."); }
      } else { T.heat = CLAMP(T.heat + 10, 0, 100); tlog("Artykuł w Tribune. Policja się nami interesuje."); }
      break;
    case EV_ALLYOFFER:
      if (c == 0 && fam_alive(e->a)) { T.f[e->a].state = F_ALLY; rel_add(e->a, 10); tlog("Sojusz z: %s!", FT[e->a].name); }
      else { rel_add(e->a, -8); }
      break;
    case EV_TRUCEOFFER:
      if (c == 0 && fam_alive(e->a)) { T.f[e->a].state = F_TRUCE; T.f[e->a].stateT = 14; tlog("Rozejm z: %s.", FT[e->a].name); }
      break;
    case EV_VASSALOFFER:
      if (c == 0 && fam_alive(e->a)) { T.f[e->a].state = F_VASSAL; T.f[e->a].vassalDays = 0; T.respect = CLAMP(T.respect + 6, 0, 100); tlog("%s zostają naszymi lennikami.", FT[e->a].name); }
      break;
    case EV_TRIBUTEDEMAND:
      if (c == 0 && H.gold >= e->b) { H.gold -= e->b; rel_add(e->a, 15); T.respect = CLAMP(T.respect - 5, 0, 100); tlog("Zapłaciliśmy haracz: %s.", FT[e->a].name); }
      else { rel_add(e->a, -20); go_war(e->a); }
      break;
    case EV_RAT:
      if (!m) break;
      if (c == 0) { tlog("%s „%s” zniknął. Nikt nie pyta.", m->name, m->nick); T.respect = CLAMP(T.respect + 4, 0, 100); remove_man(e->a); }
      else if (c == 1) { tlog("%s „%s” wyjechał do Kalifornii.", m->name, m->nick); remove_man(e->a); }
      else { T.invest = CLAMP(T.invest + 12, 0, 100); T.heat = CLAMP(T.heat + 10, 0, 100); tlog("Szczur dalej sypie. Śledztwo przyspiesza."); }
      break;
    case EV_WEDDING:
      if (c == 0 && H.gold >= 700) { H.gold -= 700; rel_add(e->a, 25); T.respect = CLAMP(T.respect + 3, 0, 100); tlog("Wesele u %s. Stosunki dużo lepsze.", FT[e->a].boss); }
      else if (c == 1) rel_add(e->a, 3);
      else rel_add(e->a, -10);
      break;
    case EV_ELECTION:
      if (c == 0 && H.gold >= 1500) { H.gold -= 1500; T.politician = 1; tlog("Radny Kowalczyk wygrał. Mamy swojego człowieka w ratuszu."); }
      break;
    case EV_REPEAL:
      if (c == 0 && H.gold >= 1000) { H.gold -= 1000; T.legalBrew = 1; tlog("Nasze browary działają teraz legalnie!"); }
      break;
    case EV_SHIPMENT:
      if (c == 0 && H.gold >= e->b) { H.gold -= e->b; T.booze += 50; tlog("Kupiono 50 beczek kanadyjskiej whisky."); }
      break;
    case EV_WIDOW:
      if (c == 0 && H.gold >= 500) { H.gold -= 500; T.respect = CLAMP(T.respect + 6, 0, 100); for (int i = 0; i < T.nmen; i++) T.men[i].loyal = CLAMP(T.men[i].loyal + 4, 0, 100); tlog("Dzwon dla św. Rocha. Dzielnica nas kocha."); }
      else { T.respect = CLAMP(T.respect - 4, 0, 100); }
      break;
    case EV_INFORMANT:
      if (c == 0 && H.gold >= 300 && fam_alive(e->a)) { H.gold -= 300; int s = rr(3, 6); T.f[e->a].soldiers -= s; tlog("Dzięki informatorowi rozbiliśmy skład broni: %s -%d.", FT[e->a].name, s); }
      break;
    case EV_BEAT: beat_apply(e); break;
    case EV_NEWS: break;
    case EV_STORY: {
      char sc[40];
      snprintf(sc, sizeof sc, "tyc_koniec_%s", FT[e->a].id);
      int s = script_find(sc);
      if (s >= 0 && S.hqMap[0] && !g_autoplay) {
        world_load_map(map_find(S.hqMap), S.hqX, S.hqY, S.hqDir);
        game_set_mode(MODE_WORLD);
        script_start(s, NULL);
      }
      break;
    }
  }
}

/* ================================================================ AI rywali */
static int pick_player_dist(int f) {
  int best = -1, bv = -1;
  for (int d = 0; d < ND; d++) {
    if (T.d[d].infl[0] <= 0) continue;
    int v = T.d[d].infl[0] + DT[d].wealth * 8 + T.d[d].infl[f] * 2 - dist_defense(d) / 3 + rnd(20);
    if (v > bv) { bv = v; best = d; }
  }
  return best;
}

static void rival_attack_player(int f) {
  int d = pick_player_dist(f);
  if (d < 0) return;
  int A = T.f[f].soldiers * rr(40, 90) / 100 * (T.f[f].state == F_WAR ? 13 : 10) / 10 + 5;
  int D = dist_defense(d);
  if (rnd(A + D) < A) {
    int g = infl_take(d, f, rr(8, 18), 0);
    tlog("%s napadli na %s! Tracimy %d wpływów.", FT[f].name, DT[d].name, g);
    /* biznes w dzielnicy */
    int cand[MAXBIZ], nc = 0;
    for (int i = 0; i < T.nbiz; i++) if (T.biz[i].dist == d) cand[nc++] = i;
    if (nc && rnd(100) < 55) {
      int bi = cand[rnd(nc)];
      if (rnd(100) < 25 && T.biz[bi].lvl == 1) { tlog("Spalili nasz lokal: %s.", BT[T.biz[bi].type].name); biz_remove(bi); }
      else { T.biz[bi].closed = rr(2, 4); tlog("%s zdemolowany, zamknięty na %d dni.", BT[T.biz[bi].type].name, T.biz[bi].closed); }
    }
    int sl = rr(1, 3);
    if (T.soldiers >= sl) { T.soldiers -= sl; tlog("Zginęło %d naszych żołnierzy.", sl); }
    for (int i = 0; i < T.nmen; i++)
      if (T.men[i].assign == 1000 + d && T.men[i].status == MS_OK && rnd(100) < 35) { T.men[i].status = MS_HURT; T.men[i].statusT = rr(2, 4); tlog("%s ranny w obronie.", T.men[i].name); }
  } else {
    int l = rr(2, 5);
    T.f[f].soldiers -= l;
    T.respect = CLAMP(T.respect + 1, 0, 100);
    tlog("Odparliśmy atak: %s w dzielnicy %s (-%d ich ludzi).", FT[f].name, DT[d].name, l);
  }
  rel_add(f, -5);
}

static void rival_vs_rival(int f) {
  int g = -1;
  for (int k = 0; k < 6; k++) { int c = 1 + rnd(NF - 1); if (c != f && fam_alive(c)) { g = c; break; } }
  if (g < 0) return;
  /* sojusznik gracza nie jest atakowany przez innego sojusznika */
  if ((T.f[f].state == F_ALLY || T.f[f].state == F_VASSAL) && (T.f[g].state == F_ALLY || T.f[g].state == F_VASSAL)) return;
  int d = -1, bv = 0;
  for (int k = 0; k < ND; k++) if (T.d[k].infl[g] > bv) { bv = T.d[k].infl[g]; d = k; }
  if (d < 0) return;
  int A = T.f[f].soldiers + rnd(20), D = T.f[g].soldiers + rnd(20);
  if (A > D) { int x = infl_take(d, f, rr(5, 12), g); T.f[g].soldiers -= rr(1, 4); if (x > 0 && rnd(2)) tlog("Strzelanina: %s odbierają %s część dzielnicy %s.", FT[f].name, FT[g].name, DT[d].name); }
  else T.f[f].soldiers -= rr(1, 4);
}

static void rivals_day(void) {
  for (int f = 1; f < NF; f++) {
    Fam *F = &T.f[f];
    if (!fam_alive(f)) continue;
    int inc = fam_income(f);
    F->cash += inc - F->soldiers * 5;
    if (F->state == F_VASSAL) {
      int tr = inc * 15 / 100; H.gold += tr; F->vassalDays++; T.lastIncome += tr;
      if (rnd(2)) rel_add(f, 1);
      if (fam_power(f) > player_power() && rnd(100) < 15) { F->state = F_WAR; rel_add(f, -40); tlog("%s zrywają się ze smyczy i przestają płacić!", FT[f].name); }
    }
    int cap = 22 + inc / 10;
    if (F->cash > 150 && F->soldiers < cap) { int r = CLAMP(F->cash / 300, 1, 3); if (F->soldiers + r > cap) r = cap - F->soldiers; F->soldiers += r; F->cash -= r * 100; }
    if (F->cash < -500) { F->soldiers -= 2; F->cash += 200; }
    if (F->soldiers <= 3) { family_collapse(f, 0); continue; }
    if (F->leaderless > 0) {
      if (--F->leaderless == 0) { F->bossDead = 0; tlog("%s mają nowego szefa.", FT[f].name); }
      continue;
    }
    /* wzrost wpływów */
    for (int d = 0; d < ND; d++) if (T.d[d].owner == f && T.d[d].infl[f] < 85 && infl_free(d) > 0 && rnd(3) == 0) T.d[d].infl[f]++;
    /* agresja wobec gracza */
    int p = 0;
    if (F->state == F_WAR) p = F->aggr * 45 / 100;
    else if (F->state == F_NEUTRAL) p = F->rel < -20 ? F->aggr / 4 : F->aggr / 14;
    if (T.day < 3) p = 0;
    if (rnd(100) < p) rival_attack_player(f);
    if (rnd(100) < 6) rival_vs_rival(f);
    /* sojusznicy pomagają */
    if (F->state == F_ALLY && rnd(100) < 8) {
      for (int g = 1; g < NF; g++)
        if (g != f && T.f[g].state == F_WAR && fam_alive(g)) { int l = rr(1, 3); T.f[g].soldiers -= l; tlog("%s uderzają w naszych wrogów: %s -%d.", FT[f].name, FT[g].name, l); break; }
    }
    /* Kane: politycy i policja */
    if (f == 4 && F->rel < -20 && F->state != F_TRUCE) T.heat = CLAMP(T.heat + 2, 0, 100);
    /* zmiana stosunków */
    if (F->state == F_NEUTRAL && rnd(3) == 0) F->rel += F->rel < 0 ? 1 : F->rel > 20 ? -1 : 0;
    if (F->state == F_TRUCE && --F->stateT <= 0) { F->state = F_NEUTRAL; tlog("Rozejm z: %s wygasł.", FT[f].name); }
    if (F->state == F_NEUTRAL && F->rel <= -60) go_war(f);
    if (F->state == F_ALLY && F->rel < 10) { F->state = F_NEUTRAL; tlog("%s zrywają sojusz.", FT[f].name); }
    /* propozycje */
    if (F->state == F_NEUTRAL && F->rel >= 45 && rnd(100) < 4) push_ev(EV_ALLYOFFER, f, 0);
    if (F->state == F_WAR && F->soldiers * 2 < player_power() && rnd(100) < 10) push_ev(F->soldiers * 3 < player_power() ? EV_VASSALOFFER : EV_TRUCEOFFER, f, 0);
    if (F->state == F_NEUTRAL && fam_power(f) > player_power() * 2 && F->rel < 0 && rnd(100) < 4) push_ev(EV_TRIBUTEDEMAND, f, 400 + T.day * 10);
    if (F->state != F_WAR && rnd(1000) < 6) push_ev(EV_WEDDING, f, 0);
    if (F->state == F_WAR && rnd(100) < 4) push_ev(EV_INFORMANT, f, 0);
  }
}

/* ================================================================ koniec dnia */
static void check_end(void);

void tycoon_end_day(void) {
  if (!T.active || T.ended) return;
  T.ntoday = 0;
  char ds[40];
  date_str(T.day, ds, sizeof ds);
  snprintf(tlogHeader, sizeof tlogHeader, "{y}— %s —{-}", ds);
  int incomeBefore = H.gold;
  T.lastIncome = T.lastCosts = 0;

  /* akcje w toku */
  for (int o = 0; o < MAXOPS; o++) {
    Op *op = &T.ops[o];
    if (!op->active) continue;
    if (--op->days > 0) continue;
    if (!op_target_valid(op->type, op->target)) { op_resolve(op, 0, 0); continue; }
    int ch = op_chance(op->type, op->target, op->team, op->nteam, op->soldiers);
    op_resolve(op, rnd(100) < ch, 0);
  }

  /* alkohol */
  int make = 0, use = 0;
  for (int i = 0; i < T.nbiz; i++) {
    if (T.biz[i].closed) continue;
    make += (int)(BT[T.biz[i].type].boozeMake * LVLMULT[T.biz[i].lvl]);
    use += BT[T.biz[i].type].boozeUse * T.biz[i].lvl;
  }
  if (T.repealed) use /= 2;
  T.booze += make;
  int boozeOk = T.booze >= use;
  T.booze = T.booze >= use ? T.booze - use : 0;
  T.lastBooze = make - use;
  if (!boozeOk && use) tlog("Brakuje alkoholu! Meliny i kluby zarabiają mniej.");
  if (T.booze > 150) { int sold = T.booze - 150; int price = T.repealed ? 5 : 10; H.gold += sold * price; T.booze = 150; tlog("Nadwyżka %d beczek sprzedana hurtowo (+$%d).", sold, sold * price); }

  /* biznesy */
  int heatAdd = 0;
  for (int i = 0; i < T.nbiz; i++) {
    int inc = biz_income(i, boozeOk);
    T.biz[i].earned += inc;
    H.gold += inc;
    heatAdd += biz_heat(i);
    if (T.biz[i].type == B_PRALNIA && !T.biz[i].closed) T.invest = CLAMP(T.invest - 1, 0, 100);
    if (T.biz[i].type == B_RESTAUR && !T.biz[i].closed && rnd(4) == 0) T.respect = CLAMP(T.respect + 1, 0, 100);
    if (T.biz[i].closed > 0) T.biz[i].closed--;
  }
  /* pensje */
  int wages = T.soldiers * 6;
  for (int i = 0; i < T.nmen; i++) if (T.men[i].status != MS_JAIL) wages += T.men[i].wage;
  H.gold -= wages;
  T.lastCosts = wages;
  T.lastIncome += H.gold + wages - incomeBefore;

  /* gorączka i śledztwo */
  T.heat = CLAMP(T.heat + heatAdd - 4 - (T.politician ? 2 : 0), 0, 100);
  if (T.heat > 60) T.invest += (T.heat - 60) / 10 + 1;
  else if (rnd(2)) T.invest -= 1;
  T.invest = CLAMP(T.invest, 0, 100);
  if (T.invest >= 50 && T.warnInvest < 1) { T.warnInvest = 1; tlog("UWAGA: agenci federalni przeglądają nasze księgi!"); }
  if (T.invest >= 80 && T.warnInvest < 2) { T.warnInvest = 2; tlog("UWAGA: prokurator zbiera dowody. Zbijcie gorączkę!"); }
  if (T.invest < 40) T.warnInvest = 0;
  /* nalot */
  if (T.heat > 40 && rnd(100) < (T.heat - 40) / 2) {
    if (T.nbiz && rnd(2)) {
      int bi = rnd(T.nbiz);
      T.biz[bi].closed = rr(2, 4);
      int fine = rr(100, 400);
      H.gold -= fine;
      tlog("Nalot policji: %s (%s) zamknięty, grzywna $%d.", BT[T.biz[bi].type].name, DT[T.biz[bi].dist].name, fine);
    } else if (T.nmen) {
      int mi = rnd(T.nmen);
      if (T.men[mi].status == MS_OK) { unassign(mi); T.men[mi].status = MS_JAIL; T.men[mi].statusT = rr(3, 7); tlog("Policja aresztowała: %s „%s”.", T.men[mi].name, T.men[mi].nick); }
    }
  }

  /* ludzie */
  for (int i = T.nmen - 1; i >= 0; i--) {
    Man *m = &T.men[i];
    if ((m->status == MS_HURT || m->status == MS_JAIL) && --m->statusT <= 0) { if (m->status == MS_JAIL) tlog("%s wyszedł z aresztu.", m->name); m->status = MS_OK; }
    if (H.gold < 0) m->loyal -= 4;
    else if (m->loyal < 60 && rnd(3) == 0) m->loyal++;
    m->loyal = CLAMP(m->loyal, 0, 100);
    if (m->status == MS_OK && m->loyal < 25 && rnd(100) < 5) {
      if (rnd(2)) push_ev(EV_RAT, i, 0);
      else {
        int f = 1 + rnd(NF - 1);
        tlog("%s „%s” zdezerterował%s.", m->name, m->nick, fam_alive(f) ? " do rywali" : "");
        if (fam_alive(f)) T.f[f].soldiers += 2;
        unassign(i);
        remove_man(i);
      }
    }
  }
  if (T.nmen && rnd(100) < 4) { int mi = rnd(T.nmen); if (T.men[mi].loyal < 70) push_ev(EV_RAISE, mi, 0); }

  rivals_day();
  recompute_owners(1);
  T.lastWar = 0;
  for (int f = 1; f < NF; f++) if (fam_alive(f) && T.f[f].state == F_WAR) T.lastWar = 1;

  /* wydarzenia losowe */
  if (rnd(100) < 5 && T.heat > 30) push_ev(EV_COPDEAL, 0, 800 + T.heat * 15);
  if (rnd(100) < 4 && T.heat > 20) push_ev(EV_JOURNALIST, 0, 0);
  if (!T.politician && rnd(1000) < 15) push_ev(EV_ELECTION, 0, 0);
  if (rnd(100) < 4 && !T.repealed) push_ev(EV_SHIPMENT, 0, rr(500, 800));
  if (rnd(1000) < 12) push_ev(EV_WIDOW, 0, 0);
  if (T.day + 1 == REPEAL_DAY) { T.repealed = 1; tlog("KONIEC PROHIBICJI!"); push_ev(EV_REPEAL, 0, 0); }

  if (H.gold < 0) {
    T.debtDays++;
    tlog("Kasa jest pusta! Ludzie tracą cierpliwość (%d/10).", T.debtDays);
  } else T.debtDays = 0;

  T.day++;
  if (T.day % 7 == 0) refresh_recruits();
  if (T.day % 7 == 3 && T.day / 7 != T.newsWeek) { T.newsWeek = T.day / 7; push_ev(EV_NEWS, 0, T.day / 7); }
  beats_check();
  /* trwałe skutki decyzji z kroniki */
  if (var_or("LUCIA_KSIEGI", 0) && T.day % 2 == 0) T.invest = CLAMP(T.invest - 1, 0, 100);
  if (var_or("BRONEK_CAPO", 0) && T.day % 4 == 0) infl_take(1, 0, 1, -1);
  if (var_or("EXPO_DNI", 0) > 0) { H.gold += 150; T.lastIncome += 150; var_set("EXPO_DNI", var_or("EXPO_DNI", 0) - 1); if (!var_or("EXPO_DNI", 0)) tlog("Kontrakt na wystawie wygasł."); }
  if (var_or("OPIUM", 0)) { H.gold += 220; T.lastIncome += 220; T.heat = CLAMP(T.heat + 1, 0, 100); }
  if (var_or("KANE_RADNY", 0) && T.day % 3 == 0) T.heat = CLAMP(T.heat - 1, 0, 100);
  T.respect = CLAMP(T.respect, 0, 100);
  check_end();
}

/* ================================================================ koniec gry */
static int win_state(int *conquest) {
  int done = 0, force = 0;
  for (int f = 1; f < NF; f++) {
    int s = T.f[f].state;
    if (s == F_ABSORBED || s == F_DEAD) { done++; force++; }
    else if (s == F_VASSAL || s == F_ALLY) done++;
  }
  if (conquest) *conquest = force;
  return done == NF - 1 && dist_count(0) >= 5;
}

static void check_end(void) {
  int conquest;
  if (T.invest >= 100) {
    T.ended = 1;
    ui_ending_start("WYROK", "Federalny sąd w Chicago skazał cię na 11 lat więzienia za uchylanie się od płacenia podatków.\n\n"
                             "Nie złapali cię za żadne morderstwo, żaden przemyt, żadną łapówkę. Złapała cię księgowość.\n\n"
                             "Rodzina rozpadła się, zanim pociąg do Atlanty minął granicę stanu.\n\nKONIEC");
    return;
  }
  if (T.debtDays >= 10) {
    T.ended = 1;
    ui_ending_start("ROZPAD", "Bez pieniędzy nie ma lojalności. Twoi ludzie odeszli jeden po drugim, a rywale rozszarpali to, co zostało.\n\n"
                              "Spędzasz wieczory w pustej trattorii, słuchając, jak w radiu grają jazz z klubów, które kiedyś były twoje.\n\nKONIEC");
    return;
  }
  if (win_state(&conquest)) {
    T.ended = 1;
    if (script_find("tyc_final") >= 0 && S.hqMap[0] && !g_autoplay) {
      H.vars[var_find("_PODBOJ", 1)] = conquest;
      world_load_map(map_find(S.hqMap), S.hqX, S.hqY, S.hqDir);
      game_set_mode(MODE_WORLD);
      script_start(script_find("tyc_final"), NULL);
      return;
    }
    if (conquest >= 3)
      ui_ending_start("CAPO DI TUTTI CAPI", "Chicago należy do ciebie. Każda melina, każdy bukmacher, każdy gliniarz.\n\n"
                                            "Twoje imię wymawia się szeptem. Ale nocami nie śpisz — wiesz, jak skończył Don.\n\nKONIEC");
    else
      ui_ending_start("KOMISJA", "Zamiast wojny — stół. Bossowie wszystkich rodzin siadają do rozmów, a ty przewodniczysz.\n\n"
                                 "Miasto zarabia, ulice są spokojne, a Don byłby z ciebie dumny.\n\nKONIEC");
  }
}

/* ================================================================ start */
void tycoon_init(void) {
  memset(&T, 0, sizeof T);
  memset(&MS, 0, sizeof MS);
  for (int i = 0; i < MAXBIZ; i++) T.biz[i].manager = -1;
}

int tycoon_active(void) { return T.active; }

static int var_or(const char *n, int def) { int v = var_find(n, 0); return v >= 0 ? H.vars[v] : def; }

void tycoon_start(int trust) {
  tycoon_init();
  T.active = 1;
  for (int d = 0; d < ND; d++) {
    for (int f = 0; f < NF; f++) T.d[d].infl[f] = DT[d].infl[f];
    T.d[d].owner = -1;
  }
  for (int f = 1; f < NF; f++) {
    T.f[f].soldiers = FT[f].soldiers;
    T.f[f].cash = FT[f].cash;
    T.f[f].aggr = FT[f].aggr;
    T.f[f].state = F_NEUTRAL;
  }
  /* decyzje z prologu */
  T.f[1].rel = var_or("REL_IRL", 0);
  T.f[2].rel = var_or("REL_RUS", -30);
  T.f[3].rel = var_or("REL_TRI", 0);
  T.f[4].rel = var_or("REL_KAN", 0);
  if (var_or("POLONIA", 0) > 0) T.d[1].infl[0] += 15;
  T.heat = CLAMP(var_or("GORACZKA", 15), 0, 100);
  if (var_or("WALSH", 0)) T.politician = 1;
  for (int f = 1; f < NF; f++) if (T.f[f].rel <= -60) T.f[f].state = F_WAR;

  trust = CLAMP(trust, 0, 100);
  T.respect = 10 + trust / 4;
  T.soldiers = 6 + trust / 8;
  T.booze = 30;
  H.gold += 1500 + trust * 15;
  int nmen = 3 + trust / 35;
  for (int i = 0; i < nmen; i++) { make_man(&T.men[T.nmen], 2); T.men[T.nmen].loyal = 50 + trust / 3; T.nmen++; }
  /* znani z prologu */
  if (var_or("VITO_ZYJE", 1) && T.nmen < MAXMEN) {
    Man *m = &T.men[T.nmen++];
    make_man(m, 4);
    snprintf(m->name, sizeof m->name, "Vito Marino");
    snprintf(m->nick, sizeof m->nick, "Kuzyn");
    snprintf(m->sprite, sizeof m->sprite, "vito");
    m->fight = 6; m->brains = 3; m->loyal = 80; m->lvl = 2;
  }
  if (var_or("BRONEK", 0) && T.nmen < MAXMEN) {
    Man *m = &T.men[T.nmen++];
    make_man(m, 4);
    snprintf(m->name, sizeof m->name, "Bronek Mazur");
    snprintf(m->nick, sizeof m->nick, "Kowal");
    snprintf(m->sprite, sizeof m->sprite, "bronek");
    m->fight = 5; m->brains = 5; m->loyal = 85; m->lvl = 2;
  }
  T.biz[T.nbiz++] = (Biz){B_RESTAUR, 0, 1, -1, 0, 0};
  T.biz[T.nbiz++] = (Biz){B_MELINA, 0, 1, -1, 0, 0};
  if (trust >= 50) T.biz[T.nbiz++] = (Biz){B_BUKMACHER, 0, 1, -1, 0, 0};
  if (T.d[1].infl[0] >= 25) T.biz[T.nbiz++] = (Biz){B_BIMBER, 1, 1, -1, 0, 0};
  refresh_recruits();
  recompute_owners(0);
  tlog("Rodzina jest teraz twoja. Odbuduj ją.");
}

/* ================================================================ zapis */
void tycoon_save(FILE *f) {
  if (!T.active) return;
  const unsigned char *p = (const unsigned char *)&T;
  size_t n = sizeof T;
  fprintf(f, "tysize %u\n", (unsigned)n);
  for (size_t off = 0; off < n; off += 64) {
    fprintf(f, "tyx %u ", (unsigned)off);
    for (size_t k = off; k < off + 64 && k < n; k++) fprintf(f, "%02x", p[k]);
    fprintf(f, "\n");
  }
}

int tycoon_load_line(const char *line) {
  static int sizeOk;
  if (!strncmp(line, "tysize ", 7)) { size_t n = (size_t)atol(line + 7); sizeOk = n > 0 && n <= sizeof T; if (sizeOk) memset(&T, 0, sizeof T); return 1; }
  if (!strncmp(line, "tyx ", 4)) {
    if (!sizeOk) return 1;
    unsigned off = (unsigned)atol(line + 4);
    const char *h = strchr(line + 4, ' ');
    if (!h) return 1;
    h++;
    unsigned char *p = (unsigned char *)&T;
    for (size_t k = off; k < sizeof T && h[0] && h[1] && h[0] != '\n'; k++, h += 2) {
      unsigned v;
      if (sscanf(h, "%2x", &v) != 1) break;
      p[k] = (unsigned char)v;
    }
    return 1;
  }
  return 0;
}

/* ================================================================ akcje osobiste (3D) */

int tycoon_mission_active(void) { return MS.active; }

static int op_map(int type) { return OT[type].map ? map_find(OT[type].map) : -1; }

static const char *enemy_for(int f, int boss) {
  static char b[24];
  if (f <= 0) return boss ? "zbir" : (rnd(2) ? "zbir" : "zbir2");
  snprintf(b, sizeof b, "%s_%s", FT[f].id, boss ? "boss" : "zbir");
  if (enemy_find(b) < 0) return "zbir";
  return b;
}

/* wejście do miejsca: region dużej mapy albo osobna mapa */
static void map_entry(int map, int *x, int *y, int *dir);
static int place_find(const char *name, int *x, int *y, int *dir) {
  int r = region_find(name);
  if (r >= 0) { *x = S.regions[r].ex; *y = S.regions[r].ey; *dir = S.regions[r].edir; return S.regions[r].map; }
  int mp = map_find(name);
  if (mp >= 0) map_entry(mp, x, y, dir);
  return mp;
}
static int district_place(int d, int *x, int *y, int *dir) {
  for (int i = 0; i < S.nregions; i++)
    if (S.regions[i].district == d + 1) { *x = S.regions[i].ex; *y = S.regions[i].ey; *dir = S.regions[i].edir; return S.regions[i].map; }
  for (int i = 0; i < S.nmaps; i++) if (S.maps[i].district == d + 1) { map_entry(i, x, y, dir); return i; }
  return -1;
}

static void map_entry(int map, int *x, int *y, int *dir) {
  MapDef *m = &S.maps[map];
  *dir = m->edir;
  if (m->ex >= 0) { *x = m->ex; *y = m->ey; return; }
  for (int yy = 1; yy < m->h - 1; yy++)
    for (int xx = 1; xx < m->w - 1; xx++)
      if (!tile_solid(m->tiles[yy][xx])) { *x = xx; *y = yy; return; }
  *x = *y = 1;
}

static void mission_finish(int success) {
  if (!MS.active) return;
  MS.active = 0;
  Op *op = &T.ops[MS.op];
  int hurt[8] = {0};
  for (int k = 0; k < MS.n; k++) if (MS.man[k] >= 0 && MS.ent[k] < W.n && W.ents[MS.ent[k]].dead) hurt[k] = 1;
  op_resolve(op, success, 1);
  for (int k = 0; k < MS.n; k++)
    if (hurt[k] && MS.man[k] < T.nmen) { T.men[MS.man[k]].status = MS_HURT; T.men[MS.man[k]].statusT = rr(2, 4); }
  if (H.hp < H.mhp / 2) H.hp = H.mhp / 2;
  H.objective[0] = 0;
  recompute_owners(1);
  int hq = S.hqMap[0] ? map_find(S.hqMap) : -1;
  if (hq >= 0) world_load_map(hq, S.hqX, S.hqY, S.hqDir);
  tycoon_open();
}

void tycoon_mission_fail(void) {
  ui_toast("Ledwo uszedłeś z życiem...");
  mission_finish(0);
}

void tycoon_world_tick(void) {
  if (!MS.active || W.trans) return;
  if (!MS.endT && world_enemies_alive() == 0) {
    MS.endT = 150;
    ui_toast("Akcja zakończona! Wracamy do kwatery.");
    snprintf(H.objective, sizeof H.objective, "Akcja zakończona sukcesem");
    sfx_play(SFX_LEVEL);
  }
  if (MS.endT > 0 && --MS.endT == 0) mission_finish(1);
}

static int mission_start(int type, int target, const int *team, int n, int soldiers) {
  int map = op_map(type);
  if (map < 0) return R_INVALID;
  int r = t_launch(type, target, team, n, soldiers);
  if (r) return r;
  int o;
  for (o = MAXOPS - 1; o >= 0; o--) if (T.ops[o].active && T.ops[o].type == type && T.ops[o].target == target) break;
  memset(&MS, 0, sizeof MS);
  MS.active = 1; MS.op = o;
  int ex, ey, ed;
  map_entry(map, &ex, &ey, &ed);
  world_load_map(map, ex, ey, ed);
  game_set_mode(MODE_WORLD);
  g_fade = 255; g_autofade = 1;
  H.hp = H.mhp;
  if (!H.hasWpn[WPN_PISTOL]) { H.hasWpn[WPN_PISTOL] = 1; H.ammo[WPN_PISTOL] += 21; combat_top_up(WPN_PISTOL); if (H.weapon == WPN_FISTS) H.weapon = WPN_PISTOL; }
  /* przeciwnicy */
  int fam = OT[type].tgt == TG_FAM ? target : OT[type].tgt == TG_DIST ? strongest_rival_in(target) : -1;
  int cnt = CLAMP(op_diff(type, target) / 16, 3, 9);
  MapDef *m = &S.maps[map];
  int cx[512], cz[512], nc = 0;
  for (int y = 1; y < m->h - 1 && nc < 512; y++)
    for (int x = 1; x < m->w - 1 && nc < 512; x++) {
      if (tile_solid(m->tiles[y][x])) continue;
      int dx = x - ex, dy = y - ey;
      if (dx * dx + dy * dy < 49) continue;
      int occupied = 0;
      for (int e = 0; e < m->nents; e++) if (m->ents[e].x == x && m->ents[e].y == y) occupied = 1;
      if (!occupied) { cx[nc] = x; cz[nc] = y; nc++; }
    }
  if (type == OP_ZAMACH && nc) { /* boss najdalej */
    int bi = 0, bd = -1;
    for (int i = 0; i < nc; i++) { int d = (cx[i] - ex) * (cx[i] - ex) + (cz[i] - ey) * (cz[i] - ey); if (d > bd) { bd = d; bi = i; } }
    world_spawn(enemy_for(fam, 1), cx[bi], cz[bi], 0);
    W.ents[W.n - 1].state = 0;
    cx[bi] = cx[--nc]; cz[bi] = cz[nc];
  }
  for (int k = 0; k < cnt && nc > 0; k++) {
    int i = rnd(nc);
    const char *id = OT[type].tgt == TG_HEIST ? (enemy_find("straznik") >= 0 ? "straznik" : "zbir") : enemy_for(fam, 0);
    world_spawn(id, cx[i], cz[i], 0);
    if (rnd(3)) W.ents[W.n - 1].state = 0;
    /* usuń sąsiednie miejsca, żeby się nie tłoczyli */
    int px = cx[i], pz = cz[i];
    for (int j = nc - 1; j >= 0; j--) if (abs(cx[j] - px) <= 1 && abs(cz[j] - pz) <= 1) { cx[j] = cx[nc - 1]; cz[j] = cz[nc - 1]; nc--; }
  }
  /* nasi ludzie */
  const char *ally = enemy_find("nasz") >= 0 ? "nasz" : "zbir";
  int extra = soldiers / 3 > 3 ? 3 : soldiers / 3;
  /* sojusznicy za plecami i po bokach gracza, nigdy przed nim */
  int fx = ed == DIR_RIGHT ? 1 : ed == DIR_LEFT ? -1 : 0, fz = ed == DIR_DOWN ? 1 : ed == DIR_UP ? -1 : 0;
  int sx = -fz, sz = fx;
  int offs[10][2] = {{-fx * 2, -fz * 2}, {-fx + sx, -fz + sz}, {-fx - sx, -fz - sz}, {sx * 2, sz * 2}, {-sx * 2, -sz * 2},
                     {-fx * 2 + sx, -fz * 2 + sz}, {-fx * 2 - sx, -fz * 2 - sz}, {sx, sz}, {-sx, -sz}, {-fx, -fz}};
  for (int k = 0, slot = 0; k < n + extra && MS.n < 8; k++) {
    int ax = ex, az = ey;
    for (; slot < 10; slot++) {
      int tx = ex + offs[slot][0], tz = ey + offs[slot][1];
      if (tx > 0 && tz > 0 && tx < m->w && tz < m->h && !tile_solid(m->tiles[tz][tx])) { ax = tx; az = tz; slot++; break; }
    }
    world_spawn(ally, ax, az, 1);
    MS.ent[MS.n] = W.n - 1;
    MS.man[MS.n] = k < n ? team[k] : -1;
    MS.n++;
  }
  char tn[48] = "";
  if (OT[type].tgt == TG_DIST) snprintf(tn, sizeof tn, ": %s", DT[target].name);
  else if (OT[type].tgt == TG_FAM) snprintf(tn, sizeof tn, ": %s", FT[target].name);
  else if (OT[type].tgt == TG_HEIST) snprintf(tn, sizeof tn, ": %s", HEIST[target]);
  snprintf(H.objective, sizeof H.objective, "%s%s — wyeliminuj wrogów", OT[type].name, tn);
  music_play("akcja");
  return R_OK;
}

/* test: akcja osobista z pierwszym wolnym człowiekiem */
int tycoon_debug_mission(int type, int target) {
  int team[1] = {0};
  if (!T.active || !T.nmen) return -1;
  T.men[0].status = MS_OK; T.men[0].assign = -1;
  return mission_start(type, target, team, 1, 3);
}

/* ================================================================ UI: warstwa zgodności (układ 480x270 -> wektorowe UI 1280x720) */
#define KS (1280.0f / 480.0f)
#define TFS 19.0f /* rozmiar tekstu */
#define TSCREEN_W 480
#define TSCREEN_H 270
static u32 pal(char c) {
  switch (c) {
    case 'k': return 0xFF1A1C2C; case 'p': return 0xFF5D275D; case 'r': return 0xFFE0605A; case 'o': return 0xFFEF8D57;
    case 'y': return 0xFFF2C46B; case 'l': return 0xFF9AE08A; case 'g': return 0xFF5AC07A; case 't': return 0xFF2E8A94;
    case 'n': return 0xFF29366F; case 'b': return 0xFF3B5DC9; case 'c': return 0xFF6AB6F6; case 'a': return 0xFF8AEFF7;
    case 'w': return 0xFFF0E6D0; case 's': return 0xFFA9B4C2; case 'd': return 0xFF6A7488; case 'e': return 0xFF333C57;
    case 'f': return 0xFFF0C8A0; case 'u': return 0xFF8B5A3C; case 'v': return 0xFF5A3A28; case 'x': return 0xFF1F4D33;
    case 'm': return 0xFFC0CAD6; case 'z': return 0xFFE8B84A; case 'j': return 0xFF7A2A3A; case 'q': return 0xFF3A2A4A;
    case 'i': return 0xFF141016; default: return 0xFFFFFFFF;
  }
}
static u32 blend(u32 a, u32 b, int t) {
  int r = (((a >> 16) & 255) * (256 - t) + ((b >> 16) & 255) * t) >> 8;
  int g = (((a >> 8) & 255) * (256 - t) + ((b >> 8) & 255) * t) >> 8;
  int bb = ((a & 255) * (256 - t) + (b & 255) * t) >> 8;
  return 0xFF000000u | (r << 16) | (g << 8) | bb;
}
static void gfx_rect(int x, int y, int w, int h, u32 c) { d2_rect(x * KS, y * KS, w * KS, h * KS, c); }
static void gfx_rect_a(int x, int y, int w, int h, u32 c, int a) { d2_rect(x * KS, y * KS, w * KS, h * KS, WITH_A(c, a < 0 ? 0 : a > 255 ? 255 : a)); }
static void gfx_vgrad(int x, int y, int w, int h, u32 t, u32 b) { d2_grad(x * KS, y * KS, w * KS, h * KS, t, b); }
static void gfx_window(int x, int y, int w, int h) { ui_panel(x * KS, y * KS, w * KS, h * KS); }
static int text_width(const char *s) { return (int)(d2_text_w(FONT_SANS, TFS, s) / KS + 0.5f); }
static int text_draw(int x, int y, const char *s, u32 c) { return (int)(d2_text(FONT_SANS, TFS, x * KS, y * KS - 3, s, c) / KS); }
static int text_draw_sh(int x, int y, const char *s, u32 c) { return (int)(d2_text_sh(FONT_SANS, TFS, x * KS, y * KS - 3, s, c) / KS); }
static void text_draw_n(int x, int y, const char *s, u32 c, int n) { d2_text_n(FONT_SANS, TFS, x * KS, y * KS - 3, s, c, n); }
static int text_wrap(const char *s, int maxw, char out[][256], int maxl) { return d2_wrap(FONT_SANS, TFS, s, maxw * KS, out, maxl); }
static void portrait(const char *sprite, int x, int y, int sz) {
  unsigned t = human_portrait(human_find(sprite));
  d2_rrect(x * KS, y * KS, sz * KS, sz * KS, 8, 0xFF2A2430);
  if (t) d2_image(t, x * KS + 3, y * KS + 3, sz * KS - 6, sz * KS - 6, 0, 1, 1, 0, 0xFFFFFFFF);
  d2_rrect_line(x * KS, y * KS, sz * KS, sz * KS, 8, 1.5f, 0xA0E8B84A);
}

/* ================================================================ UI: narzędzia */
typedef struct { short x, y, w, h, kind, idx; } Zone;
enum { Z_TAB = 1, Z_ROW, Z_MOPT, Z_NEXT, Z_BACK };
static Zone zones[128];
static int nz;
static int lastMx = -1, lastMy = -1, mouseMoved;
static void zone(int x, int y, int w, int h, int kind, int idx) {
  if (nz < 128) zones[nz++] = (Zone){(short)x, (short)y, (short)w, (short)h, (short)kind, (short)idx};
}
static Zone *zone_at(void) {
  if (in.mx < 0) return NULL;
  int mx = (int)(in.mx / KS), my = (int)(in.my / KS);
  for (int i = nz - 1; i >= 0; i--)
    if (mx >= zones[i].x && my >= zones[i].y && mx < zones[i].x + zones[i].w && my < zones[i].y + zones[i].h) return &zones[i];
  return NULL;
}

static void text_r(int xr, int y, const char *s, u32 c) { text_draw_sh(xr - text_width(s), y, s, c); }
static void text_c(int xc, int y, const char *s, u32 c) { text_draw_sh(xc - text_width(s) / 2, y, s, c); }
static void bar(int x, int y, int w, int h, int v, int max, u32 c) {
  float r = h * KS * 0.5f;
  d2_rrect(x * KS, y * KS, w * KS, h * KS, r, 0xFF221C24);
  float f = max > 0 ? (w * KS - 4) * CLAMP(v, 0, max) / max : 0;
  if (f > 1) d2_rrect(x * KS + 2, y * KS + 2, f, h * KS - 4, r - 2, c);
}
static void panel(int x, int y, int w, int h) {
  d2_shadow(x * KS, y * KS, w * KS, h * KS, 12, 14, 0x80000000);
  d2_grad(x * KS, y * KS, w * KS, h * KS, 0xE81A161C, 0xE8100D12);
  d2_rrect_line(x * KS, y * KS, w * KS, h * KS, 10, 1.2f, 0x60E8B84A);
}
static void fmt_money(char *b, int n, int v) {
  int a = v < 0 ? -v : v;
  if (a >= 1000000) snprintf(b, n, "%s$%d %03d %03d", v < 0 ? "-" : "", a / 1000000, a / 1000 % 1000, a % 1000);
  else if (a >= 1000) snprintf(b, n, "%s$%d %03d", v < 0 ? "-" : "", a / 1000, a % 1000);
  else snprintf(b, n, "%s$%d", v < 0 ? "-" : "", a);
}
static u32 fcol(int f) { return f < 0 ? pal('d') : pal(FT[f].col); }

static int md; /* liczba otwartych okien (definicja niżej) */
/* ================================================================ kronika: sceny fabularne, gazeta, wizyty */
typedef struct { int day; const char *script, *map; int x, y, dir; const char *title, *l1, *l2, *need; } Beat;
static const Beat BEATS[] = {
  {1, "k_lucia_ksiegi", NULL, 0, 0, 0, "Lucia czeka w gabinecie", "Mówi, że chodzi o księgi ojca.", "I że to nie może czekać.", NULL},
  {5, "k_kessler", "trattoria", 6, 7, DIR_UP, "Gość z Waszyngtonu", "W trattorii siedzi człowiek w szarym płaszczu.", "Pije tylko wodę. Pyta o ciebie po nazwisku.", NULL},
  {10, "k_klub", "klub", 10, 12, DIR_UP, "Wieczór w Blue Moon", "Lucia pyta, czy zabierzesz ją dziś na jazz.", "„Ojciec nigdy mnie nie zabierał. Mówił, że to nie miejsce dla córki.”", NULL},
  {15, "k_mama", "dom", 2, 2, DIR_RIGHT, "List z Polonii", "„Tomuś, przyjdź w niedzielę na obiad. Zrobię gołąbki.", "Dawno cię nie widziałam. Mama.”", NULL},
  {21, "k_bronek", NULL, 0, 0, 0, "Bronek prosi o rozmowę", "Mówi, że to sprawa „między nami, z Polonii”.", NULL, "_BRONEK_JEST"},
  {27, "k_zamach", "trattoria", 12, 4, DIR_LEFT, "Spokojny wieczór", "Kolacja w trattorii. Beppe poleca dziś osso buco.", "Nic nie zapowiada kłopotów.", NULL},
  {36, "k_szczur", NULL, 0, 0, 0, "Gino jest zdyszany", "Gino z warsztatu przybiegł bez czapki.", "Mówi, że widział coś, czego nie powinien był widzieć.", "_BRONEK_JEST"},
  {44, "k_balbo", "italia", 13, 5, DIR_RIGHT, "Balbo nad Chicago!", "Hydroplany generała Balbo wylądowały na jeziorze Michigan.", "Mała Italia świętuje na ulicach. Ludzie chcą zobaczyć ciebie.", NULL},
  {50, "k_kane", "klub", 10, 12, DIR_UP, "Zaproszenie od Kane'a", "Victor Kane zaprasza na rozmowę w Blue Moon.", "„O przyszłości miasta”, jak napisał na wizytówce.", "_KAN_ZYJE"},
  {57, "k_lee", "herbaciarnia", 6, 7, DIR_UP, "Herbata z Lee Wongiem", "Do trattorii przyszła paczka jaśminowej herbaty.", "I zaproszenie wypisane kaligraficznym pismem.", "_TRI_ZYJE"},
  {64, "k_kessler2", NULL, 0, 0, 0, "Kessler wraca", "Agent Kessler stoi w drzwiach trattorii.", "Tym razem nie jest sam.", NULL},
  {72, "k_lucia", "kosciol", 7, 11, DIR_UP, "Lucia u św. Rocha", "Ojciec Bernardo mówi, że Lucia siedzi w kościele od rana.", "Prosiła, żebyś przyszedł. Sam.", NULL},
  {78, "k_vito", NULL, 0, 0, 0, "Rodzinna sprawa", "Vito pił od południa.", "Chce mówić z tobą. Teraz.", "_VITO_JEST"},
  {188, "k_repeal", "klub", 10, 12, DIR_UP, "Koniec prohibicji!", "Ameryka znów może legalnie pić.", "W Blue Moon trwa największa impreza w historii Chicago.", NULL},
};
#define NBEATS ((int)(sizeof(BEATS) / sizeof(BEATS[0])))

static void var_set(const char *n, int v) { H.vars[var_find(n, 1)] = v; }

static int man_by_sprite(const char *sp) {
  for (int i = 0; i < T.nmen; i++) if (!strcmp(T.men[i].sprite, sp)) return i;
  return -1;
}

static int most_hostile(void) {
  int best = -1, bv = 1000;
  for (int f = 1; f < NF; f++) {
    if (!fam_alive(f)) continue;
    int v = T.f[f].rel - (T.f[f].state == F_WAR ? 200 : 0) - T.f[f].soldiers / 4;
    if (v < bv) { bv = v; best = f; }
  }
  return best;
}

/* stan Księgi widoczny dla skryptów fabuły */
static void export_vars(void) {
  var_set("_DZIEN", T.day);
  var_set("_SZACUNEK", T.respect);
  var_set("_GORACZKA", T.heat);
  var_set("_SLEDZTWO", T.invest);
  var_set("_ZOLNIERZE", T.soldiers);
  var_set("_BRONEK_JEST", man_by_sprite("bronek") >= 0);
  var_set("_VITO_JEST", man_by_sprite("vito") >= 0);
  int wars = 0;
  for (int f = 1; f < NF; f++) {
    char b[24];
    snprintf(b, sizeof b, "_%s_ZYJE", FT[f].id);
    for (char *c = b; *c; c++) *c = (char)toupper((unsigned char)*c);
    var_set(b, fam_alive(f));
    if (fam_alive(f) && T.f[f].state == F_WAR) wars++;
  }
  var_set("_WOJNA", wars);
  int h = most_hostile();
  for (int f = 1; f < NF; f++) {
    char b[24];
    snprintf(b, sizeof b, "_WROG_%s", FT[f].id);
    for (char *c = b; *c; c++) *c = (char)toupper((unsigned char)*c);
    var_set(b, f == h);
  }
  var_set("_PROHIBICJA", !T.repealed);
}

static char sceneVar[40];
static void scene_clear(void) {
  if (sceneVar[0]) var_set(sceneVar, 0);
  sceneVar[0] = 0;
}

static void scene_start_at(int mp, int x, int y, int dir, const char *script);
static void scene_start(const char *map, int x, int y, int dir, const char *script) {
  if (!map) { scene_start_at(map_find(S.hqMap), S.hqX, S.hqY, S.hqDir, script); return; }
  int mp;
  if (x == 0 && y == 0) mp = place_find(map, &x, &y, &dir);
  else mp = map_resolve(map, &x, &y);
  scene_start_at(mp, x, y, dir, script);
}
static void scene_start_at(int mp, int x, int y, int dir, const char *script) {
  int sc = script_find(script);
  if (mp < 0 || sc < 0 || g_autoplay) return;
  scene_clear();
  snprintf(sceneVar, sizeof sceneVar, "_S_%s", script);
  for (char *c = sceneVar; *c; c++) *c = (char)toupper((unsigned char)*c);
  var_set(sceneVar, 1);
  export_vars();
  md = 0;
  world_load_map(mp, x, y, dir);
  game_set_mode(MODE_WORLD);
  g_fade = 255; g_autofade = 1;
  script_start(sc, NULL);
}

static void beats_check(void) {
  for (int i = 0; i < NBEATS; i++) {
    if (T.beats & (1 << i) || T.day < BEATS[i].day) continue;
    if (BEATS[i].need) { export_vars(); if (!var_or(BEATS[i].need, 0)) { T.beats |= 1 << i; continue; } }
    T.beats |= 1 << i;
    push_ev(EV_BEAT, i, 0);
    break; /* najwyżej jedna scena dziennie */
  }
}

static void beat_view(const Ev *e, EvView *v) {
  const Beat *b = &BEATS[e->a];
  snprintf(v->title, 48, "%s", b->title);
  snprintf(v->text[v->ntext++], 100, "%s", b->l1);
  if (b->l2) snprintf(v->text[v->ntext++], 100, "%s", b->l2);
  snprintf(v->opt[v->nopt++], 60, "Jedź (scena w świecie 3D)");
}

static void beat_apply(const Ev *e) {
  const Beat *b = &BEATS[e->a];
  scene_start(b->map, b->x, b->y, b->dir, b->script);
}

/* ---------------------------------------------------------------- gazeta */
typedef struct { int day; const char *head, *sub; } News;
static const News HIST[] = {
  {0, "WYSTAWA ŚWIATOWA OTWARTA NAD JEZIOREM", "„Stulecie Postępu” przyciąga tłumy. Burmistrz Kelly: „Chicago pokazuje światu przyszłość”."},
  {14, "FEDERALNI OBIECUJĄ: SKOŃCZYMY Z GANGAMI", "Prokurator generalny zapowiada nowy wydział do walki z przestępczością zorganizowaną."},
  {28, "ROOSEVELT PODPISUJE USTAWĘ O ODBUDOWIE", "Błękitny Orzeł NRA w witrynach sklepów. „Robimy swoje” — piszą kupcy z Halsted Street."},
  {42, "BALBO I JEGO ESKADRA LĄDUJĄ W CHICAGO!", "24 hydroplany na jeziorze Michigan. Tysiące Włochów wiwatuje na brzegu."},
  {56, "PROCES ROGERA TOUHY'EGO W CIENIU SKANDALU", "Obrona twierdzi, że oskarżenie o porwanie Factora to spisek. Miasto wstrzymuje oddech."},
  {70, "CAPONE WCIĄŻ ZA KRATAMI W ATLANCIE", "Były król Chicago pisze listy do rodziny. „Nikt już nie pamięta jego nazwiska” — mówi policja."},
  {84, "„NIE STRZELAJCIE, G-MEN!”", "Machine Gun Kelly aresztowany w Memphis. Federalni mają nowy przydomek."},
  {98, "WYSTAWA BIJE REKORD: 22 MILIONY GOŚCI", "Hale Postępu zamykają się na zimę. Organizatorzy zapowiadają drugi sezon."},
  {112, "ZIMA NADCHODZI, KOLEJKI PO ZUPĘ ROSNĄ", "Na West Madison Street tysiąc bezrobotnych czeka na miskę grochówki."},
  {126, "STANY GŁOSUJĄ NAD 21. POPRAWKĄ", "Utah może przesądzić o końcu prohibicji. Barmani z Loopu ostrzą korkociągi."},
  {182, "PROHIBICJA NIE ŻYJE! AMERYKA ZNÓW PIJE", "Tłumy na ulicach Loopu. „Szczęśliwe dni znowu tu są” — śpiewają w każdym barze."},
  {196, "ŚWIĘTA W MIEŚCIE WIATRU", "Pierwsze od czternastu lat legalne Boże Narodzenie z kieliszkiem. Kościoły pełne."},
  {224, "DILLINGER UCIEKA Z WIĘZIENIA W CROWN POINT", "„Wrogiem publicznym numer jeden” podobno wystrugał drewniany pistolet."},
};

static void news_view(const Ev *e, EvView *v) {
  int week = e->b;
  char ds[40];
  date_str(T.day, ds, sizeof ds);
  snprintf(v->title, 48, "Chicago Daily Herald");
  snprintf(v->text[v->ntext++], 100, "{s}%s • 3 centy • wydanie poranne{-}", ds);
  const News *h = NULL;
  for (int i = 0; i < (int)(sizeof HIST / sizeof HIST[0]); i++) if (HIST[i].day <= week * 7 + 6 && HIST[i].day >= week * 7 - 7) h = &HIST[i];
  if (h) {
    snprintf(v->text[v->ntext++], 100, "{y}%s{-}", h->head);
    snprintf(v->text[v->ntext++], 100, "%s", h->sub);
  }
  /* wiadomości z naszego półświatka */
  const char *lh = NULL, *ls = NULL;
  static char b1[100], b2[100];
  int hf = most_hostile();
  if (T.lastWar > 0 && hf > 0) {
    snprintf(b1, sizeof b1, "STRZELANINA W %s", DT[FT[hf].hq].name);
    for (char *c = b1; *c; c++) if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 32);
    snprintf(b2, sizeof b2, "Policja łączy ją z wojną gangów. Ludzie „%s” nie komentują.", FT[hf].boss);
    lh = b1; ls = b2;
  } else if (T.invest >= 50) {
    lh = "URZĄD SKARBOWY BADA KSIĘGI TRATTORII";
    ls = "Anonimowy urzędnik: „Liczby się nie zgadzają. A liczby nie kłamią”.";
  } else if (T.heat >= 55) {
    lh = "KOMISARZ ZAPOWIADA CZYSTKĘ NA ULICACH";
    ls = "Nocne naloty na meliny w kilku dzielnicach. Właściciele milczą.";
  } else if (dist_count(0) >= 4) {
    snprintf(b1, sizeof b1, "KIM JEST „POLAK” Z TAYLOR STREET?");
    snprintf(b2, sizeof b2, "Rodzina Marino kontroluje już %d dzielnic, szepczą na posterunkach.", dist_count(0));
    lh = b1; ls = b2;
  } else if (T.respect >= 50) {
    lh = "DATKI DLA SIEROCIŃCA OD „ANONIMOWEGO DOBROCZYŃCY”";
    ls = "Siostry z parafii św. Rocha dziękują. Wszyscy wiedzą, kto to.";
  } else {
    static const char *H2[][2] = {
      {"NOCNY POŻAR W MAGAZYNIE NA LEVEE", "Strażacy znaleźli puste beczki po piwie. Właściciel nieznany."},
      {"CICHE ULICE MAŁEJ ITALII", "Kupcy z Taylor Street: „Ostatnio nikt nie przychodzi po pieniądze”."},
      {"RADNY KOWALCZYK OBIECUJE NOWE LATARNIE", "Polonia czeka na nie od dziesięciu lat."},
      {"PRZEMYTNIK ZATRZYMANY NA MOŚCIE", "W ciężarówce z jabłkami znaleziono 40 skrzynek kanadyjskiej whisky."},
    };
    int k = (week * 7 + T.respect) % 4;
    lh = H2[k][0]; ls = H2[k][1];
  }
  snprintf(v->text[v->ntext++], 100, "{y}%s{-}", lh);
  snprintf(v->text[v->ntext++], 100, "%s", ls);
  snprintf(v->opt[v->nopt++], 60, "Odłóż gazetę");
}

/* ---------------------------------------------------------------- polecenia skryptów: tycoon KLUCZ ... */
void tycoon_script_cmd(const char *key, const char *a, int n) {
  int na = a ? atoi(a) : 0;
  if (!T.active) return;
  if (!strcmp(key, "respect")) T.respect = CLAMP(T.respect + na, 0, 100);
  else if (!strcmp(key, "heat")) T.heat = CLAMP(T.heat + na, 0, 100);
  else if (!strcmp(key, "invest")) T.invest = CLAMP(T.invest + na, 0, 100);
  else if (!strcmp(key, "soldiers")) T.soldiers = CLAMP(T.soldiers + na, 0, 200);
  else if (!strcmp(key, "booze")) T.booze = CLAMP(T.booze + na, 0, 150);
  else if (!strcmp(key, "politician")) T.politician = na;
  else if (!strcmp(key, "rel") || !strcmp(key, "war") || !strcmp(key, "truce") || !strcmp(key, "ally")) {
    int f = -1;
    for (int k = 1; k < NF; k++) if (a && !strcmp(a, FT[k].id)) f = k;
    if (a && !strcmp(a, "wrog")) f = most_hostile();
    if (f <= 0 || !fam_alive(f)) return;
    if (!strcmp(key, "rel")) rel_add(f, n);
    else if (!strcmp(key, "war")) go_war(f);
    else if (!strcmp(key, "truce")) { T.f[f].state = F_TRUCE; T.f[f].stateT = n ? n : 14; }
    else if (!strcmp(key, "ally")) { T.f[f].state = F_ALLY; rel_add(f, 10); }
  } else if (!strcmp(key, "infl")) {
    int d = na;
    if (d >= 0 && d < ND) infl_take(d, 0, n, -1);
    recompute_owners(0);
  } else if (!strcmp(key, "loyal") || !strcmp(key, "remove") || !strcmp(key, "capo") || !strcmp(key, "hurt")) {
    int i = a ? man_by_sprite(a) : -1;
    if (i < 0) return;
    if (!strcmp(key, "loyal")) T.men[i].loyal = CLAMP(T.men[i].loyal + n, 0, 100);
    else if (!strcmp(key, "capo")) { T.men[i].capo = 1; T.men[i].brains = CLAMP(T.men[i].brains + 1, 1, 10); }
    else if (!strcmp(key, "hurt")) { unassign(i); T.men[i].status = MS_HURT; T.men[i].statusT = n ? n : 5; }
    else { unassign(i); remove_man(i); }
  } else if (!strcmp(key, "spawn")) {
    /* wróg z najbardziej wrogiej rodziny: tycoon spawn X Y */
    world_spawn(enemy_for(most_hostile(), 0), na, n, 0);
  } else if (!strcmp(key, "log")) {
    if (a) tlog("%s", a);
  }
  export_vars();
}

/* ---------------------------------------------------------------- wizyty we własnych lokalach */
static const char *visit_map(int type) {
  switch (type) {
    case B_MELINA: return "melina";
    case B_KASYNO: return "kasyno";
    case B_BUKMACHER: return "bukmacher";
    case B_KLUB: return "klub";
    case B_BOKS: return "hala";
    case B_RESTAUR: return "trattoria";
    case B_BROWAR: return "browar";
    case B_PORT: return "doki";
  }
  return NULL;
}

static int visit_ok(int i) {
  const char *m = visit_map(T.biz[i].type);
  return m && map_find(m) >= 0 && !T.biz[i].closed && !MS.active;
}

static void visit_start(int i) {
  const char *m = visit_map(T.biz[i].type);
  char sc[40];
  snprintf(sc, sizeof sc, "w_%s", m);
  var_set("_WIZ_POZIOM", T.biz[i].lvl);
  var_set("_WIZ_ZARZADCA", T.biz[i].manager >= 0);
  var_set("_WIZ_DZIS", T.visitDay == T.day + 1);
  T.visitDay = T.day + 1;
  int x, y, d, mp = place_find(m, &x, &y, &d);
  if (script_find(sc) >= 0) scene_start_at(mp, x, y, d, sc);
  else { export_vars(); md = 0; world_load_map(mp, x, y, d); game_set_mode(MODE_WORLD); g_fade = 255; g_autofade = 1; }
}

/* test: scena kroniki (beat >= 0) albo wizyta w lokalu typu -1-beat */
int tycoon_debug_beat(int b) {
  if (!T.active) tycoon_start(60);
  int ga = g_autoplay;
  g_autoplay = 0;
  if (b >= 0 && b < NBEATS) { Ev e = {EV_BEAT, b, 0}; T.beats |= 1 << b; beat_apply(&e); }
  else if (b == 100) { T.nev = 0; push_ev(EV_NEWS, 0, T.day / 7); tycoon_open(); }
  else if (b < 0) {
    int type = -1 - b;
    T.biz[T.nbiz++] = (Biz){type, 0, 1, -1, 0, 0};
    visit_start(T.nbiz - 1);
  }
  g_autoplay = ga;
  return b;
}

/* ================================================================ UI: okna modalne */
enum { MK_INFO = 1, MK_EVENT, MK_DIST, MK_BUILD, MK_BIZ, MK_MANAGER, MK_MAN, MK_GUARD, MK_OPTARGET, MK_TEAM, MK_FAM, MK_PICKDIST, MK_REPORT };
#define MOPT 16
typedef struct {
  int kind, a, b;
  char title[64];
  char text[8][100]; int ntext;
  char opt[MOPT][72]; char desc[MOPT][100]; int en[MOPT], val[MOPT]; int nopt, cur, scroll;
  int pick[MAXMEN]; int soldiers;
} Modal;
static Modal MD[4];
static int md;
static int tab, tsel[8], tscroll[8];
enum { TAB_OVER, TAB_MAP, TAB_BIZ, TAB_MEN, TAB_OPS, TAB_FAM, TAB_LOG, TAB_N };
static const char *TABN[TAB_N] = {"Przegląd", "Miasto", "Biznesy", "Ludzie", "Akcje", "Rodziny", "Dziennik"};

static Modal *mopen(int kind, const char *title) {
  if (md >= 4) md = 3;
  Modal *m = &MD[md++];
  memset(m, 0, sizeof *m);
  m->kind = kind;
  snprintf(m->title, sizeof m->title, "%s", title);
  return m;
}
static void mtext(Modal *m, const char *fmt, ...) {
  if (m->ntext >= 8) return;
  va_list ap; va_start(ap, fmt); vsnprintf(m->text[m->ntext++], 100, fmt, ap); va_end(ap);
}
static void mopt(Modal *m, int en, int val, const char *desc, const char *fmt, ...) {
  if (m->nopt >= MOPT) return;
  va_list ap; va_start(ap, fmt); vsnprintf(m->opt[m->nopt], 72, fmt, ap); va_end(ap);
  snprintf(m->desc[m->nopt], 100, "%s", desc ? desc : "");
  m->en[m->nopt] = en; m->val[m->nopt] = val;
  m->nopt++;
}
static void mclose(void) { if (md > 0) md--; }

static void err_toast(int r) {
  static const char *E[] = {"", "Za mało pieniędzy.", "Brak wolnych miejsc w dzielnicy.", "Za małe wpływy w dzielnicy (min. 25%).",
                            "Nie można tego zrobić.", "Limit osiągnięty.", "Odmówili."};
  if (r > 0 && r <= R_REFUSED) { ui_toast(E[r]); sfx_play(SFX_CANCEL); }
}

static void build_team(Modal *m);
static void open_events(void);

static void open_dist(int d) {
  char t[64];
  snprintf(t, sizeof t, "%s", DT[d].name);
  Modal *m = mopen(MK_DIST, t);
  m->a = d;
  int bn = biz_in(d);
  char b[100];
  snprintf(b, sizeof b, "Miejsca: %d/%d, twoje wpływy: %d%%", bn, DT[d].slots, T.d[d].infl[0]);
  mopt(m, T.d[d].infl[0] >= 25 && bn < DT[d].slots, 0, b, "Otwórz biznes");
  snprintf(b, sizeof b, "Poziom %d/3. Obrona dzielnicy: %d.", T.d[d].fort, dist_defense(d));
  mopt(m, T.d[d].fort < 3 && T.d[d].infl[0] > 0, 1, b, "Wzmocnij ochronę ($%d)", fort_cost(d));
  mopt(m, op_target_valid(OP_HARACZ, d), 2, OT[OP_HARACZ].desc, "Akcja: haracz");
  mopt(m, op_target_valid(OP_ATAK, d), 3, OT[OP_ATAK].desc, "Akcja: atak na dzielnicę");
  int qx, qy, qd, mp = district_place(d, &qx, &qy, &qd);
  mopt(m, mp >= 0 && !MS.active, 4, mp >= 0 ? "Pojedź tam samochodem (świat 3D)." : "Do tej dzielnicy nie ma dojazdu.", "Jedź tam");
  mopt(m, 1, 9, NULL, "Wróć");
}

static void open_build(int d) {
  char t[64];
  snprintf(t, sizeof t, "Nowy biznes: %s", DT[d].name);
  Modal *m = mopen(MK_BUILD, t);
  m->a = d;
  for (int k = 0; k < B_N; k++) {
    if (BT[k].dockOnly && !DT[d].dock) continue;
    char desc[100];
    snprintf(desc, sizeof desc, "%s (ok. $%d/dzień)", BT[k].desc, (int)(BT[k].income * 0.7f * (0.6f + 0.2f * DT[d].wealth)));
    mopt(m, can_build(d, k) == R_OK, k, desc, "%s\t$%d", BT[k].name, BT[k].cost);
  }
  mopt(m, 1, -1, NULL, "Anuluj");
}

static void open_biz(int i) {
  Biz *b = &T.biz[i];
  char t[64];
  snprintf(t, sizeof t, "%s — %s", BT[b->type].name, DT[b->dist].name);
  Modal *m = mopen(MK_BIZ, t);
  m->a = i;
  mtext(m, "Poziom %d, dochód ok. $%d/dzień, zarobił łącznie $%ld.", b->lvl, biz_income(i, 1), b->earned);
  mtext(m, "Zarządca: %s", b->manager >= 0 ? T.men[b->manager].name : "brak");
  if (b->closed) mtext(m, "Zamknięty jeszcze przez %d dni.", b->closed);
  mopt(m, b->lvl < 3 && H.gold >= upgrade_cost(i), 0, "Większy lokal, więcej klientów: x1,6 / x2,3 dochodu.", b->lvl < 3 ? "Rozbuduj ($%d)" : "Maksymalny poziom", upgrade_cost(i));
  mopt(m, T.nmen > 0, 1, "Zarządca zwiększa dochód o 6% za każdy punkt sprytu.", "Wyznacz zarządcę");
  mopt(m, b->manager >= 0, 2, NULL, "Odwołaj zarządcę");
  mopt(m, 1, 3, "Połowa wartości inwestycji wraca do kasy.", "Sprzedaj (+$%d)", sell_value(i));
  if (visit_map(b->type)) mopt(m, visit_ok(i), 4, b->closed ? "Lokal jest zamknięty." : "Zajrzyj osobiście: pogadaj z ludźmi, załatw sprawy na miejscu.", "Odwiedź lokal (świat 3D)");
  mopt(m, 1, 9, NULL, "Wróć");
}

static const char *man_status(const Man *m, char *b, int n) {
  if (m->status == MS_HURT) snprintf(b, n, "ranny (%d dni)", m->statusT);
  else if (m->status == MS_JAIL) snprintf(b, n, "areszt (%d dni)", m->statusT);
  else if (m->status == MS_OP) snprintf(b, n, "na akcji");
  else if (m->assign >= 1000) snprintf(b, n, "ochrona: %s", DT[m->assign - 1000].name);
  else if (m->assign >= 0) snprintf(b, n, "zarządza: %s", BT[T.biz[m->assign].type].name);
  else snprintf(b, n, "wolny");
  return b;
}

static void open_pick_man(int kind, int a, const char *title) {
  Modal *m = mopen(kind, title);
  m->a = a;
  for (int i = 0; i < T.nmen; i++) {
    char st[48], d[100];
    man_status(&T.men[i], st, sizeof st);
    snprintf(d, sizeof d, "Walka %d, spryt %d, lojalność %d. Teraz: %s.", T.men[i].fight, T.men[i].brains, T.men[i].loyal, st);
    mopt(m, T.men[i].status == MS_OK || T.men[i].status == MS_HURT, i, d, "%s „%s”", T.men[i].name, T.men[i].nick);
  }
  mopt(m, 1, -1, NULL, "Anuluj");
}

static void open_man(int i) {
  Man *mn = &T.men[i];
  char t[64];
  snprintf(t, sizeof t, "%s „%s”", mn->name, mn->nick);
  Modal *m = mopen(MK_MAN, t);
  m->a = i;
  char st[48];
  mtext(m, "Walka %d   Spryt %d   Lojalność %d   Poziom %d%s", mn->fight, mn->brains, mn->loyal, mn->lvl, mn->capo ? "   CAPO" : "");
  mtext(m, "Pensja $%d/dzień. Akcje: %d. Teraz: %s.", mn->wage, mn->ops, man_status(mn, st, sizeof st));
  int ok = mn->status == MS_OK || mn->status == MS_HURT;
  mopt(m, ok, 0, "Strażnik podnosi obronę dzielnicy przed atakami.", "Przydziel do ochrony dzielnicy");
  mopt(m, ok && mn->assign >= 0, 1, NULL, "Zwolnij z przydziału");
  mopt(m, H.gold >= 200, 2, "Lojalność +15.", "Daj premię ($200)");
  mopt(m, !mn->capo && mn->lvl >= 3 && H.gold >= 1000, 3, "Capo: +1 walka, +1 spryt, silniejsze akcje. Wymaga poziomu 3.", "Awansuj na capo ($1000)");
  mopt(m, mn->status != MS_OP, 4, "Odchodzi z Rodziny. Inni ludzie trochę się zaniepokoją.", "Wyrzuć z Rodziny");
  mopt(m, 1, 9, NULL, "Wróć");
}

static void open_guard(int man) {
  Modal *m = mopen(MK_GUARD, "Ochrona dzielnicy");
  m->a = man;
  for (int d = 0; d < ND; d++) {
    char b[100];
    snprintf(b, sizeof b, "Obrona teraz: %d, nasze wpływy %d%%.", dist_defense(d), T.d[d].infl[0]);
    mopt(m, T.d[d].infl[0] > 0, d, b, "%s", DT[d].name);
  }
  mopt(m, 1, -1, NULL, "Anuluj");
}

static void open_optarget(int type) {
  Modal *m = mopen(MK_OPTARGET, OT[type].name);
  m->a = type;
  mtext(m, "%s", OT[type].desc);
  if (OT[type].tgt == TG_DIST)
    for (int d = 0; d < ND; d++) {
      char b[100];
      int f = strongest_rival_in(d);
      snprintf(b, sizeof b, "Trudność %d. Wpływy: nasze %d%%%s%s.", op_diff(type, d), T.d[d].infl[0], f > 0 ? ", najsilniejsi: " : "", f > 0 ? FT[f].name : "");
      mopt(m, op_target_valid(type, d), d, b, "%s", DT[d].name);
    }
  else if (OT[type].tgt == TG_FAM)
    for (int f = 1; f < NF; f++) {
      char b[100];
      snprintf(b, sizeof b, "Trudność %d. Żołnierze: %d. Stan: %s.", op_diff(type, f), T.f[f].soldiers, FSTATE[T.f[f].state]);
      mopt(m, op_target_valid(type, f), f, b, "%s", FT[f].name);
    }
  else if (OT[type].tgt == TG_HEIST)
    for (int h = 0; h < 3; h++) {
      char b[100];
      snprintf(b, sizeof b, "Trudność %d. Łup $%d-%d. Gorączka +%d.", HEIST_DIFF[h], HEIST_MIN[h], HEIST_MAX[h], HEIST_HEAT[h]);
      mopt(m, 1, h, b, "%s", HEIST[h]);
    }
  mopt(m, 1, -1, NULL, "Anuluj");
}

static void open_team(int type, int target) {
  char t[64];
  snprintf(t, sizeof t, "Ekipa: %s", OT[type].name);
  Modal *m = mopen(MK_TEAM, t);
  m->a = type; m->b = target;
  m->soldiers = T.soldiers < 4 ? T.soldiers : 4;
  /* domyślnie najlepszy wolny człowiek */
  int best = -1, bv = -1;
  for (int i = 0; i < T.nmen; i++)
    if (T.men[i].status == MS_OK && T.men[i].assign < 0) { int v = OT[type].fightOp ? T.men[i].fight : T.men[i].brains; if (v > bv) { bv = v; best = i; } }
  if (best >= 0) m->pick[best] = 1;
  build_team(m);
}

static void build_team(Modal *m) {
  int type = m->a, target = m->b;
  int keep = m->cur;
  m->nopt = 0; m->ntext = 0;
  int team[4], n = 0;
  for (int i = 0; i < T.nmen; i++) if (m->pick[i] && n < 4) team[n++] = i;
  mtext(m, "Wybierz do 4 ludzi (strażnicy i zarządcy zostaną odwołani).");
  for (int i = 0; i < T.nmen; i++) {
    Man *mn = &T.men[i];
    char st[48], d[100];
    man_status(mn, st, sizeof st);
    snprintf(d, sizeof d, "Walka %d, spryt %d, poziom %d. Teraz: %s.", mn->fight, mn->brains, mn->lvl, st);
    mopt(m, mn->status == MS_OK && (m->pick[i] || n < 4), 100 + i, d, "[%s] %s „%s”", m->pick[i] ? "x" : " ", mn->name, mn->nick);
  }
  mopt(m, T.soldiers > 0, 50, "Strzałki w lewo/prawo zmieniają liczbę żołnierzy.", "Żołnierze: < %d >  (z %d)", m->soldiers, T.soldiers);
  int ch = op_chance(type, target, team, n, m->soldiers);
  int okTeam = n >= OT[type].minTeam || m->soldiers >= 3;
  char d[100];
  snprintf(d, sizeof d, "Wynik poznasz za %d %s. Trudność %d, siła ekipy %d.", OT[type].days, OT[type].days == 1 ? "dzień" : "dni", op_diff(type, target), team_power(type, team, n, m->soldiers));
  mopt(m, okTeam && ops_active() < MAXOPS, 60, d, "Wyślij ekipę (szansa %d%%)", ch);
  if (op_map(type) >= 0)
    mopt(m, okTeam && !MS.active, 61, "Prowadzisz akcję sam, w 3D. Wygrana = pewny sukces, ludzie bezpieczniejsi.", "Poprowadź osobiście (3D)");
  mopt(m, 1, -1, NULL, "Anuluj");
  m->cur = keep < m->nopt ? keep : 0;
}

static void open_fam(int f) {
  char t[64];
  snprintf(t, sizeof t, "%s", FT[f].name);
  Modal *m = mopen(MK_FAM, t);
  m->a = f;
  Fam *F = &T.f[f];
  mtext(m, "Boss: %s%s. Stan: %s%s.", FT[f].boss, F->bossDead ? " (nie żyje)" : "", FSTATE[F->state], F->state == F_TRUCE ? " (dni)" : "");
  mtext(m, "Stosunki: %+d.  Żołnierze: %d.  Dzielnice: %d.", F->rel, F->soldiers, dist_count(f));
  mtext(m, "Nasza siła: %d,  ich siła: %d.", player_power(), fam_power(f));
  for (int a = 0; a < DP_N; a++) mopt(m, dp_available(f, a), a, dp_hint(f, a), "%s", DPN[a]);
  mopt(m, 1, -1, NULL, "Wróć");
}

static void open_report(void) {
  Modal *m = mopen(MK_REPORT, "Raport dnia");
  int from = T.ntoday > 8 ? T.ntoday - 8 : 0;
  for (int i = from; i < T.ntoday; i++) mtext(m, "%s", T.today[i]);
  mopt(m, 1, 0, NULL, "Dalej");
}

static void open_events(void) {
  if (!T.nev || md) return;
  Ev *e = &T.ev[0];
  if (e->id == EV_RAISE || e->id == EV_RAT) if (e->a < 0 || e->a >= T.nmen) { memmove(T.ev, T.ev + 1, sizeof(Ev) * (--T.nev)); open_events(); return; }
  EvView v;
  ev_view(e, &v);
  Modal *m = mopen(MK_EVENT, v.title);
  for (int i = 0; i < v.ntext; i++) mtext(m, "%s", v.text[i]);
  for (int i = 0; i < v.nopt; i++) mopt(m, 1, i, NULL, "%s", v.opt[i]);
}

static void after_day(void) {
  open_report();
  sfx_play(SFX_OK);
}

/* aktywacja wybranej opcji okna */
static void mactivate(void) {
  Modal *m = &MD[md - 1];
  if (m->cur < 0 || m->cur >= m->nopt) return;
  if (!m->en[m->cur]) { sfx_play(SFX_CANCEL); return; }
  int v = m->val[m->cur];
  int a = m->a, b = m->b;
  sfx_play(SFX_OK);
  switch (m->kind) {
    case MK_INFO: mclose(); break;
    case MK_REPORT: mclose(); open_events(); break;
    case MK_EVENT: {
      Ev e = T.ev[0];
      memmove(T.ev, T.ev + 1, sizeof(Ev) * (T.nev - 1));
      T.nev--;
      mclose();
      ev_apply(&e, v);
      recompute_owners(1);
      if (g_mode == MODE_TYCOON) open_events();
      break;
    }
    case MK_DIST:
      if (v == 0) open_build(a);
      else if (v == 1) { err_toast(t_fort(a)); mclose(); open_dist(a); }
      else if (v == 2) open_team(OP_HARACZ, a);
      else if (v == 3) open_team(OP_ATAK, a);
      else if (v == 4) {
        int x, y, d, mp = district_place(a, &x, &y, &d);
        if (mp >= 0) { md = 0; world_load_map(mp, x, y, d); game_set_mode(MODE_WORLD); g_fade = 255; g_autofade = 1; }
      } else mclose();
      break;
    case MK_PICKDIST: if (v < 0) mclose(); else { mclose(); open_build(v); } break;
    case MK_BUILD:
      if (v < 0) { mclose(); break; }
      { int r = t_build(a, v); if (r) err_toast(r); else { mclose(); if (md && MD[md - 1].kind == MK_DIST) { mclose(); open_dist(a); } ui_toast("Biznes otwarty!"); sfx_play(SFX_CHEST); } }
      break;
    case MK_BIZ:
      if (v == 0) { err_toast(t_upgrade(a)); mclose(); open_biz(a); }
      else if (v == 1) open_pick_man(MK_MANAGER, a, "Wybierz zarządcę");
      else if (v == 2) { unassign(T.biz[a].manager); mclose(); open_biz(a); }
      else if (v == 3) { t_sell(a); mclose(); tsel[TAB_BIZ] = 0; }
      else if (v == 4) visit_start(a);
      else mclose();
      break;
    case MK_MANAGER:
      if (v >= 0) { t_manager(v, a); mclose(); mclose(); open_biz(a); } else mclose();
      break;
    case MK_MAN: {
      Man *mn = &T.men[a];
      if (v == 0) open_guard(a);
      else if (v == 1) { unassign(a); mclose(); open_man(a); }
      else if (v == 2) { H.gold -= 200; mn->loyal = CLAMP(mn->loyal + 15, 0, 100); mclose(); open_man(a); }
      else if (v == 3) { H.gold -= 1000; mn->capo = 1; mn->fight = CLAMP(mn->fight + 1, 1, 10); mn->brains = CLAMP(mn->brains + 1, 1, 10); mn->wage += 20; tlog("%s zostaje capo!", mn->name); mclose(); open_man(a); }
      else if (v == 4) { tlog("%s „%s” odchodzi z Rodziny.", mn->name, mn->nick); unassign(a); remove_man(a); for (int i = 0; i < T.nmen; i++) T.men[i].loyal = CLAMP(T.men[i].loyal - 2, 0, 100); mclose(); tsel[TAB_MEN] = 0; }
      else mclose();
      break;
    }
    case MK_GUARD:
      if (v >= 0) { t_guard(a, v); mclose(); mclose(); open_man(a); } else mclose();
      break;
    case MK_OPTARGET:
      if (v < 0) mclose(); else { mclose(); open_team(a, v); }
      break;
    case MK_TEAM: {
      if (v >= 100) { m->pick[v - 100] = !m->pick[v - 100]; build_team(m); break; }
      if (v == 50) break;
      if (v < 0) { mclose(); break; }
      int team[4], n = 0;
      for (int i = 0; i < T.nmen; i++) if (m->pick[i] && n < 4) team[n++] = i;
      int sol = m->soldiers;
      if (v == 60) {
        int r = t_launch(a, b, team, n, sol);
        if (r) err_toast(r);
        else { tlog("Ekipa wyruszyła: %s.", OT[a].name); ui_toast("Ekipa wyruszyła. Wynik po zakończeniu dnia."); md = 0; }
      } else if (v == 61) {
        md = 0;
        int r = mission_start(a, b, team, n, sol);
        if (r) err_toast(r);
      }
      break;
    }
    case MK_FAM:
      if (v < 0) { mclose(); break; }
      { int r = t_diplo(a, v); if (r == R_REFUSED) { ui_toast("Odmówili."); sfx_play(SFX_CANCEL); } else if (r) err_toast(r); else ui_toast("Załatwione."); recompute_owners(1); mclose(); open_fam(a); }
      break;
  }
}

/* ================================================================ UI: główny ekran */
static int tab_rows(int t) {
  switch (t) {
    case TAB_OVER: return 1;
    case TAB_MAP: return ND;
    case TAB_BIZ: return T.nbiz + 1;
    case TAB_MEN: return T.nmen + 1 + T.nrec;
    case TAB_OPS: return OPT_N;
    case TAB_FAM: return NF - 1;
    case TAB_LOG: return T.nlog;
  }
  return 0;
}

static void tab_activate(void) {
  int s = tsel[tab];
  switch (tab) {
    case TAB_OVER: tycoon_end_day(); if (g_mode == MODE_TYCOON && !T.ended) after_day(); break;
    case TAB_MAP: open_dist(s); sfx_play(SFX_OK); break;
    case TAB_BIZ:
      if (s == 0) {
        Modal *m = mopen(MK_PICKDIST, "Gdzie otworzyć biznes?");
        for (int d = 0; d < ND; d++) {
          char b[100];
          snprintf(b, sizeof b, "Miejsca: %d/%d, nasze wpływy: %d%%, bogactwo: %d/5.", biz_in(d), DT[d].slots, T.d[d].infl[0], DT[d].wealth);
          mopt(m, T.d[d].infl[0] >= 25 && biz_in(d) < DT[d].slots, d, b, "%s", DT[d].name);
        }
        mopt(m, 1, -1, NULL, "Anuluj");
      } else open_biz(s - 1);
      sfx_play(SFX_OK);
      break;
    case TAB_MEN:
      if (s < T.nmen) { open_man(s); sfx_play(SFX_OK); }
      else if (s == T.nmen) { int r = t_soldiers(); if (r) err_toast(r); else { ui_toast("Pięciu nowych żołnierzy."); sfx_play(SFX_CHEST); } }
      else { int r = t_hire(s - T.nmen - 1); if (r) err_toast(r); else { sfx_play(SFX_CHEST); ui_toast("Nowy człowiek w Rodzinie!"); } }
      break;
    case TAB_OPS:
      sfx_play(SFX_OK);
      if (OT[s].tgt == TG_NONE) open_team(s, 0); else open_optarget(s);
      break;
    case TAB_FAM: sfx_play(SFX_OK); open_fam(s + 1); break;
    default: break;
  }
}

void tycoon_open(void) {
  if (!T.active) {
    int v = var_find("SZEF", 0);
    if (v < 0 || !H.vars[v]) return;
    tycoon_start(H.vars[var_find("ZAUFANIE", 1)]); /* awaryjnie: zapis sprzed startu */
  }
  scene_clear();
  game_set_mode(MODE_TYCOON);
  music_play("biuro");
  audio_ambience(AMB_OFFICE, 0.7f, 1);
  input_clear();
  if (!md && T.nev) open_events();
}

static void close_ledger(void) {
  md = 0;
  game_set_mode(MODE_WORLD);
  world_audio();
}

void tycoon_update(void) {
  mouseMoved = in.mx != lastMx || in.my != lastMy;
  lastMx = in.mx; lastMy = in.my;
  Zone *z = zone_at();
  if (md) {
    Modal *m = &MD[md - 1];
    if (in.rep[BTN_UP] || in.rep[BTN_TL]) { m->cur = (m->cur + m->nopt - 1) % m->nopt; sfx_play(SFX_BLIP); }
    if (in.rep[BTN_DOWN] || in.rep[BTN_TR] || in.pressed[BTN_WNEXT]) { m->cur = (m->cur + 1) % m->nopt; sfx_play(SFX_BLIP); }
    if (m->kind == MK_TEAM && m->val[m->cur] == 50) {
      int d = (in.rep[BTN_RIGHT] || in.rep[BTN_SR]) - (in.rep[BTN_LEFT] || in.rep[BTN_SL]);
      if (d) { m->soldiers = CLAMP(m->soldiers + d, 0, T.soldiers); build_team(m); sfx_play(SFX_BLIP); }
    }
    if (z && z->kind == Z_MOPT && mouseMoved) m->cur = z->idx;
    if (in.pressed[BTN_A] || (in.click && z && z->kind == Z_MOPT)) { mactivate(); return; }
    if (in.click && z && z->kind == Z_ROW && m->kind == MK_TEAM) { /* klik w strzałki żołnierzy */
      m->soldiers = CLAMP(m->soldiers + z->idx, 0, T.soldiers); build_team(m); return;
    }
    if (in.pressed[BTN_B] && m->kind != MK_EVENT && m->kind != MK_REPORT) { mclose(); sfx_play(SFX_CANCEL); }
    return;
  }
  int rows = tab_rows(tab);
  if (in.pressed[BTN_LEFT] || in.pressed[BTN_SL] || in.pressed[BTN_TL]) { tab = (tab + TAB_N - 1) % TAB_N; sfx_play(SFX_BLIP); }
  if (in.pressed[BTN_RIGHT] || in.pressed[BTN_SR] || in.pressed[BTN_TR]) { tab = (tab + 1) % TAB_N; sfx_play(SFX_BLIP); }
  if (tab == TAB_LOG) {
    if (in.rep[BTN_UP]) tscroll[TAB_LOG]++;
    if (in.rep[BTN_DOWN] || in.pressed[BTN_WNEXT]) tscroll[TAB_LOG]--;
    tscroll[TAB_LOG] = CLAMP(tscroll[TAB_LOG], 0, T.nlog > 18 ? T.nlog - 18 : 0);
  } else if (rows > 0) {
    if (in.rep[BTN_UP]) { tsel[tab] = (tsel[tab] + rows - 1) % rows; sfx_play(SFX_BLIP); }
    if (in.rep[BTN_DOWN] || in.pressed[BTN_WNEXT]) { tsel[tab] = (tsel[tab] + 1) % rows; sfx_play(SFX_BLIP); }
  }
  if (tsel[tab] >= rows) tsel[tab] = rows > 0 ? rows - 1 : 0;
  if (z && mouseMoved && z->kind == Z_ROW) tsel[tab] = z->idx;
  if (in.click && z) {
    if (z->kind == Z_TAB) { tab = z->idx; sfx_play(SFX_BLIP); return; }
    if (z->kind == Z_NEXT) { tycoon_end_day(); if (g_mode == MODE_TYCOON && !T.ended) after_day(); return; }
    if (z->kind == Z_BACK) { close_ledger(); return; }
    if (z->kind == Z_ROW) { tsel[tab] = z->idx; tab_activate(); return; }
  }
  if (in.pressed[BTN_A]) { tab_activate(); return; }
  if (in.pressed[BTN_N]) { tycoon_end_day(); if (g_mode == MODE_TYCOON && !T.ended) after_day(); return; }
  if (in.pressed[BTN_B] || in.pressed[BTN_TAB]) { close_ledger(); sfx_play(SFX_CANCEL); }
}

/* ---------------------------------------------------------------- rysowanie zakładek */
#define CY 38
#define CH 214

static void draw_row_bg(int x, int y, int w, int h, int selected, int idx) {
  if (selected) { gfx_rect_a(x, y, w, h, pal('b'), 90); gfx_rect(x, y, 2, h, pal('y')); }
  else if (idx % 2) gfx_rect_a(x, y, w, h, RGB(0x30, 0x28, 0x38), 60);
  zone(x, y, w, h, Z_ROW, idx);
}

static void draw_over(void) {
  char b[100], m1[32];
  panel(8, CY, 226, CH);
  int y = CY + 6;
  text_draw_sh(16, y, "STAN RODZINY", pal('z')); y += 15;
#define LINE(lbl, val, col) do { text_draw_sh(16, y, lbl, pal('s')); text_r(226, y, val, col); y += 12; } while (0)
  fmt_money(m1, sizeof m1, H.gold);
  LINE("Gotówka", m1, H.gold < 0 ? pal('r') : pal('l'));
  int net = T.lastIncome - T.lastCosts;
  fmt_money(m1, sizeof m1, T.lastIncome);
  snprintf(b, sizeof b, "+%s", m1); LINE("Wpływy (wczoraj)", T.day ? b : "-", pal('l'));
  fmt_money(m1, sizeof m1, T.lastCosts);
  snprintf(b, sizeof b, "-%s", m1); LINE("Pensje (wczoraj)", T.day ? b : "-", pal('o'));
  fmt_money(m1, sizeof m1, net);
  LINE("Bilans", T.day ? m1 : "-", net >= 0 ? pal('l') : pal('r'));
  snprintf(b, sizeof b, "%d / %d", T.soldiers, max_soldiers()); LINE("Żołnierze", b, pal('w'));
  snprintf(b, sizeof b, "%d (wolnych %d)", T.nmen, men_free()); LINE("Ludzie", b, pal('w'));
  snprintf(b, sizeof b, "%d (%+d/dzień)", T.booze, T.lastBooze); LINE("Alkohol (beczki)", b, pal('y'));
  snprintf(b, sizeof b, "%d / %d", dist_count(0), ND); LINE("Dzielnice", b, pal('z'));
  snprintf(b, sizeof b, "%d", T.nbiz); LINE("Biznesy", b, pal('w'));
  y += 4;
  text_draw_sh(16, y, "Szacunek", pal('s')); bar(110, y + 1, 116, 8, T.respect, 100, pal('z')); y += 13;
  text_draw_sh(16, y, "Gorączka", pal('s')); bar(110, y + 1, 116, 8, T.heat, 100, T.heat > 55 ? pal('r') : pal('o')); y += 13;
  text_draw_sh(16, y, "Śledztwo FBI", T.invest > 60 ? pal('r') : pal('s')); bar(110, y + 1, 116, 8, T.invest, 100, pal('r')); y += 15;
#undef LINE
  /* przycisk końca dnia */
  int bx = 16, by = CY + CH - 26, bw = 210, bh = 18;
  int sel = 1;
  gfx_rect(bx, by, bw, bh, sel ? RGB(0x6a, 0x4a, 0x18) : RGB(0x30, 0x24, 0x14));
  gfx_rect(bx, by + bh - 2, bw, 2, pal('z'));
  text_c(bx + bw / 2, by + 4, "Zakończ dzień  ▶", pal('y'));
  zone(bx, by, bw, bh, Z_NEXT, 0);

  panel(240, CY, 232, CH);
  text_draw_sh(248, CY + 6, "OSTATNIE WYDARZENIA", pal('z'));
  y = CY + 21;
  int from = T.nlog > 16 ? T.nlog - 16 : 0;
  for (int i = from; i < T.nlog; i++) {
    char lines[3][256];
    int n = text_wrap(T.log[i], 216, lines, 3);
    for (int k = 0; k < n && y < CY + CH - 10; k++) { text_draw(248 + (k ? 6 : 0), y, lines[k], T.log[i][0] == 0xE2 ? pal('z') : pal('m')); y += 10; }
  }
  (void)from;
}

static void draw_map(void) {
  panel(8, CY, 288, CH);
  int ox = 0, oy = CY - 40;
  /* jezioro */
  gfx_vgrad(254, CY + 4, 38, CH - 8, RGB(0x1a, 0x30, 0x58), RGB(0x10, 0x20, 0x40));
  for (int i = 0; i < 14; i++) gfx_rect(258 + (i * 13) % 30, CY + 12 + i * 15, 6, 1, RGB(0x40, 0x60, 0x90));
  text_draw(258, CY + CH - 16, "Jez.", RGB(0x60, 0x80, 0xb0));
  for (int d = 0; d < ND; d++) {
    const DistTpl *t = &DT[d];
    int x = t->x + ox, y = t->y + oy, w = t->w, h = t->h;
    int own = T.d[d].owner;
    u32 c = fcol(own);
    gfx_rect(x, y, w, h, blend(RGB(0x18, 0x14, 0x20), c, own < 0 ? 40 : 110));
    /* paski wpływów */
    int bx = x + 3;
    for (int f = 0; f < NF; f++) {
      int bw = (w - 6) * T.d[d].infl[f] / 100;
      if (bw > 0) { gfx_rect(bx, y + h - 6, bw, 3, fcol(f)); bx += bw; }
    }
    int sel = tsel[TAB_MAP] == d;
    u32 bc = sel ? pal('y') : RGB(0x08, 0x06, 0x0c);
    gfx_rect(x, y, w, 1, bc); gfx_rect(x, y + h - 1, w, 1, bc); gfx_rect(x, y, 1, h, bc); gfx_rect(x + w - 1, y, 1, h, bc);
    if (sel) { gfx_rect(x + 1, y + 1, w - 2, 1, bc); gfx_rect(x + 1, y + h - 2, w - 2, 1, bc); }
    text_c(x + w / 2, y + 4, t->name, sel ? pal('y') : pal('w'));
    int nb = biz_in(d);
    char b[16];
    if (nb) { snprintf(b, sizeof b, "$ x%d", nb); text_c(x + w / 2, y + 15, b, pal('l')); }
    if (T.d[d].fort) { snprintf(b, sizeof b, "#%d", T.d[d].fort); text_draw(x + 3, y + h - 17, b, pal('s')); }
    zone(x, y, w, h, Z_ROW, d);
  }
  text_draw(14, CY + CH - 34, "# ochrona", pal('d'));
  text_draw(14, CY + CH - 23, "$ biznesy", pal('d'));
  /* panel dzielnicy */
  int d = tsel[TAB_MAP];
  panel(302, CY, 170, CH);
  int y = CY + 6;
  text_draw_sh(310, y, DT[d].name, pal('z')); y += 13;
  int own = T.d[d].owner;
  text_draw_sh(310, y, own < 0 ? "Teren sporny" : own == 0 ? "Pod naszą kontrolą" : FT[own].name, fcol(own)); y += 14;
  for (int f = 0; f < NF; f++) {
    if (!T.d[d].infl[f]) continue;
    text_draw(310, y, f == 0 ? "My" : FT[f].name, fcol(f));
    bar(400, y + 1, 50, 7, T.d[d].infl[f], 100, fcol(f));
    char b[8]; snprintf(b, sizeof b, "%d", T.d[d].infl[f]); text_r(468, y, b, pal('s'));
    y += 11;
  }
  y += 4;
  char b[64];
  snprintf(b, sizeof b, "Bogactwo: %d/5   Policja: %d/5", DT[d].wealth, DT[d].police); text_draw(310, y, b, pal('s')); y += 11;
  snprintf(b, sizeof b, "Obrona: %d   Ochrona: %d/3", dist_defense(d), T.d[d].fort); text_draw(310, y, b, pal('s')); y += 11;
  snprintf(b, sizeof b, "Biznesy: %d/%d", biz_in(d), DT[d].slots); text_draw(310, y, b, pal('s')); y += 13;
  for (int i = 0; i < T.nbiz && y < CY + CH - 24; i++)
    if (T.biz[i].dist == d) { snprintf(b, sizeof b, "• %s (%d)", BT[T.biz[i].type].name, T.biz[i].lvl); text_draw(312, y, b, pal('l')); y += 10; }
  text_draw(310, CY + CH - 14, "E / klik: działania", pal('y'));
}

static void draw_list_scroll(int t, int visible) {
  int s = tsel[t];
  if (s < tscroll[t]) tscroll[t] = s;
  if (s >= tscroll[t] + visible) tscroll[t] = s - visible + 1;
  if (tscroll[t] < 0) tscroll[t] = 0;
}

static void draw_biz(void) {
  panel(8, CY, 464, CH);
  int y = CY + 5;
  text_draw_sh(16, y, "Lokal", pal('z')); text_draw_sh(150, y, "Dzielnica", pal('z')); text_draw_sh(250, y, "Poz.", pal('z'));
  text_draw_sh(285, y, "Zarządca", pal('z')); text_r(462, y, "Dochód/dzień", pal('z'));
  y += 14;
  int rows = T.nbiz + 1, vis = 15;
  draw_list_scroll(TAB_BIZ, vis);
  for (int r = tscroll[TAB_BIZ]; r < rows && r < tscroll[TAB_BIZ] + vis; r++) {
    draw_row_bg(10, y - 1, 460, 12, tsel[TAB_BIZ] == r, r);
    if (r == 0) text_draw_sh(16, y, "+ Otwórz nowy biznes", pal('y'));
    else {
      int i = r - 1;
      Biz *b = &T.biz[i];
      char s[64];
      text_draw_sh(16, y, BT[b->type].name, b->closed ? pal('d') : pal('w'));
      text_draw(150, y, DT[b->dist].name, pal('s'));
      snprintf(s, sizeof s, "%d", b->lvl); text_draw(258, y, s, pal('w'));
      text_draw(285, y, b->manager >= 0 ? T.men[b->manager].name : "-", pal('s'));
      if (b->closed) snprintf(s, sizeof s, "zamknięty (%d)", b->closed);
      else snprintf(s, sizeof s, "$%d", biz_income(i, T.booze > 0 || !BT[b->type].boozeUse));
      text_r(462, y, s, b->closed ? pal('r') : pal('l'));
    }
    y += 12;
  }
  int tot = 0;
  for (int i = 0; i < T.nbiz; i++) tot += biz_income(i, T.booze > 0);
  char s[64];
  snprintf(s, sizeof s, "Razem ok. $%d dziennie   •   alkohol: %d beczek (%+d/dzień)", tot, T.booze, T.lastBooze);
  text_draw(16, CY + CH - 13, s, pal('m'));
}

static void draw_men(void) {
  panel(8, CY, 312, CH);
  int y = CY + 5;
  text_draw_sh(16, y, "Człowiek", pal('z')); text_draw_sh(156, y, "W", pal('z')); text_draw_sh(172, y, "S", pal('z'));
  text_draw_sh(188, y, "L", pal('z')); text_draw_sh(210, y, "Zajęcie", pal('z'));
  y += 14;
  int rows = T.nmen + 1 + T.nrec, vis = 15;
  draw_list_scroll(TAB_MEN, vis);
  for (int r = tscroll[TAB_MEN]; r < rows && r < tscroll[TAB_MEN] + vis; r++) {
    draw_row_bg(10, y - 1, 308, 12, tsel[TAB_MEN] == r, r);
    char s[64];
    if (r < T.nmen) {
      Man *m = &T.men[r];
      snprintf(s, sizeof s, "%s%s", m->capo ? "★ " : "", m->name);
      text_draw_sh(16, y, s, m->status == MS_OK ? pal('w') : pal('d'));
      snprintf(s, sizeof s, "%d", m->fight); text_draw(156, y, s, pal('o'));
      snprintf(s, sizeof s, "%d", m->brains); text_draw(172, y, s, pal('c'));
      snprintf(s, sizeof s, "%d", m->loyal); text_draw(188, y, s, m->loyal < 30 ? pal('r') : pal('l'));
      man_status(m, s, sizeof s);
      text_draw_n(210, y, s, pal('s'), 20);
    } else if (r == T.nmen) {
      snprintf(s, sizeof s, "+ Werbuj %d żołnierzy ($%d)   [%d/%d]", SOLDIER_PACK, SOLDIER_COST, T.soldiers, max_soldiers());
      text_draw_sh(16, y, s, pal('y'));
    } else {
      Man *m = &T.rec[r - T.nmen - 1];
      snprintf(s, sizeof s, "Rekrut: %s", m->name);
      text_draw_sh(16, y, s, pal('a'));
      snprintf(s, sizeof s, "%d", m->fight); text_draw(156, y, s, pal('o'));
      snprintf(s, sizeof s, "%d", m->brains); text_draw(172, y, s, pal('c'));
      snprintf(s, sizeof s, "%d", m->loyal); text_draw(188, y, s, pal('l'));
      snprintf(s, sizeof s, "zatrudnij $%d", hire_cost(m)); text_draw(210, y, s, pal('y'));
    }
    y += 12;
  }
  text_draw(16, CY + CH - 13, "W = walka, S = spryt, L = lojalność", pal('d'));
  /* szczegóły */
  panel(326, CY, 146, CH);
  int r = tsel[TAB_MEN];
  Man *m = r < T.nmen ? &T.men[r] : r > T.nmen ? &T.rec[r - T.nmen - 1] : NULL;
  if (m) {
    portrait(m->sprite, 334, CY + 8, 44);
    char s[64];
    text_draw_sh(384, CY + 10, m->nick, pal('y'));
    snprintf(s, sizeof s, "Poziom %d", m->lvl); text_draw(384, CY + 22, s, pal('s'));
    if (m->capo) text_draw(384, CY + 33, "CAPO", pal('z'));
    int y2 = CY + 60;
    text_draw_sh(334, y2, m->name, pal('w')); y2 += 14;
    text_draw(334, y2, "Walka", pal('s')); bar(380, y2 + 1, 84, 7, m->fight, 10, pal('o')); y2 += 11;
    text_draw(334, y2, "Spryt", pal('s')); bar(380, y2 + 1, 84, 7, m->brains, 10, pal('c')); y2 += 11;
    text_draw(334, y2, "Lojaln.", pal('s')); bar(380, y2 + 1, 84, 7, m->loyal, 100, m->loyal < 30 ? pal('r') : pal('l')); y2 += 14;
    snprintf(s, sizeof s, "Pensja: $%d/dzień", m->wage); text_draw(334, y2, s, pal('s')); y2 += 11;
    if (r < T.nmen) { snprintf(s, sizeof s, "Akcje: %d", m->ops); text_draw(334, y2, s, pal('s')); y2 += 11; }
    else { snprintf(s, sizeof s, "Koszt: $%d", hire_cost(m)); text_draw(334, y2, s, pal('y')); y2 += 11; }
  } else {
    char s[64];
    text_draw_sh(334, CY + 10, "Żołnierze", pal('y'));
    snprintf(s, sizeof s, "Mamy: %d", T.soldiers); text_draw(334, CY + 26, s, pal('w'));
    snprintf(s, sizeof s, "Limit: %d", max_soldiers()); text_draw(334, CY + 37, s, pal('s'));
    text_draw(334, CY + 52, "Pensja: $6/dzień", pal('s'));
    char lines[8][256];
    int n = text_wrap("Żołnierze bronią dzielnic i wspierają akcje. Limit rośnie z liczbą dzielnic i szacunkiem.", 130, lines, 8);
    for (int i = 0; i < n; i++) text_draw(334, CY + 70 + i * 10, lines[i], pal('m'));
  }
}

static void draw_ops(void) {
  panel(8, CY, 300, CH);
  int y = CY + 5;
  text_draw_sh(16, y, "Akcja", pal('z')); text_r(300, y, "Czas", pal('z'));
  y += 14;
  for (int r = 0; r < OPT_N; r++) {
    draw_row_bg(10, y - 1, 296, 12, tsel[TAB_OPS] == r, r);
    text_draw_sh(16, y, OT[r].name, pal('w'));
    if (op_map(r) >= 0) text_draw(140, y, "3D", pal('a'));
    char s[16]; snprintf(s, sizeof s, "%d d.", OT[r].days); text_r(300, y, s, pal('s'));
    y += 12;
  }
  y += 6;
  char lines[4][256];
  int n = text_wrap(OT[tsel[TAB_OPS]].desc, 280, lines, 4);
  for (int i = 0; i < n; i++) { text_draw(16, y, lines[i], pal('m')); y += 10; }
  text_draw(16, CY + CH - 25, "3D = możesz poprowadzić akcję osobiście", pal('a'));
  char s[64]; snprintf(s, sizeof s, "Wolnych ludzi: %d, żołnierzy: %d", men_free(), T.soldiers);
  text_draw(16, CY + CH - 13, s, pal('s'));

  panel(314, CY, 158, CH);
  text_draw_sh(322, CY + 6, "W TOKU", pal('z'));
  y = CY + 21;
  int any = 0;
  for (int o = 0; o < MAXOPS; o++) {
    Op *op = &T.ops[o];
    if (!op->active) continue;
    any = 1;
    char tn[48] = "";
    if (OT[op->type].tgt == TG_DIST) snprintf(tn, sizeof tn, "%s", DT[op->target].name);
    else if (OT[op->type].tgt == TG_FAM) snprintf(tn, sizeof tn, "%s", FT[op->target].name);
    else if (OT[op->type].tgt == TG_HEIST) snprintf(tn, sizeof tn, "%s", HEIST[op->target]);
    text_draw_sh(322, y, OT[op->type].name, pal('w')); y += 10;
    if (tn[0]) { text_draw(328, y, tn, pal('s')); y += 10; }
    snprintf(s, sizeof s, "%d ludzi, %d żołn., %d d.", op->nteam, op->soldiers, op->days);
    text_draw(328, y, s, pal('d')); y += 13;
    if (y > CY + CH - 30) break;
  }
  if (!any) text_draw(322, y, "Brak akcji.", pal('d'));
}

static void draw_fam(void) {
  int y = CY;
  for (int r = 0; r < NF - 1; r++) {
    int f = r + 1;
    Fam *F = &T.f[f];
    int h = 52;
    panel(8, y, 464, h);
    zone(8, y, 464, h, Z_ROW, r);
    if (tsel[TAB_FAM] == r) { gfx_rect(8, y, 3, h, pal('y')); gfx_rect_a(11, y + 1, 461, h - 2, pal('b'), 50); }
    portrait(FT[f].sprite, 16, y + 5, 42);
    if (!fam_alive(f)) gfx_rect_a(16, y + 5, 42, 42, RGB(0, 0, 0), 150);
    text_draw_sh(66, y + 5, FT[f].name, pal(FT[f].col));
    char s[80];
    snprintf(s, sizeof s, "%s%s", FT[f].boss, F->bossDead ? " (nie żyje)" : "");
    text_draw(66, y + 17, s, pal('s'));
    u32 sc = F->state == F_WAR ? pal('r') : F->state == F_ALLY || F->state == F_VASSAL ? pal('l') : F->state >= F_ABSORBED ? pal('d') : pal('w');
    snprintf(s, sizeof s, "%s%s", FSTATE[F->state], F->state == F_TRUCE ? "" : "");
    if (F->state == F_TRUCE) snprintf(s, sizeof s, "rozejm (%d dni)", F->stateT);
    text_draw_sh(66, y + 31, s, sc);
    if (fam_alive(f)) {
      text_draw(220, y + 6, "Stosunki", pal('s'));
      gfx_rect(290, y + 7, 120, 7, RGB(0x10, 0x0c, 0x14));
      int c = 290 + 60, v = F->rel * 60 / 100;
      if (v >= 0) gfx_rect(c, y + 8, v, 5, pal('l')); else gfx_rect(c + v, y + 8, -v, 5, pal('r'));
      gfx_rect(c, y + 6, 1, 9, pal('w'));
      snprintf(s, sizeof s, "%+d", F->rel); text_r(462, y + 6, s, pal('w'));
      snprintf(s, sizeof s, "Żołnierze: %d    Dzielnice: %d    Siła: %d (my: %d)", F->soldiers, dist_count(f), fam_power(f), player_power());
      text_draw(220, y + 20, s, pal('s'));
      if (F->state == F_VASSAL) { snprintf(s, sizeof s, "Płacą trybut od %d dni", F->vassalDays); text_draw(220, y + 32, s, pal('l')); }
      else if (F->leaderless) { snprintf(s, sizeof s, "Bez szefa jeszcze %d dni", F->leaderless); text_draw(220, y + 32, s, pal('o')); }
    }
    y += h + 2;
  }
}

static void draw_log(void) {
  panel(8, CY, 464, CH);
  int vis = 19;
  int end = T.nlog - tscroll[TAB_LOG];
  int start = end - vis < 0 ? 0 : end - vis;
  int y = CY + 6;
  for (int i = start; i < end; i++) { text_draw(16, y, T.log[i], (unsigned char)T.log[i][0] == 0xE2 ? pal('z') : pal('m')); y += 11; }
  text_r(466, CY + CH - 13, "↑↓ przewijanie", pal('d'));
}

/* gazeta: kremowy papier, winieta i nagłówki szeryfowe */
static void strip_codes(const char *in, char *out, int n) {
  int o = 0;
  for (const char *p = in; *p && o < n - 1; p++) {
    if (*p == '{' && p[1] && p[2] == '}') { p += 2; continue; }
    out[o++] = *p;
  }
  out[o] = 0;
}
static void ctext(int font, float size, float cx, float y, const char *s, u32 col) {
  d2_text(font, size, cx - d2_text_w(font, size, s) * 0.5f, y, s, col);
}
static void draw_news(Modal *m) {
  float W = 780, Hh = 560, x = (UI_W - W) / 2, y = (UI_H - Hh) / 2;
  u32 ink = 0xFF1C1814, ink2 = 0xFF3A342C;
  d2_rect(0, 0, UI_W, UI_H, 0x9A000000);
  d2_shadow(x, y, W, Hh, 4, 22, 0xC0000000);
  d2_grad(x, y, W, Hh, 0xFFEDE5CF, 0xFFDCD0B2);
  d2_rrect_line(x + 10, y + 10, W - 20, Hh - 20, 2, 1.0f, 0x60302820);
  float cx = x + W / 2;
  ctext(FONT_SERIF, 50, cx, y + 32, "CHICAGO DAILY HERALD", ink);
  d2_rect(x + 30, y + 98, W - 60, 3, ink);
  d2_rect(x + 30, y + 104, W - 60, 1, ink);
  char b[256];
  strip_codes(m->text[0], b, sizeof b);
  ctext(FONT_SANS, 17, cx, y + 110, b, ink2);
  d2_rect(x + 30, y + 134, W - 60, 1, ink);
  float yy = y + 150;
  for (int i = 1; i + 1 < m->ntext + 1 && i < m->ntext; i += 2) {
    strip_codes(m->text[i], b, sizeof b);
    char hl[4][256];
    int n = d2_wrap(FONT_SERIF, i == 1 ? 38 : 28, b, W - 80, hl, 3);
    for (int k = 0; k < n; k++) { ctext(FONT_SERIF, i == 1 ? 38 : 28, cx, yy, hl[k], ink); yy += i == 1 ? 44 : 34; }
    if (i + 1 < m->ntext) {
      strip_codes(m->text[i + 1], b, sizeof b);
      n = d2_wrap(FONT_SANS, 20, b, W - 120, hl, 3);
      for (int k = 0; k < n; k++) { ctext(FONT_SANS, 20, cx, yy + 4, hl[k], ink2); yy += 26; }
    }
    yy += 14;
    if (i + 2 < m->ntext) { d2_rect(x + 200, yy - 4, W - 400, 1, 0x80302820); yy += 12; }
  }
  /* szpalty tekstu (faktura) */
  float colTop = yy + 4, colBot = y + Hh - 96;
  for (int c = 0; c < 3; c++) {
    float cx0 = x + 40 + c * ((W - 80) / 3), cw = (W - 80) / 3 - 18;
    for (float ly = colTop; ly < colBot; ly += 9) {
      unsigned hsh = (unsigned)(ly * 7 + c * 131 + T.day * 17);
      float lw = cw * (0.72f + (hsh % 28) / 100.0f);
      d2_rect(cx0, ly, lw, 3, 0x38302820);
    }
  }
  /* przycisk */
  float bw = 260, bh = 44, bx = cx - bw / 2, by = y + Hh - 72;
  d2_rrect(bx, by, bw, bh, 6, m->cur == 0 ? 0xFF2A221A : 0xFF4A3E30);
  ctext(FONT_BOLD, 21, cx, by + 10, m->opt[0], 0xFFF0E6D0);
  zone((int)(bx / KS), (int)(by / KS), (int)(bw / KS), (int)(bh / KS), Z_MOPT, 0);
}

static void draw_modal(Modal *m) {
  if (m->kind == MK_EVENT && T.nev && T.ev[0].id == EV_NEWS) { draw_news(m); return; }
  int w = 330;
  int lines = m->ntext;
  int vis = m->nopt > 12 ? 12 : m->nopt;
  int h = 30 + lines * 11 + (lines ? 6 : 0) + vis * 12 + 30;
  int x = (TSCREEN_W - w) / 2, y = (TSCREEN_H - h) / 2;
  if (y < 4) y = 4;
  gfx_rect_a(0, 0, TSCREEN_W, TSCREEN_H, RGB(0, 0, 0), 110);
  gfx_window(x, y, w, h);
  text_draw_sh(x + 10, y + 7, m->title, pal('z'));
  gfx_rect(x + 8, y + 19, w - 16, 1, RGB(0x6a, 0x52, 0x2a));
  int yy = y + 25;
  for (int i = 0; i < lines; i++) { text_draw(x + 10, yy, m->text[i], pal('m')); yy += 11; }
  if (lines) yy += 6;
  if (m->cur < m->scroll) m->scroll = m->cur;
  if (m->cur >= m->scroll + vis) m->scroll = m->cur - vis + 1;
  for (int i = m->scroll; i < m->nopt && i < m->scroll + vis; i++) {
    int sel = i == m->cur;
    if (sel) gfx_rect_a(x + 5, yy - 1, w - 10, 12, pal('b'), 90);
    if (sel) text_draw(x + 8, yy, "▶", pal('y'));
    u32 oc = !m->en[i] ? pal('d') : sel ? pal('y') : pal('w');
    char *tabc = strchr(m->opt[i], '\t');
    if (tabc) {
      *tabc = 0;
      text_draw_sh(x + 18, yy, m->opt[i], oc);
      text_r(x + w - 14, yy, tabc + 1, oc);
      *tabc = '\t';
    } else text_draw_sh(x + 18, yy, m->opt[i], oc);
    zone(x + 5, yy - 1, w - 10, 12, Z_MOPT, i);
    if (m->kind == MK_TEAM && m->val[i] == 50) { zone(x + 5, yy - 1, 90, 12, Z_ROW, -1); zone(x + 95, yy - 1, 60, 12, Z_ROW, 1); }
    yy += 12;
  }
  if (m->nopt > vis) { char b[24]; snprintf(b, sizeof b, "%d/%d", m->cur + 1, m->nopt); text_r(x + w - 10, y + 7, b, pal('d')); }
  /* opis */
  if (m->desc[m->cur][0]) {
    char dl[3][256];
    int n = text_wrap(m->desc[m->cur], w - 20, dl, 2);
    gfx_rect(x + 8, y + h - 27, w - 16, 1, RGB(0x3a, 0x2c, 0x1a));
    for (int i = 0; i < n; i++) text_draw(x + 10, y + h - 24 + i * 10, dl[i], pal('s'));
  }
}

void tycoon_draw(void) {
  nz = 0;
  /* tło: skórzana okładka */
  d2_grad(0, 0, UI_W, UI_H, 0xFF2A1C16, 0xFF0E0A0A);
  d2_grad(0, 0, UI_W, 120, 0x50000000, 0x00000000);
  /* nagłówek */
  d2_grad(0, 0, UI_W, 50, 0xFF100C0E, 0xFF1A1418);
  d2_rect(0, 50, UI_W, 2, UI_GOLD);
  d2_text_sh(FONT_SERIF, 30, 22, 8, "Księga Rodziny", UI_GOLD);
  char ds[40], b[64], mm[32];
  date_str(T.day, ds, sizeof ds);
  snprintf(b, sizeof b, "Dzień %d  \xe2\x80\xa2  %s", T.day + 1, ds);
  d2_text_c(FONT_SANS, 20, UI_W / 2 + 20, 13, b, UI_CREAM);
  fmt_money(mm, sizeof mm, H.gold);
  d2_text_r(FONT_BOLD, 24, UI_W - 22, 11, mm, H.gold < 0 ? UI_RED : 0xFF9AE08A);
  /* zakładki */
  int tw = 66, tx = 9;
  for (int i = 0; i < TAB_N; i++) {
    int x = tx + i * tw;
    int sel = i == tab;
    float X = x * KS, Y = 22 * KS, Wd = (tw - 3) * KS, Hd = 13 * KS;
    if (sel) { d2_rrect(X, Y, Wd, Hd, 8, 0xFF4A3618); d2_rrect_line(X, Y, Wd, Hd, 8, 1.5f, UI_GOLD); }
    else d2_rrect(X, Y, Wd, Hd, 8, 0xFF1C1618);
    d2_text_c(sel ? FONT_BOLD : FONT_SANS, 18, X + Wd / 2, Y + 7, TABN[i], sel ? 0xFFFFE6A8 : UI_GREY);
    zone(x, 22, tw - 2, 13, Z_TAB, i);
  }
  switch (tab) {
    case TAB_OVER: draw_over(); break;
    case TAB_MAP: draw_map(); break;
    case TAB_BIZ: draw_biz(); break;
    case TAB_MEN: draw_men(); break;
    case TAB_OPS: draw_ops(); break;
    case TAB_FAM: draw_fam(); break;
    case TAB_LOG: draw_log(); break;
  }
  /* stopka */
  gfx_rect(0, 255, TSCREEN_W, 15, RGB(0x0c, 0x08, 0x0a));
  gfx_rect(0, 255, TSCREEN_W, 1, RGB(0x6a, 0x52, 0x2a));
  text_draw(8, 258, "←→ zakładki  ↑↓ wybór  E wybierz", pal('d'));
  text_draw_sh(206, 258, "N: koniec dnia", pal('y'));
  zone(204, 256, 80, 14, Z_NEXT, 0);
  text_r(TSCREEN_W - 8, 258, "Esc/Tab: do miasta", pal('s'));
  zone(TSCREEN_W - 110, 256, 110, 14, Z_BACK, 0);
  if (md) draw_modal(&MD[md - 1]);
}

/* ================================================================ podróż (nieużywany osobny tryb — mapa w księdze) */
void tycoon_travel_open(void) { tab = TAB_MAP; tycoon_open(); }
void tycoon_travel_update(void) { tycoon_update(); }
void tycoon_travel_draw(void) { tycoon_draw(); }

/* ================================================================ symulacja (test balansu) */
static void sim_choose_events(void) {
  while (T.nev) {
    Ev e = T.ev[0];
    memmove(T.ev, T.ev + 1, sizeof(Ev) * (T.nev - 1));
    T.nev--;
    EvView v;
    ev_view(&e, &v);
    int c = 0;
    if (e.id == EV_TRIBUTEDEMAND || e.id == EV_COPDEAL) c = H.gold > e.b * 3 ? 0 : 1;
    else if (e.id == EV_RAT) c = 0;
    else if (e.id == EV_VASSALOFFER || e.id == EV_ALLYOFFER || e.id == EV_TRUCEOFFER) c = 0;
    else c = rnd(v.nopt > 0 ? v.nopt : 1);
    if (e.id != EV_STORY) ev_apply(&e, c);
  }
}

static void sim_player_day(void) {
  /* budowa */
  for (int tries = 0; tries < 3; tries++) {
    int bestD = -1, bestT = -1, bv = 0;
    for (int d = 0; d < ND; d++)
      for (int k = 0; k < B_N; k++) {
        if (can_build(d, k) != R_OK) continue;
        if ((k == B_MELINA || k == B_KLUB) && T.lastBooze < 2 && T.booze < 40) continue;
        int v = BT[k].income * (6 + DT[d].wealth * 2) / 10 - BT[k].cost / 30 + BT[k].boozeMake * 20 * (T.lastBooze < 3) - BT[k].heat * T.heat * 2;
        if (H.gold - BT[k].cost < 600) continue;
        if (v > bv) { bv = v; bestD = d; bestT = k; }
      }
    if (bestD >= 0) t_build(bestD, bestT); else break;
  }
  for (int i = 0; i < T.nbiz; i++) if (H.gold > upgrade_cost(i) + 3000) t_upgrade(i);
  /* zarządcy i strażnicy */
  for (int i = 0; i < T.nbiz; i++) if (T.biz[i].manager < 0)
    for (int m = 0; m < T.nmen; m++) if (T.men[m].status == MS_OK && T.men[m].assign < 0 && T.men[m].brains >= 5) { t_manager(m, i); break; }
  /* ludzie */
  if (H.gold > 2500 && T.nrec && T.nmen < MAXMEN) t_hire(0);
  if (H.gold > 1500 && T.soldiers < max_soldiers() - SOLDIER_PACK) t_soldiers();
  /* akcje */
  int team[4];
  for (int m = 0; m < T.nmen && ops_active() < (T.heat > 50 ? 1 : 3); m++) {
    if (T.men[m].status != MS_OK || T.men[m].assign >= 0) continue;
    team[0] = m;
    if (T.heat > 45 && H.gold > 900) { t_launch(OP_LAPOWKA, 0, team, 1, 0); continue; }
    if (T.heat > 50) continue;
    if (T.booze < 20) { t_launch(OP_PRZEMYT, 0, team, 1, 2); continue; }
    int bd = -1, bc = 0;
    for (int d = 0; d < ND; d++) {
      int type = T.d[d].infl[0] >= 25 && strongest_rival_in(d) > 0 && player_power() > 50 ? OP_ATAK : OP_HARACZ;
      if (!op_target_valid(type, d)) continue;
      int c = op_chance(type, d, team, 1, T.soldiers / 4);
      if (c > bc) { bc = c; bd = d * 10 + type; }
    }
    if (bd >= 0 && bc > 40) t_launch(bd % 10, bd / 10, team, 1, T.soldiers / 4);
  }
  /* dyplomacja */
  for (int f = 1; f < NF; f++) {
    if (!fam_alive(f)) continue;
    if (dp_available(f, DP_MERGE)) { t_diplo(f, DP_MERGE); continue; }
    if (dp_available(f, DP_TRUCE) && T.f[f].soldiers > T.soldiers) t_diplo(f, DP_TRUCE);
    if (dp_available(f, DP_ALLY)) t_diplo(f, DP_ALLY);
    else if (dp_available(f, DP_TRIBUTE) && T.day % 5 == 0) t_diplo(f, DP_TRIBUTE);
    else if (H.gold > 6000 && T.f[f].state != F_WAR && T.f[f].rel < 60 && T.day % 3 == 0) t_diplo(f, DP_GIFT);
  }
}

int tycoon_sim_days(int days, int verbose) {
  g_autoplay = 2;
  H.gold = 0;
  tycoon_start(60);
  for (int d = 0; d < days && !T.ended; d++) {
    sim_player_day();
    tycoon_end_day();
    sim_choose_events();
    if (verbose && (d % 10 == 9 || T.ended)) {
      printf("dzień %3d: $%6d inc=%5d koszt=%4d heat=%3d fbi=%3d szac=%3d żoł=%3d ludzi=%2d biz=%2d dzielnice=%d alk=%3d |",
             T.day, H.gold, T.lastIncome, T.lastCosts, T.heat, T.invest, T.respect, T.soldiers, T.nmen, T.nbiz, dist_count(0), T.booze);
      for (int f = 1; f < NF; f++) printf(" %s:%s/%d/%+d", FT[f].id, FSTATE[T.f[f].state], T.f[f].soldiers, T.f[f].rel);
      printf("\n");
    }
  }
  if (verbose) {
    printf("koniec: %s (dzień %d)\n", T.ended ? (T.invest >= 100 ? "WYROK" : T.debtDays >= 10 ? "ROZPAD" : "WYGRANA") : "trwa", T.day);
    for (int i = T.nlog > 12 ? T.nlog - 12 : 0; i < T.nlog; i++) printf("  %s\n", T.log[i]);
  }
  return 0;
}
