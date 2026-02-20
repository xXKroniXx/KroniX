const labels = {
  1: 'Dopiero zaczynam',
  2: 'Coś niecoś potrafię',
  3: 'Kumam o co chodzi',
  4: 'Siedzę w tym długo (+2)',
  5: 'Robię swoje (+5)'
};

export default function LevelSlider({ level, onChange }) {
  return (
    <div className="space-y-2">
      <label className="text-sm font-semibold">🎚 Poziom: {level}</label>
      <input
        type="range"
        min="1"
        max="5"
        value={level}
        onChange={(e) => onChange(Number(e.target.value))}
        className="w-full accent-accent"
      />
      <p className="text-xs text-zinc-300">{labels[level]}</p>
    </div>
  );
}
