#!/bin/sh
# Testy: walidacja fabuły, obie ścieżki prologu (N losowań) i symulacja tycoona.
cd "$(dirname "$0")/.."
N=${1:-5}
./build.sh linux >/dev/null || exit 1
TMP=$(mktemp -d)
cp build/kronix_test "$TMP/"
cd "$TMP" || exit 1
./kronix_test --validate || exit 1
for t in "$OLDPWD"/tests/sciezka_*.txt; do
  ok=0
  for i in $(seq 1 $N); do
    rm -f kronix_rodzina_zapis.txt
    out=$(./kronix_test --seed $i --script "$t" 2>&1)
    if echo "$out" | grep -q "OK   _UKONCZONO" && ! echo "$out" | grep -q "BŁĄD"; then ok=$((ok + 1)); else echo "$out" | grep -E "BŁĄD|UWAGA" | head -5; fi
  done
  echo "$(basename "$t" .txt): ukończono $ok/$N"
done
# kronika: wszystkie sceny okresu Księgi i wizyty w lokalach muszą dojść do końca
{ echo "wait 10"; echo "debug newgame"; echo "wait 10"; echo "autoplay 2"; echo "untilidle"
  for b in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 -1 -2 -3 -4 -11; do echo "debug beat $b"; echo "wait 5"; echo "untilidle"; echo "debug killall"; echo "untilidle"; done
  echo "quit"; } > kronika.txt
ok=0
for i in $(seq 1 $N); do
  out=$(./kronix_test --seed $i --script kronika.txt --frames 100000 2>&1)
  if echo "$out" | grep -q "beat -11" && ! echo "$out" | grep -q "BŁĄD"; then ok=$((ok + 1)); fi
done
echo "kronika: ukończono $ok/$N"
for s in 1 2 3; do ./kronix_test --seed $s --simtycoon 300 2>/dev/null | grep "^koniec"; done
rm -rf "$TMP"
