# Generator map 3D dla KroniX: Rodzina -> data/10_mapy.txt (uruchom: python3 tools/maps.py)
import sys
import os
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data', '10_mapy.txt')
SOLID = set('RWwGaD#gijMQxzn' + 'XkthbSPVdeBufAYFIm' + '~Tp' + 'L' + 'cC')
maps = []

def grid(w, h, fill):
    return [[fill] * w for _ in range(h)]

def put(g, x, y, s):
    for i, ch in enumerate(s):
        g[y][x + i] = ch

def rect(g, x0, y0, x1, y1, ch):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            g[y][x] = ch

def border(g, ch):
    h, w = len(g), len(g[0])
    for x in range(w):
        g[0][x] = ch; g[h - 1][x] = ch
    for y in range(h):
        g[y][0] = ch; g[y][w - 1] = ch

def add(mid, props, g, ents):
    rows = [''.join(r) for r in g]
    w = len(rows[0])
    for r in rows:
        assert len(r) == w, (mid, r)
    # kontrola: npc/mob/event na podłodze
    for e in ents:
        t = e.split()
        if t[0] in ('npc', 'mob', 'event'):
            x, y = int(t[2] if t[0] == 'npc' else t[1]), int(t[3] if t[0] == 'npc' else t[2])
            if rows[y][x] in SOLID:
                print('UWAGA %s: %s stoi na "%s"' % (mid, e, rows[y][x]), file=sys.stderr)
    maps.append((mid, props, rows, ents))

# ------------------------------------------------------------------ mieszkanie Tomka
g = grid(10, 7, '_')
border(g, 'i')
put(g, 0, 0, 'iiiiiiiiii')
put(g, 1, 1, 'B___i_f_')
put(g, 1, 3, '__t_____')
put(g, 1, 4, '___i__u_')
put(g, 0, 6, 'iiiiDiiiii')
add('dom', ['ambient room', 'name "Mieszkanie Kowalskich"', 'music polonia', 'night 120', 'interior 1.5', 'bg wood'], g, [
    'npc mama 7 3 mama left s_mama',
    'obj zdjecie 3 3 papers down s_zdjecie',
    'warp 4 6 polonia 5 2 down',
])

# ------------------------------------------------------------------ Polonia (ulica)
W_, H_ = 34, 11
g = grid(W_, H_, '.')
for x in range(W_):
    g[0][x] = 'R'; g[1][x] = 'R'; g[9][x] = 'R'; g[10][x] = 'R'
put(g, 0, 1, 'RRRRwDwRRRRRGGaDaGGRRRRRRRwwDwwRRR')
put(g, 0, 9, 'RRRRRRRRWDWRRRRRRRRRRwwDwwRRRRRRRR')
for y in (2, 3, 7, 8):
    for x in range(1, W_ - 1):
        g[y][x] = ','
    g[y][0] = 'R'; g[y][W_ - 1] = 'R'
for y in (4, 5, 6):
    g[y][0] = 'R'
for x in range(1, W_):
    g[5][x] = '-'
for x, y in ((12, 2), (25, 2), (12, 8), (28, 8)):
    g[y][x] = 'L'
for x, y in ((3, 8), (21, 8)):
    g[y][x] = 'T'
g[4][9] = 'c'; g[6][6] = 'c'; g[6][17] = 'C'; g[4][24] = 'c'
g[3][30] = 'X'; g[3][31] = 'k'; g[7][2] = 'k'
add('polonia', ['name "Polonia — Back of the Yards"', 'music polonia', 'night 120', 'rain 1', 'district 2', 'entry 31 5 left'], g, [
    'warp 5 1 dom 4 5 up',
    'warp 15 1 piekarnia 5 5 up',
    'warp 33 4 italia 1 5 right', 'warp 33 5 italia 1 5 right', 'warp 33 6 italia 1 5 right',
    'npc bronek 26 3 bronek down s_bronek',
    'npc menel 4 7 menel up s_menel wander',
    'npc robotnik 18 7 robotnik up s_robotnik wander',
    'npc zoska 9 8 zoska up s_zoska',
    'npc vito_p 16 3 vito down s_vito_polonia if PIEKARNIA_OK ifnot DON_POZNANY',
])

# ------------------------------------------------------------------ piekarnia
g = grid(12, 8, '_')
border(g, 'Q')
put(g, 1, 1, 'bbbbb_SSS_')
put(g, 2, 4, 'X')
put(g, 7, 4, 't')
put(g, 9, 4, 'k')
put(g, 0, 7, 'QQQQQDQQQQQQ')
add('piekarnia', ['ambient room', 'name "Piekarnia Wiśniewskiego"', 'music polonia', 'night 120', 'interior 1.6', 'bg wood'], g, [
    'warp 5 7 polonia 15 2 down',
    'npc piekarz 3 2 piekarz down s_piekarz',
    'npc zb1 4 3 sal down s_zbiry_piek if MISJA_PIEKARNIA ifnot PIEKARNIA_OK',
    'npc zb2 6 3 enzo down s_zbiry_piek if MISJA_PIEKARNIA ifnot PIEKARNIA_OK',
])

