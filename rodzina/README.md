# KroniX: Rodzina

Gra 3D z widokiem z pierwszej osoby (FPP) + **tycoon mafii**. Chicago, Illinois, 1932–1934 — prohibicja,
Wielki Kryzys, Wystawa Światowa i koniec ery gangsterów.
Własny silnik w czystym C na **OpenGL 3.3** (bez zewnętrznych bibliotek), jeden plik `.exe` na Windows.

**Styl:** gładki low-poly 3D jak w niezależnych grach o budowaniu biznesu — noc, deszcz, mokry asfalt,
neony, światło latarni, HDR z poświatą (bloom), wygładzanie krawędzi (MSAA), mgła i ziarno filmu.
Wszystkie tekstury, modele, postacie i muzyka są generowane proceduralnie przy starcie.

**Dźwięk:** syntezator małego zespołu jazzowego (fortepian, kontrabas, trąbka z tłumikiem, saksofon,
szczotki, wibrafon, akordeon, organy) w stereo z pogłosem; tło miasta — deszcz, kolejka „L”,
klaksony, gwar w lokalach, zegar w gabinecie, syrena mgłowa w dokach.

## Jak uruchomić

1. Pobierz `KroniX_Rodzina.exe` (ten folder) na komputer z Windows.
2. Kliknij dwa razy. Jeśli Windows SmartScreen zapyta — „Więcej informacji” → „Uruchom mimo to”.
3. Zapis gry trafia do pliku `kronix_rodzina_zapis.txt` obok .exe, ustawienia do `kronix_rodzina_ustawienia.txt`.
4. Wymagana karta graficzna z OpenGL 3.3 (praktycznie każda od 2010 r.). Jakość grafiki
   (Niska / Średnia / Wysoka / Ultra), pole widzenia i czułość myszy zmienisz w Opcjach.

## Otwarte miasto i samochody

Całe Chicago to **jedna duża mapa**: Polonia, Mała Italia, Chinatown, doki, Levee, North Side, wieżowce
Loopu i Michigan Avenue nad jeziorem Michigan połączone alejami (Ashland, Halsted, Grand, Cermak).
Do budynków wchodzisz drzwiami (krótkie przyciemnienie ekranu), ulice przechodzą płynnie jedna w drugą.

* **Znacznik celu** — złoty słup światła, punkt na minimapie i odległość w metrach.
* **Samochody** — podejdź do auta i naciśnij **E**. W/S — gaz/hamulec/wsteczny, A/D — skręt,
  LPM — klakson, mysz — rozglądanie, E (po zatrzymaniu) — wysiądź. Na Racine Street przy trattorii
  stoi Packard Rodziny. Po arteriach jeździ ruch uliczny; auto z ulicy też można „pożyczyć”.

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

### Kronika (czerwiec 1933 – 1934)

Okres Księgi to nie tylko liczby. W wyznaczone dni ktoś puka do drzwi — i scena rozgrywa się w 3D:

* **agent Kessler** z wydziału podatkowego Departamentu Skarbu (ten od Capone) przychodzi do trattorii,
* wieczór z Lucią w klubie jazzowym **Blue Moon** na Rush Street,
* niedzielny obiad u matki w Polonii,
* Bronek, przyjaciel z dzieciństwa, chce zostać capo — a potem okazuje się, co federalni mają na jego brata,
* **zamach** na trattorię podczas kolacji,
* lądowanie eskadry Balbo na jeziorze Michigan i festa na Taylor Street,
* Victor Kane, wybory radnych i kontrakt na **Wystawie Światowej**,
* herbata z Lee Wongiem i propozycja nie do odrzucenia (opium),
* przeszukanie federalnych, Lucia w kościele św. Rocha, pijany Vito… i noc końca prohibicji.

Wybory zmieniają Księgę (szacunek, gorączka, śledztwo, sojusze, wojny) i **zakończenie** (m.in. KOMISJA,
CAPO DI TUTTI CAPI, KALIFORNIA). Co tydzień przychodzi **Chicago Daily Herald** z prawdziwymi nagłówkami
z 1933 roku i wiadomościami o twojej Rodzinie.

**Wizyty w lokalach:** w zakładce Biznesy wybierz lokal → „Odwiedź lokal”. Wchodzisz do klubu jazzowego
(zespół na scenie, tancerze), meliny za zakładem fryzjerskim, kasyna z ruletką, biura bukmachera albo hali
bokserskiej. Kierownik zawsze ma sprawę do załatwienia: dziennikarz, szuler, radny z długami, ustawiona
walka, awanturnik z North Side…

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
# silnik: ../engine (README tam), przykład: ../engine/examples/demo
./build/kronix_test --simtycoon 200   # symulacja 200 dni imperium z prostą AI gracza
```

Struktura: `src/render.c` (OpenGL: oświetlenie, HDR, bloom, deszcz), `src/glapi.*` (własny loader GL),
`src/tex.c` (proceduralne tekstury), `src/models.c` (rekwizyty), `src/human.c` (postacie ze szkieletem),
`src/city.c` (mapy → miasto 3D: fasady, witryny, neony, wnętrza), `src/ui2d.c` (UI z czcionkami SDF),
`src/audio.c` (syntezator jazzowy, efekty, ambient), `src/world3d.c` (ruch, kolizje, NPC),
`src/combat.c` (FPS, AI wrogów), `src/tycoon.c` (Księga Rodziny + kronika), `src/script.c` + `src/story.c`
(język fabuły), `data/*.txt` (fabuła i mapy), `tools/` (generator map i czcionek).

Testy z obrazem (Linux, OSMesa): `./build/kronix_test --render --script plik.txt` — polecenie `shot x.bmp`
zapisuje zrzut ekranu; `--wav utwór 30 x.wav` renderuje muzykę do pliku.
