/* Parser pliku fabuły (story.txt) i walidacja.
 * Format: sekcje "@typ id", w skryptach jedna komenda na linię,
 * linia zaczynająca się od cudzysłowu = kwestia aktualnego mówcy, ":etykieta" = etykieta. */
#include "engine.h"

Story S;

static int curLine;
static char errbuf[256];

static void err(const char *fmt, const char *a) {
  snprintf(errbuf, sizeof errbuf, fmt, a ? a : "");
  fprintf(stderr, "story.txt:%d: %s\n", curLine, errbuf);
  S.errors++;
}
static void warn(const char *fmt, const char *a) {
  char b[256];
  snprintf(b, sizeof b, fmt, a ? a : "");
  fprintf(stderr, "story.txt:%d: uwaga: %s\n", curLine, b);
}

/* ---------------------------------------------------------------- wyszukiwanie */
int map_find(const char *id) { for (int i = 0; i < S.nmaps; i++) if (!strcmp(S.maps[i].id, id)) return i; return -1; }
int item_find(const char *id) { for (int i = 0; i < S.nitems; i++) if (!strcmp(S.items[i].id, id)) return i; return -1; }
int enemy_find(const char *id) { for (int i = 0; i < S.nenemies; i++) if (!strcmp(S.enemies[i].id, id)) return i; return -1; }
int script_find(const char *n) { for (int i = 0; i < S.nscripts; i++) if (!strcmp(S.scripts[i].name, n)) return i; return -1; }
int speaker_find(const char *id) { for (int i = 0; i < S.nspeakers; i++) if (!strcmp(S.speakers[i].id, id)) return i; return -1; }
int var_find(const char *name, int create) {
  for (int i = 0; i < S.nvars; i++) if (!strcmp(S.varnames[i], name)) return i;
  if (!create || S.nvars >= MAX_VARS) return -1;
  snprintf(S.varnames[S.nvars], 32, "%s", name);
  return S.nvars++;
}

static int dir_parse(const char *s) {
  if (!s) return DIR_DOWN;
  if (!strcmp(s, "up") || !strcmp(s, "gora")) return DIR_UP;
  if (!strcmp(s, "left") || !strcmp(s, "lewo")) return DIR_LEFT;
  if (!strcmp(s, "right") || !strcmp(s, "prawo")) return DIR_RIGHT;
  return DIR_DOWN;
}

/* ---------------------------------------------------------------- tokenizer */
#define MAXTOK 24
static char *tok[MAXTOK];
static int ntok;

static void unescape(char *s) {
  char *w = s;
  for (char *r = s; *r; r++) {
    if (*r == '\\' && r[1]) {
      r++;
      *w++ = *r == 'n' ? '\n' : *r;
    } else *w++ = *r;
  }
  *w = 0;
}

static void tokenize(char *line) {
  ntok = 0;
  char *p = line;
  while (*p && ntok < MAXTOK) {
    while (*p == ' ' || *p == '\t') p++;
    if (!*p || *p == '#') break;
    if (*p == '"') {
      char *start = ++p;
      while (*p && !(*p == '"' && p[-1] != '\\')) p++;
      if (*p) *p++ = 0;
      unescape(start);
      tok[ntok++] = start;
    } else {
      tok[ntok++] = p;
      while (*p && *p != ' ' && *p != '\t') p++;
      if (*p) *p++ = 0;
    }
  }
}

static int ival(int i, int def) { return i < ntok ? atoi(tok[i]) : def; }
static const char *sval(int i) { return i < ntok ? tok[i] : NULL; }

/* ---------------------------------------------------------------- etykiety */
typedef struct { char name[40]; int cmd; } Label;
typedef struct { char name[40]; int *target; int line; } Fixup;
static Label labels[256]; static int nlabels;
static Fixup fixups[512]; static int nfixups;
typedef struct { int cmd; char name[40]; int line; } CallFix;
static CallFix callfix[256]; static int ncallfix;

static void need_label(const char *name, int *target) {
  if (!name) { err("brak nazwy etykiety", NULL); return; }
  if (nfixups >= 512) return;
  snprintf(fixups[nfixups].name, 40, "%s", name);
  fixups[nfixups].target = target;
  fixups[nfixups].line = curLine;
  nfixups++;
}

static void resolve_labels(void) {
  for (int i = 0; i < nfixups; i++) {
    int found = -1;
    for (int k = 0; k < nlabels; k++) if (!strcmp(labels[k].name, fixups[i].name)) found = labels[k].cmd;
    if (found < 0) { int l = curLine; curLine = fixups[i].line; err("nieznana etykieta '%s'", fixups[i].name); curLine = l; }
    *fixups[i].target = found < 0 ? 0 : found;
  }
  nlabels = 0; nfixups = 0;
}

