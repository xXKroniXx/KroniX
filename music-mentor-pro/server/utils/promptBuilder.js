const levelInstructions = {
  1: 'Bardzo proste rymy, krótkie zdania i podstawowe skojarzenia.',
  2: 'Umiarkowanie proste rymy z pierwszymi metaforami i lekkim flow.',
  3: 'Dobrze wyważone rymy, widoczne metafory, pewne punchline’y i spójny storytelling.',
  4: 'Zaawansowane schematy rymów z podwójnymi znaczeniami i wyraźnym flow.',
  5: 'Najwyższa złożoność: techniczne rymy, mocne punchline’y, wielowarstwowe metafory i dopracowany storytelling.'
};

const bpmRanges = {
  Trap: '130-150 BPM',
  Drill: '140-150 BPM',
  'Pop Rap': '100-120 BPM',
  'Cloud Rap': '120-140 BPM',
  'New School': '120-145 BPM',
  'TikTok Anthem': '95-115 BPM'
};

export function buildPrompt({ level, vibe, style, mode, isExplicit }) {
  const modeInstruction =
    mode === 'hit'
      ? 'Tryb HIT MODE: chwytliwe frazy, prostsze rymy, refrenowy vibe, powtarzalne motywy, duży potencjał na TikTok hook.'
      : 'Tryb UNDERGROUND MODE: surowe brzmienie, techniczne rymy, wielokrotne rymy wewnętrzne, mniej komercyjny, cięższy klimat.';

  const explicitInstruction = isExplicit
    ? 'Tryb 18+ aktywny: możesz używać wulgaryzmów i bardziej bezpośredniego języka.'
    : 'Tryb 18+ wyłączony: unikaj przekleństw i zachowaj czysty język.';

  return `Jesteś ekspertem ghostwriterem rapowym i trenerem lirycznym.
Pisz wyłącznie po polsku.
Nie używaj emoji.
Nie dodawaj komentarzy.
Zachowaj spójność tematyczną.

Dane wejściowe:
- Poziom: ${level}
- Vibe: ${vibe}
- Styl: ${style}
- Tryb: ${mode}
- Zakres BPM dla stylu: ${bpmRanges[style] || '120-140 BPM'}

Zasady poziomu:
${levelInstructions[level] || levelInstructions[3]}

${modeInstruction}
${explicitInstruction}

Wygeneruj odpowiedź DOKŁADNIE w tym formacie:
Tytuł: ...
Wers 1
Wers 2
Wers 3
Wers 4
Wers 5
Analiza rymów: ...

Sugerowane BPM: ...`;
}
