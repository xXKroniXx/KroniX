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
for s in 1 2 3; do ./kronix_test --seed $s --simtycoon 300 2>/dev/null | grep "^koniec"; done
rm -rf "$TMP"