# ------------------------------------------------------------------ Mała Italia
W_, H_ = 36, 11
g = grid(W_, H_, '.')
put(g, 0, 0, 'R' * W_)
put(g, 0, 1, 'RRRwwwGaDaGwwwRRRRRgggDgggRRRRwGDGwR')
put(g, 0, 9, 'RRRRRwwGDGwwRRRRRRRRRRRRwwaDawwRRRRR')
put(g, 0, 10, 'R' * W_)
for y in (2, 3, 7, 8):
    for x in range(1, W_ - 1):
        g[y][x] = ','
    g[y][0] = 'R'; g[y][W_ - 1] = 'R'
for y in (4, 5, 6):
    g[y][W_ - 1] = 'R'
for x in range(0, W_ - 1):
    g[5][x] = '-'
for x, y in ((4, 3), (17, 3), (30, 3), (14, 7), (22, 8)):
    g[y][x] = 'L'
for x, y in ((2, 8), (19, 8), (34, 8)):
    g[y][x] = 'T'
g[4][10] = 'c'; g[4][21] = 'c'; g[6][32] = 'C'; g[6][14] = 'c'
g[2][12] = 't'; g[2][13] = 't'
add('italia', ['name "Mała Italia"', 'music italia', 'night 120', 'district 1', 'entry 26 6 left'], g, [
    'warp 0 4 polonia 32 5 left', 'warp 0 5 polonia 32 5 left', 'warp 0 6 polonia 32 5 left',
    'warp 8 1 trattoria 6 8 up',
    'warp 22 1 kosciol 7 11 up',
    'warp 32 1 sklep 4 5 up',
    'npc vito 9 2 vito down s_vito if DON_POZNANY ifnot DON_NIE_ZYJE',
    'npc luigi 8 8 pan up s_luigi',
    'npc rosa 12 7 pani up s_rosa',
    'npc paddy 27 8 paddy up s_paddy',
    'npc stefek 31 7 stefek up s_kierowca if DON_POZNANY ifnot STEFEK_OUT',
    'npc kierowca2 31 7 mafioso up s_kierowca if STEFEK_OUT',
    'npc gliniarz 20 2 gliniarz down s_gliniarz wander',
    'npc pani2 25 7 pani up s_przechodzien wander',
    'npc parada1 15 3 flapper down - dance if _S_K_BALBO',
    'npc parada2 16 3 pan down - dance if _S_K_BALBO',
    'npc parada3 18 7 pani up - dance if _S_K_BALBO',
    'npc parada4 19 7 pan up - dance if _S_K_BALBO',
    'npc parada5 11 3 muzyk down - play if _S_K_BALBO',
    'npc parada6 12 3 muzyk down - play if _S_K_BALBO',
    'npc ksiadz_p 16 7 ksiadz up - if _S_K_BALBO',
    'npc babcia_p 15 2 mama down - if _S_K_BALBO',
])

# ------------------------------------------------------------------ sklep Gina (broń)
g = grid(10, 7, '_')
border(g, 'i')
put(g, 1, 1, 'uuSSSuu_')
put(g, 1, 3, '_bbbbb__')
put(g, 0, 6, 'iiiiDiiiii')
add('sklep', ['name "Sklep z narzędziami Gina"', 'music italia', 'night 120', 'interior 1.5', 'bg wood'], g, [
    'warp 4 6 italia 32 2 down',
    'npc gino 4 2 gino down s_gino',
])

# ------------------------------------------------------------------ trattoria (kwatera)
g = [list(r) for r in [
    'jjjjjjjjjjjjjjjjjjjjjj',
    'jSSSSqqqqqqqqqjuuuuuuj',
    'jqqqqqqqqqqqqqjrrrrrrj',
    'jbbbbqqtqqqtqqjrrdddrj',
    'jqqqqqqqqqqqqqjrrrrrrj',
    'jqqqqqqtqqqtqqqrrrrrrj',
    'jqqqqqqqqqqqqqjrrrrrrj',
    'jPqqqqqtqqqtqqjrrrrfrj',
    'jqqqqqqqqqqqqqjjjjjjjj',
    'jjjjjjDjjjjjjjjjjjjjjj']]
add('trattoria', ['ambient crowd', 'name "Trattoria Bella Napoli"', 'music italia', 'night 120', 'interior 1.8'], g, [
    'warp 6 9 italia 8 2 down',
    'npc don 18 2 don down s_don ifnot DON_NIE_ZYJE',
    'npc kelner 2 2 kelner down s_kelner',
    'npc lucia 10 6 lucia left s_lucia ifnot _S_K_LUCIA_KSIEGI ifnot _S_K_ZAMACH',
    'npc vito_hq 8 4 vito down s_vito_hq if SZEF if VITO_ZYJE ifnot _S_K_VITO',
    'npc bronek_hq 12 7 bronek up s_bronek_hq if SZEF if BRONEK ifnot _S_K_BRONEK',
    'npc kane 5 6 kane right s_kane if MISJA_URODZINY ifnot STRZELANINA',
    'npc gosc1 9 2 pan down s_gosc if MISJA_URODZINY ifnot STRZELANINA',
    'npc gosc2 12 4 pani left s_gosc if MISJA_URODZINY ifnot STRZELANINA',
    'obj ksiega 18 3 ledger up s_ksiega if SZEF',
    'npc lucia_o 16 3 lucia right - if _S_K_LUCIA_KSIEGI',
    'npc bronek_o 16 3 bronek right - if _S_K_BRONEK',
    'npc gino_o 16 3 gino right - if _S_K_SZCZUR',
    'npc kessler_o 16 3 kessler right - if _S_K_KESSLER2',
    'npc agent_o1 15 2 agent down - if _S_K_KESSLER2',
    'npc agent_o2 19 5 agent left - if _S_K_KESSLER2',
    'npc vito_o 16 3 vito right - if _S_K_VITO',
    'npc kessler_t 8 5 kessler left - sit if _S_K_KESSLER',
    'npc lucia_z 11 4 lucia left - sit if _S_K_ZAMACH',
])

