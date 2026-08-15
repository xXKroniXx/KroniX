# KroniX: Loch Bez Końca

Proceduralny dungeon crawler RPG na telefon — nieskończone, losowo generowane
poziomy, automatyczna walka, levelowanie postaci i system nagród. Działa
bezpośrednio w przeglądarce mobilnej, bez instalacji ze sklepu — można dodać
jako ikonę na ekranie głównym (PWA).

## Jak grać

- **Joystick** (dotknij i przeciągnij dowolne miejsce ekranu) — porusza postacią.
- Postać **atakuje automatycznie** najbliższego wroga w zasięgu — nie musisz celować.
- **UNIK** (prawy dolny róg) — krótki sprint z chwilową niewrażliwością na obrażenia, odnawia się co 3s.
- Znajdź świecące **wyjście**, by zejść na kolejne, trudniejsze piętro.
- Zbieraj **skrzynie** (złoto, leczenie, artefakty) i pokonuj wrogów, by zdobywać XP.
- Co 5. piętro to **piętro Bossa** — silny unikalny przeciwnik i gwarantowana skrzynia z nagrodą.
- Po awansie poziomu wybierasz **1 z 3 nagród** (perków) — bezpośrednio wpływają na styl gry.
- Złoto jest trwałe — po śmierci wracasz do Bazy i wydajesz je na **stałe ulepszenia** postaci.
- Sterowanie na komputerze (do testów): WASD/strzałki + Spacja (unik), Esc (pauza).

## System proceduralnych poziomów

Każde piętro generowane jest algorytmem **rekurencyjnego podziału przestrzeni
(BSP)**: loch dzielony jest losowo na coraz mniejsze fragmenty, w liściach
podziału powstają pokoje, które łączone są korytarzami wzdłuż drzewa podziału.
Start i wyjście wybierane są na podstawie przeszukiwania grafu (BFS) — wyjście
to zawsze pokój najdalszy od startu. Trudność (rozmiar lochu, liczba i siła
wrogów, szansa na elitarnych przeciwników) skaluje się wraz z numerem piętra,
a co 5. piętro generowana jest specjalna, mniejsza arena bossa. Zobacz
`js/dungeon.js`.

## Struktura projektu

```
index.html          punkt wejścia, HUD i ekrany (baza, awans, pauza, koniec gry)
css/style.css        cały styl, mobile-first, motywy jasny/ciemny, safe-area
js/dungeon.js         generator poziomów (BSP)
js/entities.js        gracz, przeciwnicy, skrzynie, pociski + skalowanie trudności
js/perks.js           nagrody za awans postaci + trwałe ulepszenia w Bazie
js/save.js             zapis postępu w localStorage
js/input.js            wirtualny joystick + przycisk uniku + klawiatura
js/game.js             pętla gry, walka, kamera, rendering canvas, UI
manifest.json, sw.js   konfiguracja PWA (instalacja, działanie offline)
icons/                 ikony aplikacji
```

Czysty HTML/CSS/JS (Canvas 2D), zero zależności i kroku budowania — otwiera
się natychmiast w dowolnej nowoczesnej przeglądarce.

## Uruchomienie na telefonie

Najwygodniej przez GitHub Pages (nic nie trzeba instalować):

1. W ustawieniach repozytorium na GitHub: **Settings → Pages → Source: branch
   z tym kodem, folder `/ (root)`**.
2. Po chwili GitHub poda adres w stylu `https://<użytkownik>.github.io/<repo>/`.
3. Otwórz ten adres w przeglądarce na telefonie.
4. W Chrome/Safari wybierz **„Dodaj do ekranu głównego”** — gra zainstaluje
   się jak zwykła aplikacja (pełny ekran, ikona, działa też offline dzięki
   Service Workerowi).

### Lokalnie (do testów na komputerze)

Wymagany jest serwer HTTP (Service Worker i moduły nie działają z `file://`):

```bash
python3 -m http.server 8000
# otwórz http://localhost:8000
```

Aby przetestować na telefonie w tej samej sieci Wi-Fi, użyj adresu IP
komputera zamiast `localhost`, np. `http://192.168.1.23:8000`.
