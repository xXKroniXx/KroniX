/* KroniX: Loch Bez Końca — nagrody: perki za awans postaci + ulepszenia trwałe (hub) */
(function (global) {
  const PERKS = [
    { id: 'ironskin',  name: 'Żelazna Skóra', desc: '+3 Obrony', apply: p => { p.def += 3; } },
    { id: 'fury',      name: 'Furia',         desc: '+4 Ataku', apply: p => { p.atk += 4; } },
    { id: 'vitality',  name: 'Witalność',     desc: '+25 Max HP i pełne leczenie', apply: p => { p.maxHp += 25; p.hp += 25; } },
    { id: 'reflex',    name: 'Refleks',       desc: 'Szybszy atak (-8% odnowienia)', apply: p => { p.atkCooldown = Math.max(0.18, p.atkCooldown * 0.92); } },
    { id: 'greed',     name: 'Chciwość',      desc: '+15% złota ze zdobyczy', apply: p => { p.goldMult += 0.15; } },
    { id: 'vampirism', name: 'Wampiryzm',     desc: '+6% kradzieży życia', apply: p => { p.lifesteal += 0.06; } },
    { id: 'crit',      name: 'Krytyk',        desc: '+8% szansy na trafienie krytyczne', apply: p => { p.critChance = Math.min(0.75, p.critChance + 0.08); } },
    { id: 'swift',     name: 'Szybkie Nogi',  desc: '+10% prędkości ruchu', apply: p => { p.moveMult += 0.10; } },
    { id: 'regen',     name: 'Regeneracja',   desc: '+1 HP odnowienia / sek.', apply: p => { p.regenPerSec = (p.regenPerSec || 0) + 1; } },
    { id: 'reach',     name: 'Zasięg',        desc: '+10 zasięgu ataku', apply: p => { p.atkRange += 10; } },
  ];

  function shuffled(arr) {
    const a = arr.slice();
    for (let i = a.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [a[i], a[j]] = [a[j], a[i]];
    }
    return a;
  }

  function rollPerkChoices(count) {
    return shuffled(PERKS).slice(0, count || 3);
  }

  function applyPerk(player, perkId) {
    const perk = PERKS.find(p => p.id === perkId);
    if (!perk) return;
    perk.apply(player);
    player.perks[perkId] = (player.perks[perkId] || 0) + 1;
  }

  // --- Trwałe ulepszenia w Hubie (kupowane za złoto zebrane w wyprawach) ---
  const META_UPGRADES = {
    hp:  { name: 'Max HP',   desc: '+9 maksymalnego zdrowia', baseCost: 18, growth: 1.32, max: 20 },
    atk: { name: 'Atak',     desc: '+1.6 obrażeń ataku',      baseCost: 22, growth: 1.35, max: 20 },
    def: { name: 'Obrona',   desc: '+1.1 obrony',             baseCost: 20, growth: 1.35, max: 20 },
  };

  function metaUpgradeCost(key, currentLevel) {
    const cfg = META_UPGRADES[key];
    return Math.round(cfg.baseCost * Math.pow(cfg.growth, currentLevel));
  }

  const EXTRA_LIFE_COST = 180;

  global.KXPerks = { PERKS, rollPerkChoices, applyPerk, META_UPGRADES, metaUpgradeCost, EXTRA_LIFE_COST };
})(typeof window !== 'undefined' ? window : globalThis);

if (typeof module !== 'undefined' && module.exports) {
  module.exports = globalThis.KXPerks;
}
