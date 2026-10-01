# KroniX Engine

Wspólny silnik gier KroniX — czyste C99, OpenGL 3.3, zero zewnętrznych bibliotek.
Jedna gra = jeden plik `.exe` na Windows. Silnik rozwijamy z każdą kolejną grą:
to, co nowa gra wymyśli i co da się użyć ponownie, trafia tutaj (patrz `CHANGELOG.md`).

Gry na silniku: **KroniX: Rodzina** (`../rodzina`) — FPP + tycoon mafii, Chicago 1932–34.

## Moduły

| Katalog | Co zawiera | Nagłówek |
|---|---|---|
| `kx.h`, `core/` | kontrakt platforma ↔ gra, akcje wejścia, stan myszy, `Input`, UTF-8 | `kx.h` |
| `gfx/` | renderer (HDR, bloom, MSAA, światła, mgła, deszcz, poświaty, szkielety), matematyka, siatki | `render.h` |
| `gfx/kx_proc.*` | proceduralne tekstury: szum, relief, napisy, tablica warstw | `kx_proc.h` |
| `gfx/ui2d.c`, `font_data.*` | UI 2D, czcionki SDF z polskimi znakami | `render.h` (`d2_*`) |
| `audio/` | syntezator muzyki (MML), efekty, tło dźwiękowe, pogłos, silnik auta | `kx_audio.h` |
| `platform/` | `win32.c` (gra), `headless.c` (testy na Linuksie, OSMesa) | — |
| `tools/` | `make_fonts.py` — generator atlasu czcionek SDF | — |
| `examples/demo/` | minimalna gra: od tego zaczynasz nowy projekt | — |

## Nowa gra w 5 krokach

1. Skopiuj `examples/demo` do nowego katalogu obok `engine/` (np. `../mojagra`).
2. W `build.sh` ustaw `KX_ROOT=../engine` i dołącz `. "$KX_ROOT/kx_sources.sh"`.
3. Zaimplementuj w grze cztery funkcje z `kx.h`:
   * `kx_game_init(argc, argv)` — tekstury (`kxp_*`), modele (`mb_*` + `gm_upload`), dźwięk,
   * `kx_game_frame()` — logika w stałym kroku 60 Hz (zacznij od `input_update()`),
   * `kx_game_draw()` — `r_frame_begin` → światła → `r_draw` → `r_frame_end`, potem UI `d2_*`,
   * `kx_game_look()` — obrót kamery myszą w każdej klatce ekranu (płynność przy 144 Hz).
4. Dźwięk: `kx_audio_tracks(utwory)`, `kx_audio_sfx(efekty, generator)`, `audio_init()`.
5. `./build.sh` → `build/demo` (Linux) i `.exe` (Windows, mingw-w64).

## Kontrakt platforma ↔ gra

* Platforma wypełnia `g_btn_raw[]` (stały zestaw akcji `BTN_*`), `g_mouse_dx/dy`, `g_mouse_x/y`
  (w przestrzeni UI 1280×720), woła `kx_game_frame()` 60 razy na sekundę i `kx_game_draw()` raz
  na klatkę ekranu, ustawiając `g_alpha` (położenie klatki między krokami — do interpolacji).
* Gra ustawia `g_want_mouse_capture` (tryb FPP), `g_quit`, `g_fullscreen_toggle`.
* `g_render == 0` oznacza tryb testowy bez obrazu — gra ma wtedy pomijać wywołania GL.

## Renderer w skrócie

```c
RCam cam = {pozycja, yaw, pitch, fov};
REnv env = {niebo, ambient, księżyc, mgła, mokro, deszcz, ...};
r_frame_begin(&cam, &env);
r_light(x, y, z, r, g, b, promień);  ... r_lights_commit();   // wybiera 16 najbliższych
r_draw(&siatka, &macierz, 0);           r_draw_bones(...) dla postaci
r_glow(...); r_glow_flush(); r_rain(0.5f);
r_grade(nasycenie, ciepło, winieta, błysk);
r_frame_end();                          // HDR → bloom → ACES → ekran
d2_begin(); d2_text(FONT_SERIF, 32, x, y, "Tekst", kolor); d2_end();
```

Budowniczy siatek: `mb_box`, `mb_rbox` (fazowany), `mb_cyl`/`mb_cyl_x`/`mb_cyl_z`, `mb_sphere`,
`mb_quad`/`mb_quad_world`, `mb_arch_z` (łuk), `mb_beam` (belka między dwoma punktami),
`mb_append_tf` (wklejanie z macierzą). Kolor i warstwa ustawiane przez `mb_paint(kolor, warstwa)`.

Wierzchołek ma kolor (alfa = emisja), warstwę tekstury, kość i flagi (`VF_ALPHATEST`,
`VF_UNLIT`, `VF_NOFOG`, `VF_WATER`, `VF_FOLIAGE`, `VF_FIRE`). Kanał alfa tekstury = połysk.

## Testy bez okna

```sh
cd examples/demo && ./build.sh
./build/demo --render --shot 90 zrzut.bmp   # klatka 90 do pliku
./build/demo --wav demo 10 muzyka.wav        # 10 s muzyki do WAV
```

## Zasady rozwoju

* Kod silnika nie zna żadnej gry: zero odwołań do fabuły, map czy nazw postaci.
* Nowe API → wpis w `CHANGELOG.md` i podbicie `KX_VERSION` w `kx.h` (+ `VERSION`).
* Zmiana łamiąca zgodność → podbicie wersji głównej; stare gry przypinamy do tagu.
