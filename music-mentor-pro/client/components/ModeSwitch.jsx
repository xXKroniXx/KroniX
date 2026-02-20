export default function ModeSwitch({ mode, onChange }) {
  return (
    <div className="space-y-2">
      <label className="text-sm font-semibold">🎯 Tryb</label>
      <div className="grid grid-cols-2 gap-2">
        <button
          type="button"
          onClick={() => onChange('hit')}
          className={`rounded-lg px-3 py-2 text-sm transition ${
            mode === 'hit' ? 'bg-accent text-white' : 'bg-zinc-800 text-zinc-300'
          }`}
        >
          1️⃣ HIT MODE
        </button>
        <button
          type="button"
          onClick={() => onChange('underground')}
          className={`rounded-lg px-3 py-2 text-sm transition ${
            mode === 'underground' ? 'bg-accent text-white' : 'bg-zinc-800 text-zinc-300'
          }`}
        >
          2️⃣ UNDERGROUND MODE
        </button>
      </div>
    </div>
  );
}
