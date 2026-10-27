"""Zamienia story.txt na plik C z tablicą znaków (fabuła wbudowana w .exe)."""
import glob
import sys
out = sys.argv[1]
# wzorce typu data/*.txt rozwijamy sami (wiersz poleceń Windows tego nie robi)
files = []
for a in sys.argv[2:]:
    files += sorted(glob.glob(a)) if any(ch in a for ch in '*?[') else [a]
# znacznik pliku pozwala grze podać przy błędzie nazwę pliku i numer linii
data = b''.join(b'#@file ' + p.replace('\\', '/').encode() + b'\n' + open(p, 'rb').read() + b'\n' for p in files)
open(out.replace('.c', '.txt'), 'wb').write(data)
with open(out, 'w') as f:
    f.write('/* Wygenerowane automatycznie z data/story.txt — nie edytować. */\n')
    f.write('const char STORY_TXT[] = {\n')
    for i in range(0, len(data), 24):
        f.write(''.join('%d,' % b for b in data[i:i + 24]) + '\n')
    f.write('0};\n')