# ------------------------------------------------------------------ kościół św. Rocha
g = [list(r) for r in [
    '###############',
    '#_____AAA_____#',
    'g_____________g',
    '#_hhhh___hhhh_#',
    'g_____________g',
    '#_hhhh___hhhh_#',
    'g_____________g',
    '#_hhhh___hhhh_#',
    'g_____________g',
    '#_hhhh___hhhh_#',
    'g_____________g',
    '#_____________#',
    '#######D#######']]
add('kosciol', ['ambient church', 'name "Kościół św. Rocha"', 'music kosciol', 'night 120', 'interior 2.6', 'bg stone'], g, [
    'warp 7 12 italia 22 2 down',
    'npc ksiadz 7 2 ksiadz down s_ksiadz',
    'npc lucia_k 5 4 lucia down s_lucia_pogrzeb if MISJA_POGRZEB ifnot POGRZEB_OK',
    'npc vito_k 9 4 vito down s_vito_pogrzeb if MISJA_POGRZEB ifnot POGRZEB_OK',
    'npc enzo_k 11 8 enzo up s_enzo if MISJA_POGRZEB ifnot POGRZEB_OK',
    'npc babcia 3 6 mama down s_babcia',
    'npc lucia_c 7 4 lucia down - if _S_K_LUCIA',
])

# ------------------------------------------------------------------ doki
W_, H_ = 30, 14
g = grid(W_, H_, 'o')
for y in range(H_):
    g[y][0] = 'M'
    for x in range(22, 24): g[y][x] = '='
    for x in range(24, W_): g[y][x] = '~'
for x in range(22):
    g[0][x] = 'M'; g[H_ - 1][x] = 'M'
for y in (5, 6, 7):
    for x in range(24, W_): g[y][x] = '='   # molo
g[6][0] = 'o'; g[7][0] = 'o'
for x, y in ((2, 2), (3, 2), (2, 3), (3, 3), (15, 2), (16, 2), (15, 3), (9, 10), (10, 10), (16, 9), (17, 9), (16, 10), (12, 4), (20, 11)):
    g[y][x] = 'X'
for x, y in ((7, 2), (8, 2), (19, 3), (5, 11), (6, 11), (13, 11), (21, 2)):
    g[y][x] = 'k'
g[6][8] = 'C'; g[6][9] = 'C'; g[10][3] = 'c'
for x, y in ((4, 5), (18, 5), (12, 12)):
    g[y][x] = 'L'
g[9][26] = 'm'; g[3][27] = 'm'
add('doki', ['ambient harbor', 'name "Doki nad rzeką"', 'music doki', 'night 120', 'rain 1', 'district 3', 'floors 2', 'entry 1 6 right'], g, [
    'warp 0 6 italia 30 6 left', 'warp 0 7 italia 30 6 left',
    'npc gino_d 12 6 gino left s_gino_doki if MISJA_DOKI ifnot DOKI_OK',
    'npc dokowiec 20 7 dokowiec left s_dokowiec wander',
    'npc mickey 18 7 mickey left s_mickey if DOKI_WALKA_OK ifnot MICKEY_DONE',
])

# ------------------------------------------------------------------ Chinatown
W_, H_ = 30, 11
g = grid(W_, H_, '.')
put(g, 0, 0, 'R' * W_)
put(g, 0, 1, 'RRaGaRRRaaDaaRRRRGGaDaGGRRRRRR')
put(g, 0, 9, 'RRRRGaGaGRRRRRRaGGGaRRRRRRRRRR')
put(g, 0, 10, 'R' * W_)
for y in (2, 3, 7, 8):
    for x in range(1, W_ - 1): g[y][x] = ','
    g[y][0] = 'R'; g[y][W_ - 1] = 'R'
for y in (4, 5, 6): g[y][W_ - 1] = 'R'
for x in range(0, W_ - 1): g[5][x] = '-'
for x, y in ((3, 2), (9, 2), (15, 2), (21, 2), (27, 2), (6, 8), (13, 8), (24, 8)):
    g[y][x] = 'L'
g[4][14] = 'c'; g[6][22] = 'c'
add('chinatown', ['name "Chinatown"', 'music chiny', 'night 120', 'district 5', 'entry 2 5 right'], g, [
    'warp 0 4 italia 30 6 left', 'warp 0 5 italia 30 6 left', 'warp 0 6 italia 30 6 left',
    'warp 10 1 herbaciarnia 6 7 up',
    'npc straz_t 11 2 triada down s_straz_triady',
    'npc chinka 18 7 pani up s_przechodzien wander',
    'npc sprzedawca 20 2 triada down s_sprzedawca',
])

