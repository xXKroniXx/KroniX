# KroniX: Rodzina

Gra 3D z widokiem z pierwszej osoby (FPP) + **tycoon mafii**. Chicago, 1932–1934.
Własny silnik w czystym C (programowy rasteryzer 3D, bez żadnych bibliotek), jeden plik `.exe` na Windows.

## Jak uruchomić

1. Pobierz `KroniX_Rodzina.exe` (ten folder) na komputer z Windows.
2. Kliknij dwa razy. Jeśli Windows SmartScreen zapyta — „Więcej informacji” → „Uruchom mimo to”.
3. Zapis gry trafia do pliku `kronix_rodzina_zapis.txt` obok .exe.

## Fabuła (prolog w 3D)

Jesteś Tomkiem Kowalskim, chłopakiem z Polonii. Stajesz w obronie piekarza nękanego przez ludzi Russo
i zwracasz na siebie uwagę Dona Salvatore Marino. Wykonujesz dla niego misje:

* haracz w Małej Italii (każdy sklepikarz to inny wybór),
* transport whisky w dokach i zasadzka Irlandczyków (oszczędzisz rannego chłopaka?),
* szczur w rodzinie (zabić, puścić, czy zrobić z niego podwójnego agenta?),
* negocjacje z Triadą w Chinatown,
* nocny atak na browar Russo z Thompsonem w rękach,
* urodziny Dona… i strzelanina, po której nic nie jest takie samo.

Twoje decyzje budują **zaufanie Dona** i stosunki z innymi rodzinami. Od nich zależy, czy Rada wybierze
od razu ciebie, czy najpierw Vito — i z jakimi przyjaciółmi i wrogami zaczniesz rządzić.

## Tycoon: Księga Rodziny (`Tab`)

Po przejęciu rodziny gra zamienia się w strategię — dzień po dniu (`N` kończy dzień):

* **Miasto** — 8 dzielnic Chicago, wpływy każdej rodziny, ochrona dzielnic, dojazd do dzielnic w 3D.
* **Biznesy** — meliny, kasyna, bukmacherzy, kluby jazzowe, browary, bimbrownie, port przemytniczy,
  pralnie pieniędzy, restauracje, lombardy, hale bokserskie. Rozbudowa do 3 poziomów, zarządcy.
  Łańcuch alkoholu: browary produkują beczki, meliny i kluby je zużywają.
* **Ludzie** — rekruci z cechami (walka, spryt, lojalność), poziomy, awans na capo, pensje,
  strażnicy dzielnic, zarządcy lokali, żołnierze. Nielojalni ludzie dezerterują albo sypią policji.
* **Akcje** — haracz, przemyt, napady (ciężarówka, pociąg, bank), sabotaż, atak na dzielnicę,
  łapówki, zamach na bossa, werbunek. Wysyłasz ekipę albo **prowadzisz akcję osobiście w 3D**.
* **Rodziny** — Russo, Irlandczycy, Triada i Syndykat Kane'a mają własną AI: zarabiają, werbują,
  atakują ciebie i siebie nawzajem. Dyplomacja: prezenty, rozejm, sojusz, trybut (lennik),
  przyłączenie rodziny, wojna.
* **Policja i FBI** — gorączka rośnie z każdą akcją; naloty, aresztowania, a przy wysokiej
  gorączce federalne śledztwo podatkowe (jak u Capone). 100% śledztwa = przegrana.
* Losowe wydarzenia z wyborami: kapitan policji, dziennikarz, szczur, wesele u rywala, wybory radnego,
  transport z Kanady… i **koniec prohibicji 5 grudnia 1933**, który zmienia ekonomię.

Wygrywasz, gdy każda rodzina jest rozbita, przyłączona, jest twoim lennikiem albo sojusznikiem,
a ty kontrolujesz co najmniej 5 dzielnic. Zakończenie zależy od tego, czy zdobyłeś miasto siłą, czy słowem.

## Sterowanie

| Klawisz | Akcja |
|---|---|
| W A S D | ruch (A/D — krok w bok) |
| mysz / ← → | rozglądanie się / obrót |
| LPM / Ctrl | strzał (Thompson — przytrzymaj) |
| R | przeładowanie |
| 1–4, kółko myszy, Q | zmiana broni |
| E / Enter / Spacja | rozmowa, drzwi, wybór |
| Shift | bieg |
| Tab | Księga Rodziny (po przejęciu rodziny) |
| N | koniec dnia (w Księdze) |
| Esc / PPM | menu / wstecz |
| F11 / Alt+Enter | pełny ekran |

Pad Xbox: lewa gałka — ruch, prawa — rozglądanie, RT — strzał, A — akcja, B — wstecz, X — przeładowanie.

## Budowanie (Linux)

```sh
./build.sh          # build/kronix_test (Linux, testy) + KroniX_Rodzina.exe (mingw-w64)
sh tests/run.sh     # walidacja fabuły, obie ścieżki prologu, symulacja tycoona
python3 tools/maps.py   # regeneracja data/10_mapy.txt
./build/kronix_test --simtycoon 200   # symulacja 200 dni imperium z prostą AI gracza
```

Struktura: `src/r3d.c` (rasteryzer), `src/textures.c`, `src/actors.c` (proceduralne postacie),
`src/world3d.c` (mapy → geometria 3D, ruch, kolizje), `src/combat.c` (FPS, AI wrogów),
`src/tycoon.c` (Księga Rodziny), `src/script.c` + `src/story.c` (język fabuły), `data/*.txt` (fabuła i mapy).
