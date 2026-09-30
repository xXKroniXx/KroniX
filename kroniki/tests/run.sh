#!/bin/sh
# Testy fabuły: walidacja + każda ścieżka przechodzona N razy przez automatycznego gracza.
cd "$(dirname "$0")/.."
N=${1:-10}
./build.sh linux >/dev/null || exit 1
./build/kronix_test --validate || exit 1
for t in tests/sciezka_*.txt; do
  ok=0
  for i in $(seq 1 $N); do
    if ./build/kronix_test --seed $i --script "$t" 2>/dev/null | grep -q "OK   _UKONCZONO"; then ok=$((ok + 1)); fi
  done
  echo "$(basename "$t" .txt): ukończono $ok/$N"
done