/* ---------------------------------------------------------------- sekcje */
enum { SEC_NONE, SEC_GAME, SEC_ITEM, SEC_ENEMY, SEC_MAP, SEC_TILES, SEC_SCRIPT, SEC_MENU };
static int sec;
static int curScript = -1;
static MapDef *curMap;
static int curMapRows;
static ItemDef *curItem;
static EnemyDef *curEnemy;
static int menuCmd = -1;

static Cmd *emit(int op) {
  if (S.ncmds >= MAX_CMDS) { err("za dużo komend", NULL); return &S.cmds[MAX_CMDS - 1]; }
  Cmd *c = &S.cmds[S.ncmds++];
  memset(c, 0, sizeof *c);
  c->op = op;
  c->line = curLine;
  return c;
}

static void end_script(void) {
  if (curScript >= 0) {
    emit(OP_END);
    resolve_labels();
    S.scripts[curScript].len = S.ncmds - S.scripts[curScript].start;
  }
  curScript = -1;
}

static void close_section(void) {
  if (sec == SEC_MENU) { err("brak 'endmenu'", NULL); sec = SEC_SCRIPT; }
  if (sec == SEC_SCRIPT) end_script();
  if (sec == SEC_TILES) err("brak 'endtiles'", NULL);
  sec = SEC_NONE;
}

static void parse_cond_ent(EntDef *e, int from) {
  e->condVar[0] = e->condVar[1] = -1;
  int k = 0;
  for (int i = from; i < ntok && k < 2; i++) {
    if (!strcmp(tok[i], "if") || !strcmp(tok[i], "ifnot")) {
      if (i + 1 >= ntok) { err("brak zmiennej po '%s'", tok[i]); break; }
      e->condNeg[k] = !strcmp(tok[i], "ifnot");
      e->condVar[k] = var_find(tok[i + 1], 1);
      S.varReads[e->condVar[k]]++;
      k++;
      i++;
    } else if (!strcmp(tok[i], "wander")) e->wander = 1;
    else if (!strcmp(tok[i], "once")) e->once = 1;
  }
}

/* warunek opcji menu: zwraca liczbę zjedzonych tokenów */
static int parse_cond(int i, int *type, int *a, int *b) {
  *type = CND_NONE; *a = *b = 0;
  if (i >= ntok) return 0;
  const char *t = tok[i];
  if (!strcmp(t, "if") || !strcmp(t, "ifnot")) {
    *type = !strcmp(t, "if") ? CND_IF : CND_IFNOT;
    *a = var_find(sval(i + 1) ? sval(i + 1) : "?", 1); S.varReads[*a]++;
    return 2;
  }
  if (!strcmp(t, "ifge") || !strcmp(t, "iflt")) {
    *type = !strcmp(t, "ifge") ? CND_GE : CND_LT;
    *a = var_find(sval(i + 1) ? sval(i + 1) : "?", 1); S.varReads[*a]++;
    *b = ival(i + 2, 0);
    return 3;
  }
  if (!strcmp(t, "ifgold")) { *type = CND_GOLD; *a = ival(i + 1, 0); return 2; }
  if (!strcmp(t, "ifhas")) {
    *type = CND_HAS;
    *a = item_find(sval(i + 1) ? sval(i + 1) : "");
    if (*a < 0) err("nieznany przedmiot '%s'", sval(i + 1));
    *b = 1;
    return 2;
  }
  return 0;
}

