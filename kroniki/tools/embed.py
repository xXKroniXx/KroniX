"""Zamienia story.txt na plik C z tablicą znaków (fabuła wbudowana w .exe)."""
import sys
data = open(sys.argv[1], 'rb').read()
with open(sys.argv[2], 'w') as f:
    f.write('/* Wygenerowane automatycznie z data/story.txt — nie edytować. */\n')
    f.write('const char STORY_TXT[] = {\n')
    for i in range(0, len(data), 24):
        f.write(''.join('%d,' % b for b in data[i:i + 24]) + '\n')
    f.write('0};\n')
