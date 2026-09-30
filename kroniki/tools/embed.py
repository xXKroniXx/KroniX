"""Zamienia story.txt na plik C z tablicą znaków (fabuła wbudowana w .exe)."""
import sys
out = sys.argv[1]
data = b''.join(open(p, 'rb').read() + b'\n' for p in sys.argv[2:])
open(out.replace('.c', '.txt'), 'wb').write(data)
with open(out, 'w') as f:
    f.write('/* Wygenerowane automatycznie z data/story.txt — nie edytować. */\n')
    f.write('const char STORY_TXT[] = {\n')
    for i in range(0, len(data), 24):
        f.write(''.join('%d,' % b for b in data[i:i + 24]) + '\n')
    f.write('0};\n')