static void parse_script_line(char *line) {
  if (line[0] == ':') { /* etykieta */
    char *n = line + 1;
    while (*n == ' ') n++;
    char *e = n;
    while (*e && *e != ' ' && *e != '\r') e++;
    *e = 0;
    if (nlabels < 256) { snprintf(labels[nlabels].name, 40, "%s", n); labels[nlabels].cmd = S.ncmds; nlabels++; }
    return;
  }
  int quoted = line[0] == '"';
  tokenize(line);
  if (!ntok) return;
  const char *c = tok[0];

  if (sec == SEC_MENU) {
    if (!strcmp(c, "opt")) {
      if (S.nopts >= MAX_OPTS) { err("za dużo opcji", NULL); return; }
      MenuOpt *o = &S.opts[S.nopts++];
      o->text = sval(1) ? sval(1) : "?";
      need_label(sval(2), &o->target);
      parse_cond(3, &o->condType, &o->condA, &o->condB);
      S.cmds[menuCmd].i[1]++;
    } else if (!strcmp(c, "endmenu")) {
      if (S.cmds[menuCmd].i[1] == 0) err("puste menu", NULL);
      sec = SEC_SCRIPT;
    } else err("w menu dozwolone tylko 'opt' i 'endmenu', jest '%s'", c);
    return;
  }

  if (quoted) { /* kwestia aktualnego mówcy */
    Cmd *k = emit(OP_SAY);
    k->s[0] = tok[0];
    k->i[0] = -2;
    return;
  }

  if (!strcmp(c, "say")) {
    Cmd *k = emit(OP_SAY);
    int sp = speaker_find(sval(1) ? sval(1) : "");
    if (sp < 0 && strcmp(sval(1) ? sval(1) : "", "-")) err("nieznany mówca '%s'", sval(1));
    k->i[0] = sp;
    k->s[0] = sval(2) ? sval(2) : "";
  } else if (!strcmp(c, "as")) {
    Cmd *k = emit(OP_AS);
    k->i[0] = -1;
    if (sval(1) && strcmp(sval(1), "-")) {
      k->i[0] = speaker_find(sval(1));
      if (k->i[0] < 0) err("nieznany mówca '%s'", sval(1));
    }
  } else if (!strcmp(c, "menu")) {
    Cmd *k = emit(OP_MENU);
    k->i[0] = S.nopts;
    k->i[1] = 0;
    menuCmd = (int)(k - S.cmds);
    sec = SEC_MENU;
  } else if (!strcmp(c, "goto")) {
    Cmd *k = emit(OP_GOTO);
    need_label(sval(1), &k->i[0]);
  } else if (!strcmp(c, "if") || !strcmp(c, "ifnot") || !strcmp(c, "ifge") || !strcmp(c, "iflt") ||
             !strcmp(c, "ifgold") || !strcmp(c, "ifhas")) {
    Cmd *k = emit(OP_IF);
    int used = parse_cond(0, &k->i[0], &k->i[1], &k->i[2]);
    need_label(sval(used), &k->i[3]);
  } else if (!strcmp(c, "set") || !strcmp(c, "clear")) {
    Cmd *k = emit(OP_SET);
    if (!sval(1)) { err("brak zmiennej", NULL); return; }
    k->i[0] = var_find(sval(1), 1);
    S.varWrites[k->i[0]]++;
    k->i[1] = !strcmp(c, "clear") ? 0 : ival(2, 1);
  } else if (!strcmp(c, "add")) {
    Cmd *k = emit(OP_ADD);
    if (!sval(1)) { err("brak zmiennej", NULL); return; }
    k->i[0] = var_find(sval(1), 1);
    S.varWrites[k->i[0]]++;
    k->i[1] = ival(2, 1);
  } else if (!strcmp(c, "give") || !strcmp(c, "take") || !strcmp(c, "equip")) {
    Cmd *k = emit(!strcmp(c, "give") ? OP_GIVE : !strcmp(c, "take") ? OP_TAKE : OP_EQUIP);
    k->i[0] = item_find(sval(1) ? sval(1) : "");
    if (k->i[0] < 0) err("nieznany przedmiot '%s'", sval(1));
    k->i[1] = ival(2, 1);
  } else if (!strcmp(c, "cash")) {
    emit(OP_CASH)->i[0] = ival(1, 0);
  } else if (!strcmp(c, "heal")) {
    emit(OP_HEAL);
  } else if (!strcmp(c, "xp")) {
    emit(OP_XP)->i[0] = ival(1, 10);
  } else if (!strcmp(c, "hurt")) {
    emit(OP_HURT)->i[0] = ival(1, 1);
  } else if (!strcmp(c, "spawn")) {
    /* spawn WRÓG X Y [ally] */
    Cmd *k = emit(OP_SPAWN);
    k->s[0] = sval(1) ? sval(1) : "";
    if (enemy_find(k->s[0]) < 0) err("nieznany wróg '%s'", k->s[0]);
    k->i[0] = ival(2, 0); k->i[1] = ival(3, 0);
    k->i[2] = sval(4) && !strcmp(sval(4), "ally");
  } else if (!strcmp(c, "waitkill")) {
    emit(OP_WAITKILL);
  } else if (!strcmp(c, "killall")) {
    emit(OP_KILLALL);
  } else if (!strcmp(c, "weapon")) {
    Cmd *k = emit(OP_WEAPON);
    const char *w = sval(1) ? sval(1) : "";
    k->i[0] = !strcmp(w, "fists") ? WPN_FISTS : !strcmp(w, "pistol") ? WPN_PISTOL : !strcmp(w, "shotgun") ? WPN_SHOTGUN : !strcmp(w, "tommy") ? WPN_TOMMY : -1;
    if (k->i[0] < 0) err("nieznana broń '%s'", w);
    k->i[1] = ival(2, 0);
  } else if (!strcmp(c, "objective")) {
    emit(OP_OBJECTIVE)->s[0] = sval(1) ? sval(1) : "";
  } else if (!strcmp(c, "tycoon")) {
    Cmd *k = emit(OP_TYCOON);
    k->s[0] = sval(1) ? sval(1) : "start";
    k->s[1] = sval(2);
    k->i[0] = ival(3, 0);
  } else if (!strcmp(c, "armor")) {
    Cmd *k = emit(OP_EQUIP);
    k->i[0] = -1; k->i[1] = ival(1, 50);
  } else if (!strcmp(c, "battle")) {
    err("komenda 'battle' nie istnieje w wersji 3D (użyj spawn + waitkill)", NULL);
  } else if (!strcmp(c, "warp")) {
    Cmd *k = emit(OP_WARP);
    k->s[0] = sval(1) ? sval(1) : "";
    k->i[1] = ival(2, 0); k->i[2] = ival(3, 0); k->i[3] = dir_parse(sval(4));
    k->i[0] = -1; /* rozwiązywane po wczytaniu wszystkich map */
  } else if (!strcmp(c, "fade")) {
    emit(OP_FADE)->i[0] = sval(1) && !strcmp(sval(1), "out");
  } else if (!strcmp(c, "wait")) {
    emit(OP_WAIT)->i[0] = ival(1, 30);
  } else if (!strcmp(c, "sfx")) {
    Cmd *k = emit(OP_SFX);
    k->i[0] = sfx_find(sval(1) ? sval(1) : "");
    if (k->i[0] < 0) err("nieznany dźwięk '%s'", sval(1));
  } else if (!strcmp(c, "music")) {
    Cmd *k = emit(OP_MUSIC);
    k->s[0] = sval(1) ? sval(1) : "";
    if (strcmp(k->s[0], "-") && !music_exists(k->s[0])) err("nieznana muzyka '%s'", k->s[0]);
  } else if (!strcmp(c, "shake")) {
    emit(OP_SHAKE)->i[0] = ival(1, 6);
  } else if (!strcmp(c, "quest")) {
    Cmd *k = emit(OP_QUEST);
    k->s[0] = sval(1) ? sval(1) : "";
    k->s[1] = sval(2) ? sval(2) : "";
  } else if (!strcmp(c, "done")) {
    emit(OP_DONE)->s[0] = sval(1) ? sval(1) : "";
  } else if (!strcmp(c, "shop")) {
    Cmd *k = emit(OP_SHOP);
    for (int i = 1; i < ntok && k->n < MAX_ARGS; i++) {
      int it = item_find(tok[i]);
      if (it < 0) err("nieznany przedmiot '%s'", tok[i]);
      k->i[k->n++] = it < 0 ? 0 : it;
    }
  } else if (!strcmp(c, "save")) {
    emit(OP_SAVE);
  } else if (!strcmp(c, "ending")) {
    Cmd *k = emit(OP_ENDING);
    k->s[0] = sval(1) ? sval(1) : "KONIEC";
    k->s[1] = sval(2) ? sval(2) : "";
  } else if (!strcmp(c, "face") || !strcmp(c, "walk")) {
    Cmd *k = emit(!strcmp(c, "face") ? OP_FACE : OP_WALK);
    k->s[0] = sval(1) ? sval(1) : "hero";
    k->i[0] = dir_parse(sval(2));
    k->i[1] = ival(3, 1);
  } else if (!strcmp(c, "call")) {
    Cmd *k = emit(OP_CALL);
    if (ncallfix < 256) { callfix[ncallfix].cmd = (int)(k - S.cmds); snprintf(callfix[ncallfix].name, 40, "%s", sval(1) ? sval(1) : ""); callfix[ncallfix].line = curLine; ncallfix++; }
  } else if (!strcmp(c, "toast")) {
    emit(OP_TOAST)->s[0] = sval(1) ? sval(1) : "";
  } else if (!strcmp(c, "chance")) {
    Cmd *k = emit(OP_CHANCE);
    k->i[0] = ival(1, 50);
    need_label(sval(2), &k->i[1]);
  } else if (!strcmp(c, "end")) {
    emit(OP_END);
  } else {
    err("nieznana komenda '%s'", c);
  }
}