g = [list(r) for r in [
    'zzzzzzzzzzzzzz',
    'z__j_______j_z',
    'z_t____d_____z',
    'z_____rrr____z',
    'z_t___rrr__t_z',
    'z_____rrr____z',
    'z_t__________z',
    'z____________z',
    'zzzzzzDzzzzzzz']]
add('herbaciarnia', ['ambient crowd', 'name "Herbaciarnia Złoty Smok"', 'music chiny', 'night 120', 'interior 1.7'], g, [
    'warp 6 8 chinatown 10 2 down',
    'npc lee 7 1 lee down s_lee ifnot _S_K_LEE',
    'npc lee_k 7 4 lee down - sit if _S_K_LEE',
    'npc tri1 4 2 triada down s_straz_triady',
    'npc tri2 10 2 triada down s_straz_triady',
])

# ------------------------------------------------------------------ browar Russo (Levee)
W_, H_ = 28, 16
g = grid(W_, H_, 'o')
border(g, 'M')
for x, y in ((4, 3), (7, 3), (10, 3), (4, 7), (7, 7), (10, 7)):
    g[y][x] = 'V'
for x in range(15, 26):
    g[8][x] = 'M'
g[8][19] = 'o'; g[8][20] = 'o'
for x, y in ((16, 3), (17, 3), (16, 4), (22, 2), (23, 2), (22, 3), (3, 12), (4, 12), (12, 11), (13, 11), (18, 12), (24, 12), (24, 11)):
    g[y][x] = 'X'
for x, y in ((19, 4), (20, 4), (25, 5), (8, 12), (9, 12), (15, 13), (20, 13)):
    g[y][x] = 'k'
g[11][6] = 'C'; g[11][7] = 'C'
g[15][13] = 'o'; g[15][14] = 'o'
add('browar', ['ambient warehouse', 'name "Levee — browar Russo"', 'music akcja', 'night 120', 'interior 2.4', 'district 8', 'entry 13 14 up'], g, [
    'warp 13 15 italia 30 6 left', 'warp 14 15 italia 30 6 left',
    'npc vito_b 12 13 vito up s_vito_browar if MISJA_BROWAR ifnot BROWAR_OK',
    'npc robotnik_b 21 12 robotnik left s_robotnik_browar if BROWAR_OK',
])

# ------------------------------------------------------------------ lokale Rodziny (wizyty z Księgi, sceny kroniki)
# klub jazzowy Blue Moon
W_, H_ = 22, 15
g = grid(W_, H_, 'r')
border(g, 'z')
rect(g, 1, 1, 20, 3, 'y')
rect(g, 5, 4, 16, 6, 'q')
g[2][3] = 'P'
for x, y in ((2, 5), (2, 8), (2, 11), (6, 8), (13, 8), (6, 11), (14, 11), (19, 5)):
    g[y][x] = 't'
for y in range(8, 12):
    g[y][17] = 'b'; g[y][20] = 'S'
g[13][1] = 'p'; g[13][20] = 'p'; g[1][1] = 'p'; g[1][20] = 'p'
g[14][10] = 'D'
add('klub', ['ambient crowd', 'name "Blue Moon — klub jazzowy"', 'music bar', 'night 120', 'interior 2.0', 'entry 10 12 up'], g, [
    'event 10 13 w_wyjscie',
    'npc pianista 3 3 muzyk up w_muzyk play',
    'obj kontrabas 7 2 upbass down -',
    'npc basista 7 3 muzyk down w_muzyk play',
    'obj perkusja 13 2 drums down -',
    'npc perkusista 13 3 muzyk down w_muzyk play',
    'npc trebacz 16 3 muzyk down w_muzyk play',
    'obj mikrofon 10 3 mic down -',
    'npc dolores 10 2 dolores down w_dolores',
    'npc tan1 7 5 flapper right w_tancerka dance',
    'npc tan2 8 5 pan left w_bywalec dance',
    'npc tan3 12 4 pani down w_tancerka dance',
    'npc tan4 13 5 pan up w_bywalec dance',
    'npc tan5 10 6 flapper up w_tancerka dance',
    'npc gosc_k1 3 5 pan left w_bywalec sit',
    'npc gosc_k2 3 8 pani left w_bywalec sit',
    'npc gosc_k3 7 8 pan left w_bywalec sit',
    'npc gosc_k4 12 8 flapper right w_tancerka sit',
    'npc gosc_k5 5 11 pan right w_bywalec sit',
    'npc gosc_k6 15 11 pani left w_bywalec sit',
    'npc barman_k 19 9 barman left w_barman',
    'npc gosc_k7 16 10 pan right w_bywalec',
    'npc bramkarz_k 12 13 ochroniarz up w_bramkarz',
    'npc kierownik_k 18 13 lucky left w_kierownik',
    'npc lucia_k 11 11 lucia up - if _S_K_KLUB',
    'npc lucia_k2 11 11 lucia up - if _S_K_REPEAL',
    'npc kane_k 12 8 kane right - sit if _S_K_KANE',
    'npc kane_ochr 14 9 ochroniarz left - if _S_K_KANE',
    'npc vito_k 4 5 vito right - sit if _S_K_REPEAL if _VITO_JEST',
    'npc bronek_k 18 10 bronek left - if _S_K_REPEAL if BRONEK_ZOSTAL',
])

