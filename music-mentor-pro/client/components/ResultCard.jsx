import { motion } from 'framer-motion';

function parseResult(text) {
  const lines = text?.split('\n').map((line) => line.trim()).filter(Boolean) || [];
  const title = lines.find((line) => line.startsWith('Tytuł:'))?.replace('Tytuł:', '').trim() || 'Brak tytułu';
  const analysisLine = lines.find((line) => line.startsWith('Analiza rymów:')) || '';
  const bpmLine = lines.find((line) => line.startsWith('Sugerowane BPM:')) || '';

  const verses = lines.filter(
    (line) => !line.startsWith('Tytuł:') && !line.startsWith('Analiza rymów:') && !line.startsWith('Sugerowane BPM:')
  ).slice(0, 5);

  return {
    title,
    verses,
    analysis: analysisLine.replace('Analiza rymów:', '').trim(),
    bpm: bpmLine.replace('Sugerowane BPM:', '').trim()
  };
}

export default function ResultCard({ result }) {
  const { title, verses, analysis, bpm } = parseResult(result);

  return (
    <motion.article
      initial={{ opacity: 0, y: 16 }}
      animate={{ opacity: 1, y: 0 }}
      className="glass-panel relative mt-6 rounded-xl p-5"
    >
      <span className="absolute left-0 top-0 h-full w-1 rounded-l-xl bg-accent" />
      <h2 className="mb-4 pl-3 text-2xl font-bold tracking-wide">{title}</h2>
      <div className="space-y-2 pl-3 text-zinc-100">
        {verses.map((verse, idx) => (
          <p key={`${verse}-${idx}`} className="leading-relaxed">{verse}</p>
        ))}
      </div>
      <div className="mt-5 space-y-3 pl-3">
        <p className="text-sm"><strong>🎼 Analiza rymów:</strong> {analysis}</p>
        <p className="text-sm"><strong>🥁 BPM:</strong> {bpm}</p>
      </div>
    </motion.article>
  );
}