static void parse_map_line(char *line) {
  tokenize(line);
  if (!ntok) return;
  const char *c = tok[0];
  MapDef *m = curMap;
  if (!strcmp(c, "name")) snprintf(m->name, sizeof m->name, "%s", sval(1) ? sval(1) : "");
  else if (!strcmp(c, "music")) { snprintf(m->music, sizeof m->music, "%s", sval(1) ? sval(1) : ""); if (!music_exists(m->music)) err("nieznana muzyka '%s'", m->music); }
  else if (!strcmp(c, "bg")) snprintf(m->bg, sizeof m->bg, "%s", sval(1) ? sval(1) : "");
  else if (!strcmp(c, "night")) m->night = ival(1, 120);
  else if (!strcmp(c, "rain")) m->rain = ival(1, 1);
  else if (!strcmp(c, "interior")) { m->interior = 1; m->ceil = sval(1) ? (float)atof(sval(1)) : 1.6f; }
  else if (!strcmp(c, "floors")) m->floors = ival(1, 3);
  else if (!strcmp(c, "ambient")) {
    static const char *AN[] = {"-", "street", "harbor", "room", "crowd", "office", "church", "warehouse"};
    m->ambient = -1;
    for (int k = 0; k < 8; k++) if (sval(1) && !strcmp(sval(1), AN[k])) m->ambient = k;
    if (m->ambient < 0) { err("nieznany ambient '%s'", sval(1)); m->ambient = 0; }
  }
  else if (!strcmp(c, "district")) m->district = ival(1, 0);
  else if (!strcmp(c, "entry")) { m->ex = ival(1, 0); m->ey = ival(2, 0); m->edir = dir_parse(sval(3)); }
  else if (!strcmp(c, "tiles")) { sec = SEC_TILES; curMapRows = 0; }
  else if (!strcmp(c, "npc") || !strcmp(c, "obj") || !strcmp(c, "warp") || !strcmp(c, "event") || !strcmp(c, "mob")) {
    if (m->nents >= MAX_ENTS) { err("za dużo obiektów na mapie", NULL); return; }
    EntDef *e = &m->ents[m->nents++];
    memset(e, 0, sizeof *e);
    e->script = -1;
    e->toMap = -1;
    if (!strcmp(c, "npc") || !strcmp(c, "obj")) {
      /* npc ID X Y SPRITE DIR SKRYPT [wander] [if X] */
      e->type = !strcmp(c, "npc") ? ENT_NPC : ENT_OBJ;
      snprintf(e->id, sizeof e->id, "%s", sval(1) ? sval(1) : "?");
      e->x = ival(2, 0); e->y = ival(3, 0);
      snprintf(e->sprite, sizeof e->sprite, "%s", sval(4) ? sval(4) : "");
      if (!art_exists(e->sprite)) err("nieznany sprite '%s'", e->sprite);
      e->dir = dir_parse(sval(5));
      /* nazwa skryptu rozwiązywana później — trzymamy w enemies[] tymczasowo */
      snprintf(e->enemies, sizeof e->enemies, "%s", sval(6) ? sval(6) : "-");
      parse_cond_ent(e, 7);
    } else if (!strcmp(c, "warp")) {
      /* warp X Y MAPA TX TY [dir] [if X] */
      e->type = ENT_WARP;
      e->x = ival(1, 0); e->y = ival(2, 0);
      snprintf(e->sprite, sizeof e->sprite, "%s", sval(3) ? sval(3) : "");
      e->toX = ival(4, 0); e->toY = ival(5, 0); e->toDir = dir_parse(sval(6));
      parse_cond_ent(e, 7);
    } else if (!strcmp(c, "event")) {
      /* event X Y SKRYPT [once] [if X] */
      e->type = ENT_EVENT;
      e->x = ival(1, 0); e->y = ival(2, 0);
      snprintf(e->enemies, sizeof e->enemies, "%s", sval(3) ? sval(3) : "-");
      snprintf(e->id, sizeof e->id, "ev%d_%d", e->x, e->y);
      parse_cond_ent(e, 4);
    } else {
      /* mob X Y SPRITE WROGOWIE [if X] */
      e->type = ENT_MOB;
      e->x = ival(1, 0); e->y = ival(2, 0);
      snprintf(e->sprite, sizeof e->sprite, "%s", sval(3) ? sval(3) : "");
      if (!art_exists(e->sprite)) err("nieznany sprite '%s'", e->sprite);
      snprintf(e->enemies, sizeof e->enemies, "%s", sval(4) ? sval(4) : "");
      char tmp[64];
      snprintf(tmp, sizeof tmp, "%s", e->enemies);
      for (char *x = strtok(tmp, ","); x; x = strtok(NULL, ","))
        if (enemy_find(x) < 0) err("nieznany wróg '%s'", x);
      snprintf(e->id, sizeof e->id, "mob%d_%d", e->x, e->y);
      e->wander = 1;
      parse_cond_ent(e, 5);
      for (int i = 5; i < ntok; i++) if (!strcmp(tok[i], "ally")) e->ally = 1;
      if (e->enemies[0] && enemy_find(e->enemies) < 0) err("nieznany wróg '%s'", e->enemies);
    }
  } else err("nieznana właściwość mapy '%s'", c);
}

