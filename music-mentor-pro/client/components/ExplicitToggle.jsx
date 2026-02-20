export default function ExplicitToggle({ enabled, onToggle }) {
  return (
    <label className="flex items-center justify-between rounded-xl border border-white/10 bg-white/5 p-3">
      <span className="text-sm font-medium">🔘 Tryb 18+</span>
      <button
        type="button"
        role="switch"
        aria-checked={enabled}
        onClick={onToggle}
        className={`relative h-7 w-14 rounded-full transition ${enabled ? 'bg-explicitAccent' : 'bg-zinc-700'}`}
      >
        <span
          className={`absolute top-1 h-5 w-5 rounded-full bg-white transition ${enabled ? 'left-8' : 'left-1'}`}
        />
      </button>
    </label>
  );
}
