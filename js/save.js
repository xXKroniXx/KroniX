/* KroniX: Loch Bez Końca — trwały zapis postępu (localStorage) */
(function (global) {
  const KEY = 'kronix_dungeon_save_v1';

  function defaultSave() {
    return {
      metaGold: 0,
      upgrades: { hp: 0, atk: 0, def: 0 },
      extraLifeOwned: false,
      bestFloor: 0,
      totalRuns: 0,
    };
  }

  function load() {
    try {
      const raw = localStorage.getItem(KEY);
      if (!raw) return defaultSave();
      const parsed = JSON.parse(raw);
      const def = defaultSave();
      return Object.assign(def, parsed, { upgrades: Object.assign(def.upgrades, parsed.upgrades) });
    } catch (e) {
      console.warn('Nie udało się wczytać zapisu, startuję od nowa.', e);
      return defaultSave();
    }
  }

  function save(state) {
    try {
      localStorage.setItem(KEY, JSON.stringify(state));
    } catch (e) {
      console.warn('Nie udało się zapisać stanu gry.', e);
    }
  }

  global.KXSave = { load, save, defaultSave };
})(typeof window !== 'undefined' ? window : globalThis);

if (typeof module !== 'undefined' && module.exports) {
  module.exports = globalThis.KXSave;
}