static void parse_tiles_line(char *line) {
  int L = (int)strlen(line);
  while (L > 0 && (line[L - 1] == '\r' || line[L - 1] == '\n')) line[--L] = 0;
  if (!strcmp(line, "endtiles")) {
    curMap->h = curMapRows;
    sec = SEC_MAP;
    return;
  }
  if (curMapRows >= MAX_MAP_H) { err("mapa za wysoka", NULL); return; }
  if (L > MAX_MAP_W) { err("mapa za szeroka", NULL); L = MAX_MAP_W; }
  if (curMapRows == 0) curMap->w = L;
  else if (L != curMap->w) {
    char b[32];
    snprintf(b, sizeof b, "%d", L);
    err("wiersz mapy ma inną szerokość (%s)", b);
  }
  for (int x = 0; x < L && x < MAX_MAP_W; x++) {
    if (!tile_valid(line[x])) { char b[2] = {line[x], 0}; err("nieznany kafelek '%s'", b); }
    curMap->tiles[curMapRows][x] = (u8)line[x];
  }
  curMapRows++;
}

static void parse_kv(char *line) {
  tokenize(line);
  if (!ntok) return;
  const char *k = tok[0];
  if (sec == SEC_GAME) {
    if (!strcmp(k, "title")) snprintf(S.title, sizeof S.title, "%s", sval(1) ? sval(1) : "");
    else if (!strcmp(k, "start")) {
      snprintf(S.startMap, sizeof S.startMap, "%s", sval(1) ? sval(1) : "");
      S.startX = ival(2, 0); S.startY = ival(3, 0); S.startDir = dir_parse(sval(4));
      snprintf(S.startScript, sizeof S.startScript, "%s", sval(5) ? sval(5) : "");
    } else if (!strcmp(k, "hq")) {
      snprintf(S.hqMap, sizeof S.hqMap, "%s", sval(1) ? sval(1) : "");
      S.hqX = ival(2, 0); S.hqY = ival(3, 0); S.hqDir = dir_parse(sval(4));
    } else if (!strcmp(k, "stat")) {
      if (S.nstats < MAX_STATS) {
        snprintf(S.stats[S.nstats].var, 32, "%s", sval(1) ? sval(1) : "");
        snprintf(S.stats[S.nstats].label, 40, "%s", sval(2) ? sval(2) : "");
        int v = var_find(S.stats[S.nstats].var, 1);
        S.varReads[v]++;
        S.nstats++;
      }
    } else if (!strcmp(k, "speaker")) {
      if (S.nspeakers < MAX_SPEAKERS) {
        Speaker *s = &S.speakers[S.nspeakers++];
        snprintf(s->id, sizeof s->id, "%s", sval(1) ? sval(1) : "");
        snprintf(s->name, sizeof s->name, "%s", sval(2) ? sval(2) : "");
        snprintf(s->sprite, sizeof s->sprite, "%s", sval(3) ? sval(3) : "");
        if (s->sprite[0] && strcmp(s->sprite, "-") && !art_exists(s->sprite)) err("nieznany sprite '%s'", s->sprite);
      }
    } else err("nieznany klucz '%s'", k);
  } else if (sec == SEC_ITEM) {
    ItemDef *it = curItem;
    if (!strcmp(k, "name")) snprintf(it->name, sizeof it->name, "%s", sval(1) ? sval(1) : "");
    else if (!strcmp(k, "desc")) snprintf(it->desc, sizeof it->desc, "%s", sval(1) ? sval(1) : "");
    else if (!strcmp(k, "power")) it->power = ival(1, 0);
    else if (!strcmp(k, "price")) it->price = ival(1, 0);
    else if (!strcmp(k, "type")) {
      const char *t = sval(1) ? sval(1) : "";
      it->type = !strcmp(t, "heal") ? IT_HEAL : !strcmp(t, "mana") ? IT_MANA : !strcmp(t, "elixir") ? IT_ELIXIR :
                 !strcmp(t, "weapon") ? IT_WEAPON : !strcmp(t, "armor") ? IT_ARMOR : !strcmp(t, "charm") ? IT_CHARM : !strcmp(t, "ammo") ? IT_AMMO : IT_KEY;
    } else err("nieznany klucz przedmiotu '%s'", k);
  } else if (sec == SEC_ENEMY) {
    EnemyDef *e = curEnemy;
    if (!strcmp(k, "name")) snprintf(e->name, sizeof e->name, "%s", sval(1) ? sval(1) : "");
    else if (!strcmp(k, "sprite")) { snprintf(e->sprite, sizeof e->sprite, "%s", sval(1) ? sval(1) : ""); if (!art_exists(e->sprite)) err("nieznany sprite '%s'", e->sprite); }
    else if (!strcmp(k, "stats")) {
      /* stats HP OBRAŻENIA CELNOŚĆ% SZYBKOŚĆ KASA */
      e->hp = ival(1, 30); e->dmg = ival(2, 8); e->acc = ival(3, 40); e->speed = ival(4, 10); e->cash = ival(5, 0);
    } else if (!strcmp(k, "weapon")) {
      const char *w = sval(1) ? sval(1) : "";
      e->weapon = !strcmp(w, "fists") ? WPN_FISTS : !strcmp(w, "shotgun") ? WPN_SHOTGUN : !strcmp(w, "tommy") ? WPN_TOMMY : WPN_PISTOL;
    } else if (!strcmp(k, "boss")) e->boss = ival(1, 1);
    else err("nieznany klucz wroga '%s'", k);
  }
}

