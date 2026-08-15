/* KroniX: Loch Bez Końca — główna pętla gry, walka, kamera, rendering i UI. */
(function (global) {
  const TILE = 32;
  const { generateFloor } = global.KXDungeon;
  const { createPlayer, createEnemy, createChest, createProjectile, xpToNext } = global.KXEntities;
  const { rollPerkChoices, applyPerk, META_UPGRADES, metaUpgradeCost, EXTRA_LIFE_COST } = global.KXPerks;

  function randInt(a, b) { return Math.floor(Math.random() * (b - a + 1)) + a; }
  function clamp(v, a, b) { return Math.max(a, Math.min(b, v)); }
  function dist(ax, ay, bx, by) { return Math.hypot(ax - bx, ay - by); }
  function computeDamage(atk, def) {
    const raw = Math.max(1, atk - def * 0.6);
    return Math.round(raw * (0.85 + Math.random() * 0.3));
  }

  const G = {
    canvas: null, ctx: null, viewW: 0, viewH: 0,
    mode: 'hub', // hub | playing | paused | levelup | gameover
    save: null, player: null, dungeon: null, floorNum: 1,
    enemies: [], chests: [], projectiles: [], particles: [], texts: [],
    camera: { x: 0, y: 0 }, shake: 0,
    pendingPerks: [], currentPerkChoices: null,
    lastTime: 0, toastTimer: 0,
  };

  function persistSave() { global.KXSave.save(G.save); }

  // ---------------------------------------------------------------- setup
  function init() {
    G.canvas = document.getElementById('game-canvas');
    G.ctx = G.canvas.getContext('2d');
    resize();
    window.addEventListener('resize', resize);
    window.addEventListener('orientationchange', resize);

    global.KXInput.init({
      touchZone: document.getElementById('touch-zone'),
      joyBase: document.getElementById('joy-base'),
      joyKnob: document.getElementById('joy-knob'),
      dashBtn: document.getElementById('dash-btn'),
    });

    G.save = global.KXSave.load();

    wireUI();
    renderHub();
    showScreen('screen-hub');

    requestAnimationFrame(loop);
    registerServiceWorker();
  }

  function resize() {
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    G.viewW = window.innerWidth;
    G.viewH = window.innerHeight;
    G.canvas.width = Math.round(G.viewW * dpr);
    G.canvas.height = Math.round(G.viewH * dpr);
    G.canvas.style.width = G.viewW + 'px';
    G.canvas.style.height = G.viewH + 'px';
    G.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  }

  function registerServiceWorker() {
    if ('serviceWorker' in navigator) {
      navigator.serviceWorker.register('sw.js').catch(() => {});
    }
  }

  // ---------------------------------------------------------------- UI wiring
  function showScreen(id) {
    document.querySelectorAll('.screen').forEach(el => el.classList.add('hidden'));
    if (id) document.getElementById(id).classList.remove('hidden');
  }

  function wireUI() {
    document.getElementById('btn-start').addEventListener('click', startRun);
    document.getElementById('btn-resume').addEventListener('click', () => { G.mode = 'playing'; showScreen(null); });
    document.getElementById('btn-quit').addEventListener('click', () => { G.mode = 'hub'; renderHub(); showScreen('screen-hub'); });
    document.getElementById('btn-retry').addEventListener('click', () => { G.mode = 'hub'; renderHub(); showScreen('screen-hub'); });
    document.getElementById('pause-btn').addEventListener('click', () => {
      if (G.mode === 'playing') { G.mode = 'paused'; showScreen('screen-pause'); }
    });

    ['hp', 'atk', 'def'].forEach(key => {
      document.getElementById('upg-' + key).addEventListener('click', () => buyMetaUpgrade(key));
    });
    document.getElementById('upg-extralife').addEventListener('click', buyExtraLife);
  }

  function buyMetaUpgrade(key) {
    const lvl = G.save.upgrades[key];
    if (lvl >= META_UPGRADES[key].max) return;
    const cost = metaUpgradeCost(key, lvl);
    if (G.save.metaGold < cost) return;
    G.save.metaGold -= cost;
    G.save.upgrades[key]++;
    persistSave();
    renderHub();
  }
  function buyExtraLife() {
    if (G.save.extraLifeOwned || G.save.metaGold < EXTRA_LIFE_COST) return;
    G.save.metaGold -= EXTRA_LIFE_COST;
    G.save.extraLifeOwned = true;
    persistSave();
    renderHub();
  }

  function renderHub() {
    document.getElementById('hub-gold').textContent = G.save.metaGold;
    document.getElementById('hub-best').textContent = G.save.bestFloor;
    ['hp', 'atk', 'def'].forEach(key => {
      const lvl = G.save.upgrades[key];
      const cfg = META_UPGRADES[key];
      const btn = document.getElementById('upg-' + key);
      const maxed = lvl >= cfg.max;
      btn.querySelector('.upg-level').textContent = 'Lv. ' + lvl + (maxed ? ' (MAX)' : '');
      btn.querySelector('.upg-cost').textContent = maxed ? '—' : metaUpgradeCost(key, lvl) + ' zł';
      btn.disabled = maxed || G.save.metaGold < metaUpgradeCost(key, lvl);
    });
    const elBtn = document.getElementById('upg-extralife');
    elBtn.querySelector('.upg-cost').textContent = G.save.extraLifeOwned ? 'Posiadane' : EXTRA_LIFE_COST + ' zł';
    elBtn.disabled = G.save.extraLifeOwned || G.save.metaGold < EXTRA_LIFE_COST;
  }

  // ---------------------------------------------------------------- run flow
  function startRun() {
    G.floorNum = 1;
    G.player = createPlayer(G.save);
    loadFloor(1);
    G.mode = 'playing';
    showScreen(null);
  }

  function loadFloor(n) {
    const d = generateFloor(n);
    G.dungeon = d;
    G.enemies = d.enemySpawns.map(s => createEnemy(s, n, TILE));
    G.chests = d.chestSpawns.map(s => createChest(s, TILE));
    G.projectiles = [];
    G.player.x = (d.start.x + 0.5) * TILE;
    G.player.y = (d.start.y + 0.5) * TILE;
    showToast((d.isBossFloor ? '👹 Piętro Bossa ' : 'Piętro ') + n, 1.8);
  }

  function goToNextFloor() {
    const cleared = G.enemies.length > 0 && G.enemies.every(e => !e.alive);
    if (cleared) {
      const bonus = 10 + G.floorNum * 3;
      addGold(bonus);
      G.player.xp += 5 + G.floorNum * 2;
      checkLevelUp();
      showToast('Piętro wyczyszczone! +' + bonus + ' zł premii', 1.8);
    }
    G.save.bestFloor = Math.max(G.save.bestFloor, G.floorNum);
    persistSave();
    G.floorNum++;
    loadFloor(G.floorNum);
  }

  function endRun() {
    G.mode = 'gameover';
    G.save.totalRuns++;
    G.save.bestFloor = Math.max(G.save.bestFloor, G.floorNum);
    persistSave();
    document.getElementById('go-floor').textContent = G.floorNum;
    document.getElementById('go-level').textContent = G.player.level;
    document.getElementById('go-gold').textContent = G.player.gold;
    showScreen('screen-gameover');
  }

  function addGold(amount) {
    const g = Math.round(amount * G.player.goldMult);
    G.player.gold += g;
    G.save.metaGold += g;
    persistSave();
    return g;
  }

  function showToast(text, seconds) {
    const el = document.getElementById('toast');
    el.textContent = text;
    el.classList.add('show');
    G.toastTimer = seconds || 1.4;
  }

  // ---------------------------------------------------------------- level up
  function checkLevelUp() {
    const p = G.player;
    let leveled = false;
    while (p.xp >= p.xpToNext) {
      p.xp -= p.xpToNext;
      p.level++;
      p.xpToNext = xpToNext(p.level);
      p.hp = p.maxHp;
      G.pendingPerks.push(rollPerkChoices(3));
      leveled = true;
    }
    if (leveled && G.mode === 'playing') openLevelUpModal();
  }

  function openLevelUpModal() {
    G.mode = 'levelup';
    G.currentPerkChoices = G.pendingPerks.shift();
    const box = document.getElementById('levelup-choices');
    box.innerHTML = '';
    document.getElementById('levelup-title').textContent = 'Poziom ' + G.player.level + '! Wybierz nagrodę:';
    G.currentPerkChoices.forEach(perk => {
      const btn = document.createElement('button');
      btn.className = 'perk-btn';
      btn.innerHTML = '<b>' + perk.name + '</b><span>' + perk.desc + '</span>';
      btn.addEventListener('click', () => choosePerk(perk.id));
      box.appendChild(btn);
    });
    showScreen('screen-levelup');
  }

  function choosePerk(id) {
    applyPerk(G.player, id);
    if (G.pendingPerks.length > 0) { openLevelUpModal(); return; }
    G.mode = 'playing';
    showScreen(null);
  }

  // ---------------------------------------------------------------- collision
  function collidesAt(px, py, radius) {
    const d = G.dungeon;
    const pts = [[px - radius, py], [px + radius, py], [px, py - radius], [px, py + radius]];
    for (const [x, y] of pts) {
      const tx = Math.floor(x / TILE), ty = Math.floor(y / TILE);
      if (ty < 0 || ty >= d.rows || tx < 0 || tx >= d.cols) return true;
      if (d.grid[ty][tx] === d.tileWall) return true;
    }
    return false;
  }
  function tryMove(e, vx, vy, dt) {
    const nx = e.x + vx * dt;
    if (!collidesAt(nx, e.y, e.radius)) e.x = nx;
    const ny = e.y + vy * dt;
    if (!collidesAt(e.x, ny, e.radius)) e.y = ny;
  }

  // ---------------------------------------------------------------- fx
  function spawnParticles(x, y, color, count) {
    for (let i = 0; i < count; i++) {
      const a = Math.random() * Math.PI * 2, s = 40 + Math.random() * 90;
      G.particles.push({ x, y, vx: Math.cos(a) * s, vy: Math.sin(a) * s, life: 0.35 + Math.random() * 0.25, maxLife: 0.6, color, size: 2 + Math.random() * 2 });
    }
  }
  function spawnText(x, y, text, color) {
    G.texts.push({ x, y, vy: -38, life: 0.8, maxLife: 0.8, text, color });
  }

  // ---------------------------------------------------------------- update
  function update(dt) {
    const p = G.player;
    const input = global.KXInput;

    // Dash
    if (input.consumeDash() && p.dashTimer <= 0) {
      p.dashTimeLeft = p.dashDuration;
      p.dashTimer = p.dashCooldownMax;
      p.invulnTimer = p.dashDuration + 0.05;
      spawnParticles(p.x, p.y, '#7ef9ff', 10);
    }
    p.dashTimer = Math.max(0, p.dashTimer - dt);
    p.dashTimeLeft = Math.max(0, p.dashTimeLeft - dt);
    p.invulnTimer = Math.max(0, p.invulnTimer - dt);
    p.hitFlash = Math.max(0, p.hitFlash - dt);
    p.atkTimer -= dt;
    G.shake = Math.max(0, G.shake - dt * 10);

    const dashMul = p.dashTimeLeft > 0 ? p.dashSpeedMult : 1;
    const mvx = input.state.moveX, mvy = input.state.moveY;
    const mlen = Math.hypot(mvx, mvy);
    if (mlen > 0.05) {
      const speed = p.speed * p.moveMult * dashMul;
      tryMove(p, (mvx / Math.max(mlen, 1)) * speed, (mvy / Math.max(mlen, 1)) * speed, dt);
      p.facing = { x: mvx / mlen, y: mvy / mlen };
    }

    // Regen
    if (p.regenPerSec) {
      p.regenTimer += dt;
      if (p.regenTimer >= 1) { p.regenTimer -= 1; p.hp = Math.min(p.maxHp, p.hp + p.regenPerSec); }
    }

    // Player auto-attack
    if (p.atkTimer <= 0) {
      let target = null, best = Infinity;
      for (const e of G.enemies) {
        if (!e.alive) continue;
        const d = dist(p.x, p.y, e.x, e.y);
        if (d <= p.atkRange + e.radius && d < best) { best = d; target = e; }
      }
      if (target) {
        p.atkTimer = p.atkCooldown;
        p.atkFlash = 0.15;
        let dmg = computeDamage(p.atk, target.def);
        const crit = Math.random() < p.critChance;
        if (crit) dmg = Math.round(dmg * p.critMult);
        target.hp -= dmg;
        target.hitFlash = 0.15;
        spawnText(target.x, target.y - target.radius - 8, (crit ? '⚡' : '') + dmg, crit ? '#ffd23f' : '#ffffff');
        spawnParticles(target.x, target.y, '#ffffff', crit ? 6 : 3);
        if (p.lifesteal > 0) {
          const heal = Math.round(dmg * p.lifesteal);
          if (heal > 0) { p.hp = Math.min(p.maxHp, p.hp + heal); spawnText(p.x, p.y - 26, '+' + heal, '#4ad66d'); }
        }
        if (target.hp <= 0 && target.alive) {
          target.alive = false;
          p.xp += target.xpValue;
          const g = addGold(target.goldValue);
          spawnText(target.x, target.y - 10, '+' + g + 'z', '#ffd23f');
          spawnParticles(target.x, target.y, target.color, 12);
          checkLevelUp();
        }
      }
    }
    if (p.atkFlash) p.atkFlash = Math.max(0, p.atkFlash - dt);

    // Enemies
    for (const e of G.enemies) updateEnemy(e, dt, p);

    // Projectiles
    G.projectiles = G.projectiles.filter(pr => {
      pr.x += pr.vx * dt; pr.y += pr.vy * dt; pr.life -= dt;
      if (pr.life <= 0) return false;
      const tx = Math.floor(pr.x / TILE), ty = Math.floor(pr.y / TILE);
      const d = G.dungeon;
      if (ty < 0 || ty >= d.rows || tx < 0 || tx >= d.cols || d.grid[ty][tx] === d.tileWall) return false;
      if (pr.owner === 'enemy' && p.invulnTimer <= 0 && dist(pr.x, pr.y, p.x, p.y) < pr.radius + p.radius) {
        const dmg = computeDamage(pr.dmg, p.def);
        p.hp -= dmg; p.hitFlash = 0.2;
        spawnText(p.x, p.y - 20, '-' + dmg, '#ff6b6b');
        G.shake = 5;
        return false;
      }
      return true;
    });

    // Chests
    for (const c of G.chests) {
      if (c.opened) continue;
      if (dist(c.x, c.y, p.x, p.y) < c.radius + p.radius + 6) openChest(c);
    }

    // Exit check
    const ex = (G.dungeon.exit.x + 0.5) * TILE, ey = (G.dungeon.exit.y + 0.5) * TILE;
    if (dist(p.x, p.y, ex, ey) < TILE * 0.55) goToNextFloor();

    // Particles / texts
    G.particles = G.particles.filter(pt => { pt.x += pt.vx * dt; pt.y += pt.vy * dt; pt.vx *= 0.9; pt.vy *= 0.9; pt.life -= dt; return pt.life > 0; });
    G.texts = G.texts.filter(t => { t.y += t.vy * dt; t.life -= dt; return t.life > 0; });

    // Toast timer
    if (G.toastTimer > 0) {
      G.toastTimer -= dt;
      if (G.toastTimer <= 0) document.getElementById('toast').classList.remove('show');
    }

    // Death
    if (p.hp <= 0) {
      if (p.extraLives > 0) {
        p.extraLives--; p.hp = Math.round(p.maxHp * 0.5); p.invulnTimer = 1.5;
        showToast('Dodatkowe życie!', 1.5);
      } else {
        endRun();
      }
    }

    // Camera
    G.camera.x = camAxis(p.x, G.dungeon.cols * TILE, G.viewW);
    G.camera.y = camAxis(p.y, G.dungeon.rows * TILE, G.viewH);

    updateHUD();
  }

  function camAxis(target, world, view) {
    if (world <= view) return -(view - world) / 2;
    return clamp(target - view / 2, 0, world - view);
  }

  function updateEnemy(e, dt, p) {
    if (!e.alive) return;
    e.hitFlash = Math.max(0, e.hitFlash - dt);
    e.contactTimer = Math.max(0, e.contactTimer - dt);
    const dx = p.x - e.x, dy = p.y - e.y;
    const d = Math.hypot(dx, dy) || 1;
    const aggro = e.boss ? 460 : 260;
    if (d < aggro) {
      if (e.ranged) {
        const desired = e.range * 0.55;
        if (d > desired) moveToward(e, dx, dy, d, dt, 1);
        else if (d < desired * 0.6) moveToward(e, -dx, -dy, d, dt, 0.8);
        e.atkTimer -= dt;
        if (d < e.range && e.atkTimer <= 0) {
          e.atkTimer = e.atkCooldown;
          G.projectiles.push(createProjectile(e.x, e.y, dx, dy, e.projSpeed, e.atk, 'enemy'));
        }
      } else {
        if (d > e.radius + p.radius + 2) moveToward(e, dx, dy, d, dt, 1);
        if (d < e.radius + p.radius + 6 && e.contactTimer <= 0 && p.invulnTimer <= 0) {
          e.contactTimer = e.contactCooldown;
          const dmg = computeDamage(e.atk, p.def);
          p.hp -= dmg; p.hitFlash = 0.2;
          spawnText(p.x, p.y - 20, '-' + dmg, '#ff6b6b');
          G.shake = e.boss ? 9 : 4;
        }
      }
    } else {
      e.wanderTimer = (e.wanderTimer || 0) - dt;
      if (e.wanderTimer <= 0) {
        e.wanderDir = { x: Math.random() * 2 - 1, y: Math.random() * 2 - 1 };
        e.wanderTimer = 1 + Math.random() * 1.5;
      }
      const wd = e.wanderDir || { x: 0, y: 0 };
      const wl = Math.hypot(wd.x, wd.y) || 1;
      moveToward(e, wd.x, wd.y, wl, dt, 0.3);
    }
  }
  function moveToward(e, dx, dy, d, dt, mul) {
    tryMove(e, (dx / d) * e.speed * mul, (dy / d) * e.speed * mul, dt);
  }

  function openChest(c) {
    c.opened = true;
    const p = G.player;
    let msg;
    if (c.guaranteed) {
      const g = addGold(randInt(60, 100) + G.floorNum * 5);
      const perk = rollPerkChoices(1)[0];
      applyPerk(p, perk.id);
      msg = 'Skrzynia Bossa! +' + g + 'z, ' + perk.name;
      checkLevelUp();
    } else {
      const roll = Math.random();
      if (roll < 0.55) {
        const g = addGold(randInt(8, 16) + G.floorNum * 3);
        msg = '+' + g + ' złota';
      } else if (roll < 0.78) {
        const heal = Math.round(p.maxHp * 0.35);
        p.hp = Math.min(p.maxHp, p.hp + heal);
        msg = '+' + heal + ' HP';
      } else if (roll < 0.93) {
        const g = addGold(randInt(20, 35) + G.floorNum * 4);
        const heal = Math.round(p.maxHp * 0.2);
        p.hp = Math.min(p.maxHp, p.hp + heal);
        msg = 'Skarb! +' + g + 'z, +' + heal + 'HP';
      } else {
        const perk = rollPerkChoices(1)[0];
        applyPerk(p, perk.id);
        msg = 'Artefakt: ' + perk.name + '!';
        checkLevelUp();
      }
    }
    spawnText(c.x, c.y - 18, msg, '#ffd23f');
    spawnParticles(c.x, c.y, '#ffd23f', 14);
  }

  // ---------------------------------------------------------------- HUD
  function updateHUD() {
    const p = G.player;
    document.getElementById('hp-fill').style.width = clamp((p.hp / p.maxHp) * 100, 0, 100) + '%';
    document.getElementById('hp-text').textContent = Math.max(0, Math.round(p.hp)) + ' / ' + Math.round(p.maxHp);
    document.getElementById('xp-fill').style.width = clamp((p.xp / p.xpToNext) * 100, 0, 100) + '%';
    document.getElementById('level-badge').textContent = 'Lv. ' + p.level;
    document.getElementById('floor-badge').textContent = 'Piętro ' + G.floorNum;
    document.getElementById('gold-badge').textContent = p.gold + ' zł';

    const boss = G.enemies.find(e => e.boss && e.alive);
    const bossBar = document.getElementById('boss-bar');
    if (boss) {
      bossBar.classList.remove('hidden');
      document.getElementById('boss-name').textContent = boss.name;
      document.getElementById('boss-fill').style.width = clamp((boss.hp / boss.maxHp) * 100, 0, 100) + '%';
    } else {
      bossBar.classList.add('hidden');
    }

    const dashBtn = document.getElementById('dash-btn');
    const cd = p.dashTimer / p.dashCooldownMax;
    dashBtn.style.setProperty('--cd', (1 - cd));
    dashBtn.classList.toggle('ready', p.dashTimer <= 0);
  }

  // ---------------------------------------------------------------- render
  function render() {
    const ctx = G.ctx, d = G.dungeon;
    ctx.fillStyle = '#0d0a14';
    ctx.fillRect(0, 0, G.viewW, G.viewH);
    if (G.mode === 'hub') return;

    const shakeX = (Math.random() - 0.5) * G.shake;
    const shakeY = (Math.random() - 0.5) * G.shake;
    ctx.save();
    ctx.translate(-Math.round(G.camera.x - shakeX), -Math.round(G.camera.y - shakeY));

    const startCol = Math.max(0, Math.floor(G.camera.x / TILE) - 1);
    const endCol = Math.min(d.cols - 1, Math.ceil((G.camera.x + G.viewW) / TILE) + 1);
    const startRow = Math.max(0, Math.floor(G.camera.y / TILE) - 1);
    const endRow = Math.min(d.rows - 1, Math.ceil((G.camera.y + G.viewH) / TILE) + 1);

    for (let y = startRow; y <= endRow; y++) {
      for (let x = startCol; x <= endCol; x++) {
        const wall = d.grid[y][x] === d.tileWall;
        if (wall) {
          ctx.fillStyle = '#1a1424';
          ctx.fillRect(x * TILE, y * TILE, TILE, TILE);
          const below = y + 1 < d.rows ? d.grid[y + 1][x] : d.tileWall;
          if (below !== d.tileWall) { ctx.fillStyle = '#2c2140'; ctx.fillRect(x * TILE, y * TILE + TILE - 4, TILE, 4); }
        } else {
          ctx.fillStyle = (x + y) % 2 === 0 ? '#2a2340' : '#2e2746';
          ctx.fillRect(x * TILE, y * TILE, TILE, TILE);
        }
      }
    }

    // Exit glow
    const ex = (d.exit.x + 0.5) * TILE, ey = (d.exit.y + 0.5) * TILE;
    const pulse = 6 * Math.sin(performance.now() / 200);
    const grad = ctx.createRadialGradient(ex, ey, 2, ex, ey, 22 + pulse);
    grad.addColorStop(0, '#ffe98a'); grad.addColorStop(1, 'rgba(255,233,138,0)');
    ctx.fillStyle = grad;
    ctx.beginPath(); ctx.arc(ex, ey, 22 + pulse, 0, Math.PI * 2); ctx.fill();

    // Chests
    for (const c of G.chests) {
      if (c.opened) continue;
      ctx.fillStyle = c.guaranteed ? '#ff9f43' : '#c98a3c';
      ctx.fillRect(c.x - 10, c.y - 8, 20, 16);
      ctx.fillStyle = '#5c3a1e';
      ctx.fillRect(c.x - 10, c.y - 2, 20, 3);
      ctx.fillStyle = '#ffe98a';
      ctx.fillRect(c.x - 2, c.y - 8, 4, 4);
    }

    // Enemies
    for (const e of G.enemies) {
      if (!e.alive) continue;
      ctx.save();
      if (e.hitFlash > 0) ctx.globalAlpha = 0.6;
      ctx.fillStyle = e.color;
      ctx.beginPath(); ctx.arc(e.x, e.y, e.radius, 0, Math.PI * 2); ctx.fill();
      if (e.elite || e.boss) {
        ctx.strokeStyle = e.boss ? '#ff5252' : '#ffd23f';
        ctx.lineWidth = 2.5;
        ctx.stroke();
      }
      ctx.restore();
      if (e.hp < e.maxHp) {
        const w = e.radius * 2;
        ctx.fillStyle = 'rgba(0,0,0,0.5)';
        ctx.fillRect(e.x - w / 2, e.y - e.radius - 10, w, 4);
        ctx.fillStyle = '#ff5252';
        ctx.fillRect(e.x - w / 2, e.y - e.radius - 10, w * clamp(e.hp / e.maxHp, 0, 1), 4);
      }
    }

    // Projectiles
    for (const pr of G.projectiles) {
      ctx.fillStyle = pr.owner === 'enemy' ? '#c96bff' : '#7ef9ff';
      ctx.beginPath(); ctx.arc(pr.x, pr.y, pr.radius, 0, Math.PI * 2); ctx.fill();
    }

    // Player
    const p = G.player;
    if (p.atkFlash) {
      ctx.strokeStyle = 'rgba(255,255,255,' + (p.atkFlash / 0.15) * 0.6 + ')';
      ctx.lineWidth = 2;
      ctx.beginPath(); ctx.arc(p.x, p.y, p.atkRange, 0, Math.PI * 2); ctx.stroke();
    }
    ctx.save();
    if (p.invulnTimer > 0 && Math.floor(performance.now() / 80) % 2 === 0) ctx.globalAlpha = 0.4;
    ctx.fillStyle = p.hitFlash > 0 ? '#ff8a8a' : '#5ec8ff';
    ctx.beginPath(); ctx.arc(p.x, p.y, p.radius, 0, Math.PI * 2); ctx.fill();
    ctx.fillStyle = '#0d0a14';
    ctx.beginPath(); ctx.arc(p.x + p.facing.x * 6, p.y + p.facing.y * 6, 3, 0, Math.PI * 2); ctx.fill();
    ctx.restore();

    // Particles
    for (const pt of G.particles) {
      ctx.globalAlpha = clamp(pt.life / pt.maxLife, 0, 1);
      ctx.fillStyle = pt.color;
      ctx.beginPath(); ctx.arc(pt.x, pt.y, pt.size, 0, Math.PI * 2); ctx.fill();
    }
    ctx.globalAlpha = 1;

    // Damage / loot texts
    ctx.textAlign = 'center';
    ctx.font = 'bold 14px sans-serif';
    for (const t of G.texts) {
      ctx.globalAlpha = clamp(t.life / t.maxLife, 0, 1);
      ctx.fillStyle = t.color;
      ctx.fillText(t.text, t.x, t.y);
    }
    ctx.globalAlpha = 1;

    ctx.restore();
  }

  // ---------------------------------------------------------------- loop
  function loop(ts) {
    const dt = Math.min((ts - (G.lastTime || ts)) / 1000, 0.05);
    G.lastTime = ts;
    if (G.mode === 'playing') update(dt);
    render();
    requestAnimationFrame(loop);
  }

  document.addEventListener('DOMContentLoaded', init);
})(typeof window !== 'undefined' ? window : globalThis);