# melina za zakładem fryzjerskim
W_, H_ = 16, 11
g = grid(W_, H_, '_')
border(g, 'i')
for x in range(2, 9):
    g[1][x] = 'S'; g[3][x] = 'b'
g[1][13] = 'k'; g[1][14] = 'k'; g[2][14] = 'k'; g[5][14] = 'k'
g[7][14] = 'P'
for x, y in ((4, 6), (9, 6), (4, 8), (11, 8)):
    g[y][x] = 't'
g[10][8] = 'D'
add('melina', ['ambient crowd', 'name "Melina „U Fryzjera”"', 'music bar', 'night 120', 'interior 1.5', 'entry 8 8 up'], g, [
    'event 8 9 w_wyjscie',
    'npc barman_m 5 2 barman down w_barman',
    'npc pianista_m 13 7 muzyk right w_muzyk play',
    'npc gosc_m1 5 6 robotnik left w_bywalec sit',
    'npc gosc_m2 3 6 dokowiec right w_bywalec sit',
    'npc gosc_m3 10 6 pan left w_bywalec sit',
    'npc gosc_m4 12 8 flapper left w_tancerka sit',
    'npc gosc_m5 7 4 menel up w_pijak',
    'npc gosc_m6 3 4 marynarz up w_bywalec',
    'npc bramkarz_m 6 9 ochroniarz up w_bramkarz',
    'npc kierownik_m 10 4 mafioso down w_kierownik',
])

# kasyno na zapleczu
W_, H_ = 18, 12
g = grid(W_, H_, 'r')
border(g, 'j')
for x, y in ((4, 3), (13, 3), (4, 7)):
    g[y][x] = 't'
for x in range(13, 17):
    g[8][x] = 'b'
for x in range(13, 17):
    g[10][x] = 'S'
g[1][1] = 'p'; g[1][16] = 'p'
g[11][9] = 'D'
add('kasyno', ['ambient crowd', 'name "Kasyno „Złota Podkowa”"', 'music bar', 'night 120', 'interior 1.7', 'entry 9 9 up'], g, [
    'event 9 10 w_wyjscie',
    'obj ruletka 9 3 roulette down -',
    'npc krupier 9 2 kelner down w_krupier',
    'npc gracz1 8 4 pan up w_gracz',
    'npc gracz2 10 4 pani up w_gracz',
    'npc gracz3 3 3 pan right w_gracz sit',
    'npc gracz4 5 3 lucky left w_gracz sit',
    'npc gracz5 12 3 pan right w_gracz sit',
    'npc gracz6 14 3 flapper left w_gracz sit',
    'npc gracz7 3 7 robotnik right w_gracz sit',
    'npc gracz8 5 7 pan left w_gracz sit',
    'npc barman_ka 15 9 barman up w_barman',
    'npc bramkarz_ka 7 10 ochroniarz up w_bramkarz',
    'npc kierownik_ka 11 7 mafioso down w_kierownik',
])

# biuro bukmachera
W_, H_ = 14, 9
g = grid(W_, H_, 'o')
border(g, 'Q')
for x in (3, 7, 10):
    g[2][x] = 'd'
g[1][12] = 'k'
g[8][7] = 'D'
add('bukmacher', ['ambient room', 'name "Biuro bukmacherskie „Pewniak”"', 'music bar', 'night 120', 'interior 1.5', 'entry 7 6 up'], g, [
    'event 7 7 w_wyjscie',
    'obj tablica 5 1 chalkboard down -',
    'obj radio_b 11 3 radio down -',
    'obj telefon_b 3 3 phone down -',
    'npc urzednik1 3 1 kelner down w_urzednik',
    'npc urzednik2 10 1 kelner down w_urzednik',
    'npc gracz_b1 4 4 robotnik up w_gracz',
    'npc gracz_b2 9 4 pan up w_gracz',
    'npc gracz_b3 11 5 dokowiec left w_gracz',
    'npc kierownik_b 7 1 lucky down w_kierownik',
])

# hala bokserska
W_, H_ = 20, 14
g = grid(W_, H_, 'o')
border(g, 'M')
rect(g, 8, 4, 10, 6, 'y')
for x in range(3, 17):
    if x < 7 or x > 11:
        g[9][x] = 'e'
for y in (3, 5, 7):
    g[y][3] = 'e'; g[y][15] = 'e'
g[1][1] = 'k'; g[1][2] = 'X'; g[1][17] = 'X'; g[12][17] = 'k'
g[13][9] = 'D'
add('hala', ['ambient warehouse', 'name "Hala bokserska na Halsted"', 'music walka', 'night 120', 'interior 2.6', 'entry 9 11 up'], g, [
    'event 9 12 w_wyjscie',
    'obj ring 9 5 boxring down -',
    'npc bokser_h1 8 5 bokser right w_bokser dance',
    'npc bokser_h2 10 5 bokser2 left w_bokser dance',
    'npc sedzia 9 4 kelner down w_bywalec',
    'npc kibic1 4 8 robotnik up w_kibic',
    'npc kibic2 5 8 dokowiec up w_kibic',
    'npc kibic3 13 8 pan up w_kibic',
    'npc kibic4 14 8 marynarz up w_kibic',
    'npc kibic5 4 4 robotnik right w_kibic',
    'npc kibic6 14 6 pan left w_kibic',
    'npc trener 12 10 feliks up w_trener',
    'npc kierownik_h 6 10 lucky up w_kierownik',
])