static void begin_section(char *line) {
  close_section();
  tokenize(line + 1);
  const char *t = ntok ? tok[0] : "";
  const char *id = ntok > 1 ? tok[1] : "";
  if (!strcmp(t, "game")) sec = SEC_GAME;
  else if (!strcmp(t, "item")) {
    if (S.nitems >= MAX_ITEMS) { err("za dużo przedmiotów", NULL); return; }
    curItem = &S.items[S.nitems++];
    memset(curItem, 0, sizeof *curItem);
    snprintf(curItem->id, sizeof curItem->id, "%s", id);
    curItem->type = IT_KEY;
    sec = SEC_ITEM;
  } else if (!strcmp(t, "enemy")) {
    if (S.nenemies >= MAX_ENEMIES) { err("za dużo wrogów", NULL); return; }
    curEnemy = &S.enemies[S.nenemies++];
    memset(curEnemy, 0, sizeof *curEnemy);
    snprintf(curEnemy->id, sizeof curEnemy->id, "%s", id);
    curEnemy->weapon = WPN_PISTOL;
    sec = SEC_ENEMY;
  } else if (!strcmp(t, "map")) {
    if (S.nmaps >= MAX_MAPS) { err("za dużo map", NULL); return; }
    curMap = &S.maps[S.nmaps++];
    memset(curMap, 0, sizeof *curMap);
    snprintf(curMap->id, sizeof curMap->id, "%s", id);
    snprintf(curMap->bg, sizeof curMap->bg, "street");
    curMap->floors = 3;
    curMap->ex = curMap->ey = -1;
    curMap->ceil = 1.6f;
    sec = SEC_MAP;
  } else if (!strcmp(t, "script")) {
    if (S.nscripts >= MAX_SCRIPTS) { err("za dużo skryptów", NULL); return; }
    if (script_find(id) >= 0) err("powtórzona nazwa skryptu '%s'", id);
    curScript = S.nscripts++;
    snprintf(S.scripts[curScript].name, sizeof S.scripts[curScript].name, "%s", id);
    S.scripts[curScript].start = S.ncmds;
    nlabels = 0; nfixups = 0;
    sec = SEC_SCRIPT;
  } else if (!strcmp(t, "end")) {
    sec = SEC_NONE;
  } else err("nieznana sekcja '@%s'", t);
}

