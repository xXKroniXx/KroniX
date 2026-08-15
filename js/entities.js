/* KroniX: Loch Bez Końca — encje gry (gracz, przeciwnicy, skrzynie, pociski)
 * Statystyki przeciwników skalują się proceduralnie wraz z numerem piętra.
 */
(function (global) {
  let uidCounter = 1;
  function uid() { return uidCounter++; }
  function randInt(min, max) { return Math.floor(Math.random() * (max - min + 1)) + min; }

  const ENEMY_BASE = {
    rat:      { hp: 12, atk: 4, def: 0, speed: 100, radius: 10, xp: 6,  gold: [1, 3], color: '#b58a5c', name: 'Szczur Lochu' },
    skeleton: { hp: 24, atk: 6, def: 2, speed: 62,  radius: 12, xp: 10, gold: [2, 5], color: '#d9dde3', name: 'Szkielet' },
    slime:    { hp: 42, atk: 5, def: 1, speed: 38,  radius: 15, xp: 14, gold: [3, 6], color: '#49d67a', name: 'Szlam' },
    cultist:  { hp: 18, atk: 7, def: 0, speed: 50,  radius: 11, xp: 16, gold: [3, 7], color: '#9a5cff', name: 'Kultysta',
                ranged: true, range: 230, projSpeed: 190, atkCooldown: 1.7 },
  };
  const BOSS_BASE = { hp: 240, atk: 15, def: 4, speed: 56, radius: 22, xp: 140, gold: [45, 75], color: '#ff5252', name: 'Strażnik Lochu' };

  function pickEnemyType(floor) {
    const pool = ['rat'];
    if (floor >= 2) pool.push('skeleton');
    if (floor >= 4) pool.push('slime');
    if (floor >= 5) pool.push('cultist');
    if (floor >= 2) pool.push('rat'); // weight rats a bit more early
    return pool[randInt(0, pool.length - 1)];
  }

  function eliteChanceForFloor(floor) {
    return Math.max(0.04, Math.min(0.4, 0.04 + floor * 0.015));
  }

  function scaleStats(base, floor, elite, boss) {
    const hpMul = (1 + floor * (boss ? 0.13 : 0.17)) * (elite ? 1.7 : 1);
    const atkMul = (1 + floor * 0.10) * (elite ? 1.4 : 1);
    return {
      hp: Math.max(1, Math.round(base.hp * hpMul)),
      atk: Math.max(1, Math.round(base.atk * atkMul)),
      def: Math.round(base.def + floor * 0.15),
    };
  }

  function createEnemy(spawn, floor, tileSize) {
    const boss = !!spawn.boss;
    const elite = !!spawn.elite && !boss;
    const type = boss ? 'boss' : pickEnemyType(floor);
    const base = boss ? BOSS_BASE : ENEMY_BASE[type];
    const stats = scaleStats(base, floor, elite, boss);
    return {
      id: uid(), kind: 'enemy', type, boss, elite,
      x: (spawn.x + 0.5) * tileSize, y: (spawn.y + 0.5) * tileSize,
      radius: base.radius * (boss ? 1.65 : 1) * (elite ? 1.15 : 1),
      hp: stats.hp, maxHp: stats.hp, atk: stats.atk, def: stats.def,
      speed: base.speed,
      xpValue: Math.round(base.xp * (1 + floor * 0.16) * (elite ? 1.8 : 1)),
      goldValue: randInt(base.gold[0], base.gold[1]) + Math.round(floor * 1.1) + (elite ? randInt(5, 10) : 0),
      color: boss ? BOSS_BASE.color : base.color,
      name: (elite ? 'Elitarny ' : '') + (boss ? base.name + ' Nr. ' + floor : base.name),
      ranged: !!base.ranged, range: base.range || 0, projSpeed: base.projSpeed || 0,
      atkCooldown: base.atkCooldown || 0.9, atkTimer: Math.random() * 0.6,
      hitFlash: 0, alive: true, contactCooldown: 0.7, contactTimer: 0,
    };
  }

  function xpToNext(level) { return Math.round(18 * Math.pow(level, 1.45) + 12); }

  function createPlayer(meta) {
    const up = (meta && meta.upgrades) || { hp: 0, atk: 0, def: 0 };
    const maxHp = 60 + up.hp * 9;
    return {
      kind: 'player', x: 0, y: 0, radius: 12, speed: 150,
      level: 1, xp: 0, xpToNext: xpToNext(1),
      maxHp, hp: maxHp, atk: 8 + up.atk * 1.6, def: 1 + up.def * 1.1,
      critChance: 0.05, critMult: 1.8, lifesteal: 0,
      atkRange: 48, atkCooldown: 0.55, atkTimer: 0,
      goldMult: 1, moveMult: 1,
      dashCooldownMax: 3.0, dashTimer: 0, dashDuration: 0.2, dashTimeLeft: 0,
      dashSpeedMult: 3.2, invulnTimer: 0,
      perks: {}, gold: 0, extraLives: meta && meta.extraLifeOwned ? 1 : 0,
      hitFlash: 0, facing: { x: 0, y: 1 }, regenTimer: 0,
    };
  }

  function createChest(spawn, tileSize) {
    return { id: uid(), kind: 'chest', x: (spawn.x + 0.5) * tileSize, y: (spawn.y + 0.5) * tileSize,
             radius: 13, opened: false, guaranteed: !!spawn.guaranteed };
  }

  function createProjectile(x, y, dx, dy, speed, dmg, owner) {
    const len = Math.hypot(dx, dy) || 1;
    return { id: uid(), kind: 'projectile', x, y, vx: (dx / len) * speed, vy: (dy / len) * speed,
             dmg, owner, radius: 5, life: 3.5 };
  }

  global.KXEntities = {
    ENEMY_BASE, BOSS_BASE, eliteChanceForFloor,
    createEnemy, createPlayer, createChest, createProjectile, xpToNext, uid,
  };
})(typeof window !== 'undefined' ? window : globalThis);

if (typeof module !== 'undefined' && module.exports) {
  module.exports = globalThis.KXEntities;
}