# ------------------------------------------------------------------ mapy akcji (tycoon)
# zaułek
W_, H_ = 28, 16
g = grid(W_, H_, ':')
border(g, 'W')
for y in range(1, H_ - 1):
    g[y][1] = 'R'; g[y][W_ - 2] = 'R'
for x in range(1, W_ - 1):
    g[1][x] = 'R'; g[H_ - 2][x] = 'R'
for x in range(2, 12): g[6][x] = 'W'
for x in range(16, 26): g[9][x] = 'W'
g[6][7] = ':'; g[9][21] = ':'
for x, y in ((4, 3), (5, 3), (9, 4), (20, 3), (21, 3), (14, 7), (6, 11), (7, 11), (18, 12), (24, 6), (12, 10)):
    g[y][x] = 'X'
for x, y in ((3, 8), (10, 8), (16, 5), (23, 11), (13, 13), (19, 7)):
    g[y][x] = 'k'
g[4][14] = 'c'; g[11][10] = 'C'; g[11][11] = 'C'
for x, y in ((8, 2), (22, 7), (15, 12)):
    g[y][x] = 'L'
for x in range(3, 8): g[13][x] = 'F'
add('op_ulica', ['name "Zaułek"', 'music akcja', 'night 140', 'rain 1', 'floors 4', 'entry 3 12 up'], g, [])

# magazyn
W_, H_ = 26, 18
g = grid(W_, H_, 'o')
border(g, 'M')
for x0 in (4, 10, 16):
    for y in range(3, 7): g[y][x0] = 'X'; g[y][x0 + 1] = 'X'
    for y in range(10, 14): g[y][x0] = 'X'; g[y][x0 + 1] = 'k'
for x, y in ((22, 3), (22, 4), (21, 8), (2, 8), (8, 8), (14, 8), (22, 14), (3, 15)):
    g[y][x] = 'k'
g[8][19] = 'V'; g[2][23] = 'V'
g[15][10] = 'C'; g[15][11] = 'C'
add('op_magazyn', ['ambient warehouse', 'name "Magazyn"', 'music akcja', 'night 120', 'interior 2.6', 'entry 2 15 up'], g, [])

# bank
g = [list(r) for r in [
    'QQQQQQQQQQQQQQQQQQQQQQQQ',
    'QIIIIIIIIIqqqqqqqqqqqQQQ',
    'QqqXqqqmqIqqqqqqqqqqqqQQ',
    'QqqqqqqqqIqqdqqqdqqqqqqQ',
    'QqmqqXqqqIqqqqqqqqqqqqqQ',
    'QIIIIqIIIIqqqqqqqqqqqqqQ',
    'Qqqqqqqqqqqqqqqqqqqqqq_Q',
    'Qbbbbbbbbbbbqqqbbbbbbb_Q',
    'Qqqqqqqqqqqqqqqqqqqqqq_Q',
    'QqqtqqqqqqqqqqqqqqqqtqqQ',
    'QqqqqqqqhhhhhqqqqqqqqqqQ',
    'QqqqqqqqqqqqqqqqqqqqqqqQ',
    'QqqtqqqqhhhhhqqqqqqqtqqQ',
    'QqqqqqqqqqqqqqqqqqqqqqqQ',
    'QQQQQQQQQQQDQQQQQQQQQQQQ']]
add('op_bank', ['ambient church', 'name "First National Bank"', 'music akcja', 'interior 2.4', 'entry 11 13 up'], g, [])

# rezydencja bossa
g = [list(r) for r in [
    'jjjjjjjjjjjjjjjjjjjjjjjjjj',
    'juuuurrrrrrjrrrrrrrrrrfrrj',
    'jrrrrrrrrrrjrrrrrrrrrrrrrj',
    'jrrrddrrrrrjrrttrrrrttrrrj',
    'jrrrrrrrrrrrrrrrrrrrrrrrrj',
    'jrrrrrrrrrrjrrrrrrrrrrrrrj',
    'jjjjjjrjjjjjjjjjjrjjjjjjjj',
    'jqqqqqqqqqqqqqqqqqqqqqqqqj',
    'jqqPqqqqqqqqqqqqqqqqqqqqqj',
    'jqqqqqqqtqqqqqqqtqqqqqqbbj',
    'jqqqqqqqqqqqqqqqqqqqqqqqqj',
    'jjjjjjjjqqqjjjjjjjjjjjjjjj',
    'jrrrrrrrqqqrrrrrrrrrrrrrrj',
    'jrrhhrrrrrrrrrrrrrrhhrrrrj',
    'jrrrrrrrrrrrrrrrrrrrrrrrrj',
    'jjjjjjjjjjjDjjjjjjjjjjjjjj']]
add('op_hq', ['ambient office', 'name "Rezydencja"', 'music boss', 'night 120', 'interior 1.9'], g, [])
for m in maps:
    if m[0] == 'op_hq':
        m[1].append('entry 9 14 up')

# ====================================================================== MIASTO: jedna duża mapa
# Ulice dzielnic (polonia, italia, chinatown, doki) są wklejane w jedną siatkę Chicago i połączone
# alejami. Stare nazwy map stają się regionami (przesunięcie współrzędnych liczy silnik).
import random
random.seed(1933)
MW, MH = 150, 110
city = grid(MW, MH, '#')
PARTS = {}   # nazwa -> (ox, oy)