int cond_check(int type, int a, int b) {
  switch (type) {
    case CND_IF: return H.vars[a] != 0;
    case CND_IFNOT: return H.vars[a] == 0;
    case CND_GE: return H.vars[a] >= b;
    case CND_LT: return H.vars[a] < b;
    case CND_GOLD: return H.gold >= a;
    case CND_HAS: return a >= 0 && H.inv[a] >= b;
    default: return 1;
  }
}

int story_load(const char *text) {
  memset(&S, 0, sizeof S);
  sec = SEC_NONE; curScript = -1; nlabels = nfixups = ncallfix = 0;
  size_t n = strlen(text);
  char *buf = (char *)malloc(n + 1); /* celowo nie zwalniany — komendy wskazują do środka */
  memcpy(buf, text, n + 1);
  char *p = buf;
  curLine = 0;
  while (p && *p) {
    char *nl = strchr(p, '\n');
    if (nl) *nl = 0;
    curLine++;
    char *line = p;
    p = nl ? nl + 1 : NULL;
    int L = (int)strlen(line);
    while (L > 0 && line[L - 1] == '\r') line[--L] = 0;
    if (sec == SEC_TILES) { parse_tiles_line(line); continue; }
    char *t = line;
    while (*t == ' ' || *t == '\t') t++;
    if (!*t || *t == '#') continue;
    if (*t == '@') { begin_section(t); continue; }
    switch (sec) {
      case SEC_SCRIPT: case SEC_MENU: parse_script_line(t); break;
      case SEC_MAP: parse_map_line(t); break;
      case SEC_GAME: case SEC_ITEM: case SEC_ENEMY: parse_kv(t); break;
      default: err("tekst poza sekcją", NULL);
    }
  }
  close_section();

  /* rozwiązywanie odwołań między sekcjami */
  for (int i = 0; i < ncallfix; i++) {
    curLine = callfix[i].line;
    int s = script_find(callfix[i].name);
    if (s < 0) err("nieznany skrypt '%s'", callfix[i].name);
    S.cmds[callfix[i].cmd].i[0] = s < 0 ? 0 : s;
  }
  for (int i = 0; i < S.ncmds; i++) {
    Cmd *c = &S.cmds[i];
    curLine = c->line;
    if (c->op == OP_WARP) {
      c->i[0] = map_find(c->s[0]);
      if (c->i[0] < 0) err("nieznana mapa '%s'", c->s[0]);
    }
  }
  for (int m = 0; m < S.nmaps; m++) {
    MapDef *md = &S.maps[m];
    curLine = 0;
    if (md->w == 0 || md->h == 0) err("mapa '%s' nie ma kafelków", md->id);
    for (int k = 0; k < md->nents; k++) {
      EntDef *e = &md->ents[k];
      char where[80];
      snprintf(where, sizeof where, "%s (%d,%d)", md->id, e->x, e->y);
      if (e->x < 0 || e->y < 0 || e->x >= md->w || e->y >= md->h) err("obiekt poza mapą: %s", where);
      if (e->type == ENT_NPC || e->type == ENT_OBJ || e->type == ENT_EVENT) {
        if (strcmp(e->enemies, "-")) {
          e->script = script_find(e->enemies);
          if (e->script < 0) { char b[120]; snprintf(b, sizeof b, "%s w %s", e->enemies, where); err("nieznany skrypt '%s'", b); }
        }
        e->enemies[0] = 0;
      }
      if (e->type == ENT_WARP) {
        e->toMap = map_find(e->sprite);
        if (e->toMap < 0) { char b[120]; snprintf(b, sizeof b, "%s z %s", e->sprite, where); err("nieznana mapa docelowa '%s'", b); }
        else {
          MapDef *t = &S.maps[e->toMap];
          if (e->toX < 0 || e->toY < 0 || e->toX >= t->w || e->toY >= t->h || tile_solid(t->tiles[e->toY][e->toX])) {
            char b[120]; snprintf(b, sizeof b, "%s -> %s (%d,%d)", where, t->id, e->toX, e->toY);
            err("cel przejścia na ścianie/poza mapą: %s", b);
          }
        }
      }
      if ((e->type == ENT_NPC || e->type == ENT_MOB) && e->x < md->w && e->y < md->h && tile_solid(md->tiles[e->y][e->x]))
        warn("postać stoi na ścianie: %s", where);
    }
  }
  if (map_find(S.startMap) < 0) err("nieznana mapa startowa '%s'", S.startMap);
  if (S.hqMap[0] && map_find(S.hqMap) < 0) err("nieznana mapa kwatery '%s'", S.hqMap);
  if (S.startScript[0] && script_find(S.startScript) < 0) err("nieznany skrypt startowy '%s'", S.startScript);
  for (int v = 0; v < S.nvars; v++)
    if (S.varReads[v] && !S.varWrites[v] && S.varnames[v][0] != '_') { curLine = 0; warn("zmienna '%s' jest sprawdzana, ale nigdy ustawiana", S.varnames[v]); }
  return S.errors;
}
