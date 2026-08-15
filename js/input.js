/* KroniX: Loch Bez Końca — sterowanie: dynamiczny joystick dotykowy, przycisk dash,
 * z zapasowym sterowaniem klawiaturą (WASD/strzałki + Spacja) do testów na komputerze.
 */
(function (global) {
  const state = {
    moveX: 0, moveY: 0,
    dashRequested: false,
    pauseRequested: false,
  };

  const keys = new Set();
  let joystickTouchId = null;
  let joystickOrigin = { x: 0, y: 0 };
  const JOY_RADIUS = 46;

  let dom = null;

  function clampVector(x, y, max) {
    const len = Math.hypot(x, y);
    if (len <= max) return { x, y, len };
    return { x: (x / len) * max, y: (y / len) * max, len: max };
  }

  function showJoystick(x, y) {
    if (!dom) return;
    dom.joyBase.style.left = x + 'px';
    dom.joyBase.style.top = y + 'px';
    dom.joyBase.style.opacity = '1';
    dom.joyKnob.style.left = x + 'px';
    dom.joyKnob.style.top = y + 'px';
    dom.joyKnob.style.opacity = '1';
  }
  function moveKnob(x, y) {
    if (!dom) return;
    dom.joyKnob.style.left = x + 'px';
    dom.joyKnob.style.top = y + 'px';
  }
  function hideJoystick() {
    if (!dom) return;
    dom.joyBase.style.opacity = '0';
    dom.joyKnob.style.opacity = '0';
  }

  function onTouchStart(e) {
    for (const t of e.changedTouches) {
      const target = document.elementFromPoint(t.clientX, t.clientY);
      if (target && target.closest('#dash-btn')) continue;
      if (target && target.closest('.hud-btn')) continue;
      if (joystickTouchId !== null) continue;
      joystickTouchId = t.identifier;
      joystickOrigin = { x: t.clientX, y: t.clientY };
      showJoystick(t.clientX, t.clientY);
    }
  }
  function onTouchMove(e) {
    for (const t of e.changedTouches) {
      if (t.identifier !== joystickTouchId) continue;
      const dx = t.clientX - joystickOrigin.x;
      const dy = t.clientY - joystickOrigin.y;
      const c = clampVector(dx, dy, JOY_RADIUS);
      moveKnob(joystickOrigin.x + c.x, joystickOrigin.y + c.y);
      state.moveX = c.x / JOY_RADIUS;
      state.moveY = c.y / JOY_RADIUS;
    }
  }
  function endTouch(e) {
    for (const t of e.changedTouches) {
      if (t.identifier !== joystickTouchId) continue;
      joystickTouchId = null;
      state.moveX = 0; state.moveY = 0;
      hideJoystick();
    }
  }

  function onKeyDown(e) {
    const k = e.key.toLowerCase();
    if (['arrowup', 'arrowdown', 'arrowleft', 'arrowright', 'w', 'a', 's', 'd'].includes(k)) {
      keys.add(k);
      updateKeyboardVector();
    } else if (k === ' ' || k === 'shift') {
      if (!e.repeat) state.dashRequested = true;
    } else if (k === 'escape') {
      state.pauseRequested = true;
    }
  }
  function onKeyUp(e) {
    const k = e.key.toLowerCase();
    keys.delete(k);
    updateKeyboardVector();
  }
  function updateKeyboardVector() {
    if (joystickTouchId !== null) return; // touch takes priority
    let x = 0, y = 0;
    if (keys.has('a') || keys.has('arrowleft')) x -= 1;
    if (keys.has('d') || keys.has('arrowright')) x += 1;
    if (keys.has('w') || keys.has('arrowup')) y -= 1;
    if (keys.has('s') || keys.has('arrowdown')) y += 1;
    const len = Math.hypot(x, y) || 1;
    state.moveX = x / len;
    state.moveY = y / len;
  }

  function consumeDash() {
    if (state.dashRequested) { state.dashRequested = false; return true; }
    return false;
  }
  function consumePause() {
    if (state.pauseRequested) { state.pauseRequested = false; return true; }
    return false;
  }

  function init(elements) {
    dom = elements; // { touchZone, joyBase, joyKnob, dashBtn }
    dom.touchZone.addEventListener('touchstart', onTouchStart, { passive: true });
    dom.touchZone.addEventListener('touchmove', onTouchMove, { passive: true });
    dom.touchZone.addEventListener('touchend', endTouch, { passive: true });
    dom.touchZone.addEventListener('touchcancel', endTouch, { passive: true });

    dom.dashBtn.addEventListener('touchstart', (e) => { e.preventDefault(); state.dashRequested = true; }, { passive: false });
    dom.dashBtn.addEventListener('click', () => { state.dashRequested = true; });

    window.addEventListener('keydown', onKeyDown);
    window.addEventListener('keyup', onKeyUp);
  }

  global.KXInput = { init, state, consumeDash, consumePause };
})(typeof window !== 'undefined' ? window : globalThis);