def hstreet(y, x0, x1, dash=True):
    """ulica wschód-zachód: wiersze y..y+6 (chodnik, chodnik, jezdnia x3, chodnik, chodnik)"""
    for x in range(x0, x1 + 1):
        for k in range(7):
            ch = ',' if k in (0, 1, 5, 6) else '.'
            if k == 3 and dash and x % 3 != 0: ch = '-'
            cur = city[y + k][x]
            if cur in '.-|' and ch == ',':
                continue  # skrzyżowanie: jezdnia wygrywa
            city[y + k][x] = ch

def vstreet(x, y0, y1, dash=True, width=7):
    for y in range(y0, y1 + 1):
        for k in range(width):
            side = (0, 1, 5, 6) if width == 7 else (0, 4)
            ch = ',' if k in side else '.'
            if width == 7 and k == 3 and dash and y % 3 != 0: ch = '|'
            if width == 5 and k == 2 and dash and y % 3 != 0: ch = '|'
            cur = city[y][x + k]
            if cur in '.-|' and ch == ',':
                continue
            if cur in '-' and ch in '.|':
                ch = '.'
            city[y][x + k] = ch

def paste(name, ox, oy):
    m = [mm for mm in maps if mm[0] == name][0]
    rows = m[2]
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            city[oy + y][ox + x] = ch
    PARTS[name] = (ox, oy, len(rows[0]), len(rows))

# --- siatka ulic
hstreet(12, 3, 139)            # Grand Avenue (North Side, dojazd do doków)
hstreet(86, 3, 88)             # Cermak Road (Levee); dalej Chinatown
vstreet(3, 12, 92)             # Ashland Avenue (zachód)
vstreet(82, 12, 92)            # Halsted Street (wschód)
# --- dzielnice
paste('polonia', 12, 46)
paste('italia', 46, 46)
paste('chinatown', 89, 84)
paste('doki', 110, 8)
# Polonia: otwarcie zachodniego końca ulicy na Ashland
for y in range(46 + 2, 46 + 9):
    for x in range(10, 13):
        city[y][x] = ',' if y in (48, 49, 54, 55) else ('.' if y != 51 else '-')
# Mała Italia: otwarcie wschodniego końca na Halsted
for y in range(46 + 2, 46 + 9):
    city[y][81] = ',' if y in (48, 49, 54, 55) else ('.' if y != 51 else '-')
# przecznice przez kwartały (ulice boczne 5 kafli)
vstreet(19, 19, 47, width=5)   # Polonia -> Grand
vstreet(19, 55, 86, width=5)   # Polonia -> Cermak
vstreet(60, 19, 47, width=5)   # Mała Italia (Racine) -> Grand
vstreet(60, 55, 86, width=5)
# Śródmieście (Loop): przedłużenie ulicy głównej na wschód od Halsted, aż do Michigan Avenue
for x in range(89, 129):
    for y in range(40, 61):
        city[y][x] = 'H'
    for k in range(7):
        y = 48 + k
        ch = ',' if k in (0, 1, 5, 6) else '.'
        if k == 3 and x % 3 != 0: ch = '-'
        city[y][x] = ch
# Michigan Avenue (pionowa, nad jeziorem) i Grant Park
vstreet(129, 24, 92)
for y in range(8, 102):
    for x in range(136, 141):
        if city[y][x] == '#': city[y][x] = '"'
    for x in range(141, MW):
        city[y][x] = '~'
for y in range(8, 102, 5):
    city[y][138] = 'T'
# Grand Avenue -> doki: jezdnia doków na wierszach 14-15
for x in range(103, 110):
    for y in range(12, 19):
        if city[y][x] == '#': city[y][x] = ','
# Cermak -> Chinatown łączy się przez Halsted (wiersze 88-90)
for x in range(82, 89):
    for y in range(86, 93):
        if city[y][x] == '#': city[y][x] = '.'
# zabudowa: urozmaicone kwartały
mats = 'RRRWWwn#g'
for by in range(0, MH, 6):
    for bx in range(0, MW, 5):
        m_ = random.choice(mats)
        for y in range(by, min(by + 6, MH)):
            for x in range(bx, min(bx + 5, MW)):
                if city[y][x] == '#': city[y][x] = m_
# granice mapy
for x in range(MW):
    city[0][x] = 'R'; city[MH - 1][x] = 'R'
for y in range(MH):
    city[y][0] = 'R'
