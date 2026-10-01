# KroniX Engine — historia zmian

Silnik rośnie razem z grami. Każda gra dopisuje tu, co wniosła do wspólnego kodu.

## 1.2.0 — pojazdy i miasto z detalami
* `mb_arch_z()` — łuk wokół osi Z (błotniki, sklepienia, arkady, mosty).
* `mb_beam()` — belka o przekroju kwadratowym między dwoma dowolnymi punktami
  (zastrzały, kratownice, rusztowania, dźwigary) — koniec ze „schodkami” z prostopadłościanów.
* Wskazówki materiałowe dla gier: kolor lakieru to kolor wierzchołka mnożony przez jasną
  warstwę tekstury z wysokim połyskiem w alfie (w „Rodzinie”: `L_PAINT`, `L_CHROME`, `L_TIRE`);
  ciemne tekstury metalu zostawiamy dla stali surowej.
* Wniesione przez „KroniX: Rodzina”: 8 modeli aut z własnym prowadzeniem, przechodnie na
  chodnikach, kolejka nadziemna z ruchomym pociągiem, wieżowce z uskokami, neony na dachach
  (kod w grze — kandydaci do przeniesienia do silnika przy następnej grze).

## 1.1.0 — grafika „mokrego miasta”
* Odbicia ekranowe (SSR) na mokrych i wodnych powierzchniach: odbijalność zapisywana w kanale
  alfa bufora HDR, marsz promienia po buforze głębi, kręgi od kropel deszczu zaburzające odbicie.
* Kałuże: proceduralna maska w shaderze świata (ciemniejsze, bardziej lustrzane plamy).
* Cienie kontaktowe SSAO (10 próbek, promień w jednostkach świata).
* Bufor głębi jako tekstura (rozwiązywany z MSAA); broń/dłonie przez `glDepthRange` zamiast
  kasowania głębi.
* `r_cone()` — objętościowe stożki światła (latarnie, reflektory).
* `r_puff()` — miękkie kłęby pary i dymu z animowanym szumem.
* `g_cfg.ssr`, `g_cfg.ssao` — przełączniki jakości.

## 1.0.0 — wydzielenie z „KroniX: Rodzina”
* **gfx**: renderer OpenGL 3.3 core z własnym loaderem (bez bibliotek), HDR + MSAA + bloom,
  ACES, korekcja koloru, winieta, ziarno; oświetlenie per piksel (do 16 świateł punktowych),
  półsferyczny ambient, księżyc, mgła wykładnicza, mokre powierzchnie, woda, ogień, liście;
  szkielety (do 24 kości), poświaty addytywne, deszcz na GPU, portrety do tekstur, zrzuty BMP.
* **gfx/mesh**: matematyka 3D, budowniczy siatek (prostopadłościany fazowane, walce, sfery,
  czworokąty z UV świata), tablica tekstur 512x512.
* **gfx/kx_proc**: proceduralne tekstury — szum value/fBm/Worley (zawijany), relief, ziarno,
  napisy z atlasu SDF, składanie warstw dla GPU.
* **gfx/ui2d**: UI w wirtualnej rozdzielczości 1280x720, czcionki SDF (polskie znaki),
  zaokrąglone panele, cienie, gradienty, przycinanie, kody kolorów w tekście.
* **audio**: syntezator MML (fortepian, kontrabas, trąbka, saksofon, perkusja, smyczki,
  akordeon, szarpane, organy, wibrafon, sekcja dęta), akordy, swing, pogłos Freeverb,
  trzaski płyty; efekty syntetyzowane do buforów z wariantami; tło dźwiękowe miejsc
  (deszcz, ulica, port, gwar, biuro, kościół, hala); głos silnika pojazdu. Stereo 44,1 kHz.
* **platform**: Windows (okno, WGL 3.3, raw input, XInput, waveOut, pełny ekran, stały krok
  60 Hz + interpolacja, vsync) i Linux bez okna (testy, OSMesa, zrzuty `--shot`, `--wav`).
* **examples/demo**: minimalna gra na silniku.
