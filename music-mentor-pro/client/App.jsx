import { useMemo, useState } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import LevelSlider from './components/LevelSlider';
import ModeSwitch from './components/ModeSwitch';
import ExplicitToggle from './components/ExplicitToggle';
import ResultCard from './components/ResultCard';
import Loader from './components/Loader';

const vibes = ['Glow up / zmiana życia', 'Sigma mindset', 'Melancholia nocna', 'Miłość toksyczna', 'Samotność', 'Motywacja', 'Viral flex', 'After party'];
const styles = ['Trap', 'Drill', 'Pop Rap', 'Cloud Rap', 'New School', 'TikTok Anthem'];
const API_URL = import.meta.env.VITE_API_URL || 'http://localhost:3001/api/generate';

const getHistory = () => {
  const raw = localStorage.getItem('mmp-history');
  return raw ? JSON.parse(raw) : [];
};

export default function App() {
  const [level, setLevel] = useState(3);
  const [vibe, setVibe] = useState(vibes[0]);
  const [style, setStyle] = useState(styles[0]);
  const [mode, setMode] = useState('hit');
  const [isExplicit, setIsExplicit] = useState(false);
  const [loading, setLoading] = useState(false);
  const [result, setResult] = useState('');
  const [history, setHistory] = useState(getHistory);
  const [error, setError] = useState('');

  const containerClass = useMemo(
    () =>
      isExplicit
        ? 'explicit-mode-bg min-h-screen text-white transition-colors duration-700'
        : 'min-h-screen bg-gradient-to-br from-black via-zinc-950 to-slate-900 text-white transition-colors duration-700',
    [isExplicit]
  );

  const persistHistory = (text) => {
    const next = [text, ...history].slice(0, 5);
    setHistory(next);
    localStorage.setItem('mmp-history', JSON.stringify(next));
  };

  const generate = async () => {
    setLoading(true);
    setError('');
    try {
      const response = await fetch(API_URL, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ level, vibe, style, mode, isExplicit })
      });

      const data = await response.json();
      if (!response.ok) throw new Error(data.error || 'Błąd generowania');

      setResult(data.text);
      persistHistory(data.text);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  const copyText = async () => {
    if (!result) return;
    await navigator.clipboard.writeText(result);
  };

  return (
    <main className={containerClass}>
      <div className="mx-auto max-w-3xl px-4 py-8 sm:py-12">
        <motion.h1
          initial={{ opacity: 0, y: -10 }}
          animate={{ opacity: 1, y: 0 }}
          className={`mb-6 text-center text-3xl font-extrabold sm:text-4xl ${isExplicit ? 'glitch-text text-explicitAccent' : 'text-accent'}`}
        >
          🎤 Music Mentor PRO
        </motion.h1>

        <section className="glass-panel animate-glow space-y-4 rounded-2xl p-4 sm:p-6">
          <ExplicitToggle enabled={isExplicit} onToggle={() => setIsExplicit((prev) => !prev)} />
          <LevelSlider level={level} onChange={setLevel} />

          <label className="block text-sm font-semibold">🔥 Vibe</label>
          <select value={vibe} onChange={(e) => setVibe(e.target.value)} className="w-full rounded-lg bg-zinc-900 p-2 text-sm">
            {vibes.map((v) => (
              <option key={v} value={v}>{v}</option>
            ))}
          </select>

          <label className="block text-sm font-semibold">🎵 Styl</label>
          <select value={style} onChange={(e) => setStyle(e.target.value)} className="w-full rounded-lg bg-zinc-900 p-2 text-sm">
            {styles.map((s) => (
              <option key={s} value={s}>{s}</option>
            ))}
          </select>

          <ModeSwitch mode={mode} onChange={setMode} />

          <div className="flex flex-wrap gap-2 pt-2">
            <button onClick={generate} disabled={loading} className="rounded-lg bg-primary px-4 py-2 font-semibold text-black transition hover:brightness-110 disabled:opacity-60">
              Generuj
            </button>
            <button onClick={generate} disabled={loading} className="rounded-lg bg-accent px-4 py-2 font-semibold text-white transition hover:brightness-110 disabled:opacity-60">
              Generuj ponownie
            </button>
            <button onClick={copyText} disabled={!result} className="rounded-lg border border-white/20 px-4 py-2 font-semibold text-white disabled:opacity-50">
              Kopiuj tekst
            </button>
          </div>

          {loading && <Loader />}
          {error && <p className="text-sm text-red-400">{error}</p>}
        </section>

        <AnimatePresence>{result && !loading ? <ResultCard result={result} /> : null}</AnimatePresence>

        {history.length > 0 && (
          <section className="mt-6 rounded-xl border border-white/10 bg-black/20 p-4">
            <h3 className="mb-3 text-sm font-semibold text-zinc-300">Ostatnie 5 wygenerowanych tekstów</h3>
            <div className="space-y-2 text-xs text-zinc-400">
              {history.map((item, idx) => (
                <button key={`${idx}-${item.slice(0, 20)}`} onClick={() => setResult(item)} className="block w-full rounded-md border border-white/10 p-2 text-left hover:bg-white/5">
                  {item.split('\n')[0] || `Tekst ${idx + 1}`}
                </button>
              ))}
            </div>
          </section>
        )}
      </div>
    </main>
  );
}