# latarnie, drzewa, zaparkowane auta na nowych ulicach
def is_street(x, y): return city[y][x] in ',.-|'
for y in range(1, MH - 1):
    for x in range(1, MW - 1):
        inside = any(ox <= x < ox + w and oy <= y < oy + h for (ox, oy, w, h) in PARTS.values())
        if inside: continue
        c = city[y][x]
        if c == ',' and (x * 7 + y * 3) % 23 == 0 and not any(city[y + dy][x + dx] in 'LT' for dx in (-1, 0, 1) for dy in (-1, 0, 1)):
            # latarnia tylko na zewnętrznym chodniku (sąsiaduje z budynkiem)
            if any(city[y + dy][x + dx] not in ',.-|LT"~' for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                city[y][x] = 'L'
        elif c == '.' and (x * 13 + y * 5) % 41 == 0:
            # auto przy krawężniku
            if any(city[y + dy][x + dx] == ',' for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                city[y][x] = 'c'

REG = []   # (id, x, y, w, h, nazwa, muzyka, dzielnica, ambient, deszcz)
def region(rid, x, y, w, h, name, music, district, ambient='street', rain=0, entry=None):
    REG.append((rid, x, y, w, h, name, music, district, ambient, rain, entry))
# kolejność: najpierw dokładne
region('polonia', 12, 46, 34, 11, 'Polonia — Back of the Yards', 'polonia', 2, rain=1, entry=(31, 5, 'left'))
region('italia', 46, 46, 36, 11, 'Mała Italia', 'italia', 1, entry=(26, 6, 'left'))
region('doki', 110, 8, 30, 14, 'Doki nad rzeką', 'doki', 3, 'harbor', rain=1, entry=(1, 6, 'right'))
region('chinatown', 89, 84, 30, 11, 'Chinatown', 'chiny', 5, entry=(2, 5, 'right'))
region('loop', 89, 40, 40, 21, 'Śródmieście — Loop', 'noc', 4, entry=(4, 10, 'right'))
region('northside', 3, 8, 107, 12, 'North Side — Grand Avenue', 'noc', 6, entry=(10, 6, 'right'))
region('levee', 3, 84, 86, 10, 'Levee — Cermak Road', 'akcja', 8, entry=(50, 5, 'right'))
region('cicero', 0, 12, 12, 82, 'Ashland Avenue (do Cicero)', 'noc', 7, entry=(6, 40, 'down'))
region('halsted', 82, 12, 7, 82, 'Halsted Street', 'noc', 0, entry=(3, 40, 'down'))
region('michigan', 129, 22, 21, 80, 'Michigan Avenue nad jeziorem', 'noc', 0, entry=(3, 40, 'down'))

# Chinatown -> Michigan Avenue
for y in range(86, 93):
    city[y][118] = ',' if y in (86, 87, 91, 92) else ('.' if y != 89 else '-')
hstreet(86, 119, 131)
# browar Russo: drzwi od Cermak Road (Levee)
city[85][60] = 'D'
BROWAR_DOOR = (60, 85)

# --- przeniesienie encji wklejonych dzielnic
def tr(mapname, x, y):
    if mapname in PARTS:
        ox, oy = PARTS[mapname][0], PARTS[mapname][1]
        return 'miasto', x + ox, y + oy
    return mapname, x, y

city_ents = []
for name in PARTS:
    m = [mm for mm in maps if mm[0] == name][0]
    ox, oy = PARTS[name][0], PARTS[name][1]
    for e in m[3]:
        t = e.split()
        if t[0] == 'warp':
            tgt = t[3]
            if tgt in PARTS:
                continue  # przejścia między ulicami nie są już potrzebne
            t[1] = str(int(t[1]) + ox); t[2] = str(int(t[2]) + oy)
            if tgt == 'browar':
                pass
        elif t[0] == 'npc' or t[0] == 'obj':
            t[2] = str(int(t[2]) + ox); t[3] = str(int(t[3]) + oy)
        elif t[0] in ('mob', 'event'):
            t[1] = str(int(t[1]) + ox); t[2] = str(int(t[2]) + oy)
        city_ents.append(' '.join(t))
city_ents.append('warp %d %d browar 13 14 up' % BROWAR_DOOR)

# --- wyjścia z wnętrz prowadzą do miasta
newmaps = []
for (mid, props, rows, ents) in maps:
    if mid in PARTS:
        continue
    ne = []
    for e in ents:
        t = e.split()
        if t[0] == 'warp' and t[3] in PARTS:
            nm, nx, ny = tr(t[3], int(t[4]), int(t[5]))
            t[3] = nm; t[4] = str(nx); t[5] = str(ny)
        if t[0] == 'warp' and mid == 'browar' and t[3] == 'miasto':
            t[4] = str(BROWAR_DOOR[0]); t[5] = str(BROWAR_DOOR[1] + 2)
        ne.append(' '.join(t))
    newmaps.append((mid, props, rows, ne))
maps[:] = newmaps
props = ['name "Chicago"', 'music italia', 'night 120', 'rain 1', 'floors 3', 'ambient street']
for (rid, x, y, w, h, name, music, district, amb, rain, entry) in REG:
    ex, ey, ed = entry if entry else (w // 2, h // 2, 'down')
    props.append('region %s %d %d %d %d "%s" %s %d %s %d %d %s' % (rid, x, y, w, h, name, music, district, amb, x + ex, y + ey, ed))
maps.insert(1, ('miasto', props, [''.join(r) for r in city], city_ents))

# ------------------------------------------------------------------ zapis
with open(OUT, 'w') as f:
    f.write('# Mapy 3D — wygenerowane przez tools/maps.py, legenda kafli w src/world3d.c\n\n')
    for mid, props, rows, ents in maps:
        f.write('@map %s\n' % mid)
        for p in props:
            f.write(p + '\n')
        f.write('tiles\n')
        for r in rows:
            f.write(r + '\n')
        f.write('endtiles\n')
        for e in ents:
            f.write(e + '\n')
        f.write('\n')
print('map:', len(maps))
