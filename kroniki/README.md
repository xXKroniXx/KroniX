# KroniX: Prohibicja — Chicago, 1931

Fabularna gra RPG w klimacie mafijnym, z **własnym silnikiem** napisanym od zera w C.
Jeden plik `KroniX_Prohibicja.exe` (ok. 300 KB) — bez instalacji, bez bibliotek, bez internetu.

> Październik 1931. Prohibicja, kryzys, deszcz. Tomek Kroniewski, polski doker z Milwaukee Avenue,
> budzi się rano i dowiaduje, że jego młodszy brat Janek nie wrócił na noc. Janek widział w dokach
> coś, czego nie powinien — a teraz szukają go irlandzki gang O'Malleya, włoska rodzina Moretti
> i skorumpowany kapitan policji.

## Jak uruchomić (Windows)

1. Pobierz plik **`KroniX_Prohibicja.exe`** z tego folderu (na GitHubie: kliknij plik → przycisk *Download raw file*).
2. Kliknij go dwukrotnie.
3. Jeśli Windows pokaże niebieskie okno „System Windows ochronił ten komputer” — to normalne dla
   programów bez płatnego certyfikatu. Kliknij **„Więcej informacji” → „Uruchom mimo to”**.

Zapis gry (`kronix_prohibicja_zapis.txt`) tworzy się obok pliku `.exe`.

## Sterowanie

| Klawisz | Akcja |
|---|---|
| Strzałki / WASD | ruch |
| Enter / Spacja / E | rozmowa, akcja, wybór |
| Esc / Backspace | menu (ekwipunek, dziennik, zapis), powrót |
| Shift | bieg |
| F11 / Alt+Enter | pełny ekran |

Działa też pad Xbox (XInput): gałka/krzyżak, A — akcja, B/Start — menu, X/spust — bieg.

## Co jest w grze

- **15 lokacji**: mieszkanie Kroniewskich, polska dzielnica, bar z nielegalnym zapleczem, piekarnia,
  lombard, kościół, sala bokserska, zaułek, atelier fotografa, Mała Italia, restauracja Dona,
  doki, magazyn nr 7, posterunek policji i browar O'Malleya.
- **Ponad 30 postaci do rozmowy** — każda ma coś do powiedzenia, a wiele z nich zmienia zdanie
  w zależności od tego, co zrobiłeś.
- **Wybory w dialogach**: przekupstwo (`[30$]`), zastraszanie (`[Szacunek 4]`), lojalność.
  Opcje, na które cię nie stać, są widoczne, ale wyszarzone.
- **Reputacja**: Szacunek, Honor, Rodzina Moretti, Zaufanie detektywa Walsha.
- **Walka turowa**: atak, sztuczki (Hak, Opatrunek, Zastraszenie, Seria z Thompsona), przedmioty,
  garda, ucieczka. Awanse poziomów, broń (od kastetu po Thompsona) i ubrania.
- **Zadania poboczne**: skradziony kielich, aparat fotografa, walka bokserska, zamordowany związkowiec…
- **Muzyka chiptune** generowana na żywo: polka na polskiej dzielnicy, blues w barze, walc u Włochów.

### Decyzje, które zmieniają fabułę

- **Piekarnia** — obronić piekarza, zapłacić za niego albo wziąć działkę od gangsterów.
- **Ring bokserski** — wygrać uczciwie albo położyć się na zlecenie Vita za 250$.
- **Bar Zośki** — gdy gangsterzy go podpalają, ratujesz Zośkę albo biegniesz dalej po brata.
- **Czarna księga O'Malleya** — najważniejszy wybór. Oddasz ją:
  - **Donowi Moretti** → zdemaskujesz zdrajcę, dostaniesz Thompsona i poparcie mafii,
  - **detektywowi Walshowi** → prawo, sąd i nalot policji,
  - **Vitowi Russo** → spisek przeciwko Donowi,
  - **O'Malleyowi** w zamian za brata → … przekonaj się sam.
- **Kapitan Doyle** — po walce proponuje 500$ za puszczenie go wolno. To kosztuje.
- **Finał** — oferta O'Malleya i los pokonanego bossa.

**Zakończenia (5):** Świadek · Polski Książę · Nowy Don · Dom · Cementowe buty.
Przed napisami epilog pokazuje, co stało się z ludźmi, którym pomogłeś (albo nie).

## Silnik KroniX (dla ciekawych)

Wszystko jest napisane od zera w C99 — bez Unity, SDL czy innych bibliotek:

| Plik | Co robi |
|---|---|
| `src/gfx.c` | programowy renderer 384×216, skalowany do okna |
| `src/font.c` | ręcznie zaprojektowany font pikselowy z polskimi znakami, zawijanie tekstu, kolorowanie słów |
| `src/art.c` | grafika w kodzie: szablony postaci + nakładki (fedora, kaszkiet, czapka policyjna, wąsy…), kafelki generowane proceduralnie |
| `src/audio.c` | syntezator (fale prostokątne, trójkąt, szum) + odtwarzacz nut MML ze swingiem |
| `src/story.c` | parser języka fabuły + walidacja (nieznane etykiety, przedmioty, mapy, sprite'y) |
| `src/script.c` | maszyna wirtualna skryptów: dialogi, wybory z warunkami, walki, zadania, sklepy |
| `src/world.c` | mapa, ruch, NPC, gangsterzy na ulicach, drzwi, noc i latarnie, deszcz |
| `src/battle.c` | walka turowa |
| `src/ui.c` | menu, ekwipunek, dziennik, sklep, ekran tytułowy, zakończenia |
| `src/plat_win32.c` | okno Windows, klawiatura, pad, dźwięk, pełny ekran |

Cała fabuła siedzi w zwykłych plikach tekstowych w `data/` (mapy jako ASCII, dialogi jako skrypty),
np.:

```
@script s_olek
as olek
"Zaplecze zamknięte. Remont."
menu
 opt "Biały orzeł nie śpi." haslo if HASLO
 opt "Masz tu 20$ na remont." kasa ifgold 20
 opt "Zejdź mi z drogi, Olek. [Szacunek 2]" zastrasz ifge SZACUNEK 2
endmenu
```

Fabułę można zmieniać bez kompilacji: `KroniX_Prohibicja.exe --story moja_fabula.txt`.

### Budowanie

Na Linuksie z `mingw-w64` i `python3`:

```
./build.sh          # build/KroniX_Prohibicja.exe + build/kronix_test (Linux)
./tests/run.sh 20   # walidacja fabuły + 5 ścieżek przechodzonych automatycznie po 20 razy
```
