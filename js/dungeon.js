/* KroniX: Loch Bez Końca — generator lochu (proceduralne poziomy)
 * Algorytm: rekurencyjny podział przestrzeni (BSP) -> pokoje w liściach ->
 * łączenie pokoi korytarzami wzdłuż drzewa podziału -> populacja
 * (przeciwnicy / skrzynie / wyjście) skalowana numerem piętra.
 */
(function (global) {
  const TILE = { WALL: 1, FLOOR: 0 };

  function clamp(v, min, max) { return Math.max(min, Math.min(max, v)); }
  function randInt(min, max) { return Math.floor(Math.random() * (max - min + 1)) + min; }
  function choice(arr) { return arr[Math.floor(Math.random() * arr.length)]; }

  function makeGrid(cols, rows, fill) {
    return Array.from({ length: rows }, () => Array(cols).fill(fill));
  }

  // --- BSP split -----------------------------------------------------
  const MIN_LEAF = 7;

  function splitNode(x, y, w, h, depth, maxDepth, force) {
    const node = { x, y, w, h, children: null, room: null };
    const canW = w > MIN_LEAF * 2 + 1;
    const canH = h > MIN_LEAF * 2 + 1;
    // At depth 0, `force` guarantees the root always splits at least once
    // (used as a fallback so a floor never ends up with a single room).
    const splitChance = force && depth === 0 ? 1 : 0.88;
    if (depth < maxDepth && (canW || canH) && Math.random() < splitChance) {
      const vertical = canW && canH ? Math.random() < 0.5 : canW;
      if (vertical) {
        const splitX = randInt(x + MIN_LEAF, x + w - MIN_LEAF);
        node.children = [
          splitNode(x, y, splitX - x, h, depth + 1, maxDepth, force),
          splitNode(splitX, y, x + w - splitX, h, depth + 1, maxDepth, force),
        ];
      } else {
        const splitY = randInt(y + MIN_LEAF, y + h - MIN_LEAF);
        node.children = [
          splitNode(x, y, w, splitY - y, depth + 1, maxDepth, force),
          splitNode(x, splitY, w, y + h - splitY, depth + 1, maxDepth, force),
        ];
      }
    }
    return node;
  }

  function carveRoomsAndConnect(node, grid) {
    if (node.children) {
      const [a, b] = node.children;
      carveRoomsAndConnect(a, grid);
      carveRoomsAndConnect(b, grid);
      if (a.room && b.room) {
        carveCorridor(grid, a.room, b.room);
        node.room = Math.random() < 0.5 ? a.room : b.room;
      } else {
        node.room = a.room || b.room;
      }
      return;
    }
    // Leaf: carve a room inside, leaving at least 1 tile margin for walls.
    const rw = randInt(Math.max(4, node.w - 4), Math.max(4, node.w - 2));
    const rh = randInt(Math.max(4, node.h - 4), Math.max(4, node.h - 2));
    const rx = node.x + randInt(1, Math.max(1, node.w - rw - 1));
    const ry = node.y + randInt(1, Math.max(1, node.h - rh - 1));
    const room = { x: rx, y: ry, w: rw, h: rh };
    for (let y = room.y; y < room.y + room.h; y++) {
      for (let x = room.x; x < room.x + room.w; x++) grid[y][x] = TILE.FLOOR;
    }
    node.room = room;
  }

  function roomCenter(r) { return { x: Math.floor(r.x + r.w / 2), y: Math.floor(r.y + r.h / 2) }; }

  function carveCorridor(grid, r1, r2) {
    const c1 = roomCenter(r1), c2 = roomCenter(r2);
    const wide = Math.random() < 0.5;
    if (wide) {
      carveH(grid, c1.x, c2.x, c1.y);
      carveV(grid, c1.y, c2.y, c2.x);
    } else {
      carveV(grid, c1.y, c2.y, c1.x);
      carveH(grid, c1.x, c2.x, c2.y);
    }
  }
  function carveH(grid, x1, x2, y) {
    const rows = grid.length, cols = grid[0].length;
    for (let x = Math.min(x1, x2); x <= Math.max(x1, x2); x++) {
      for (let dy = 0; dy <= 1; dy++) {
        const yy = clamp(y + dy, 0, rows - 1);
        if (x >= 0 && x < cols) grid[yy][x] = TILE.FLOOR;
      }
    }
  }
  function carveV(grid, y1, y2, x) {
    const rows = grid.length, cols = grid[0].length;
    for (let y = Math.min(y1, y2); y <= Math.max(y1, y2); y++) {
      for (let dx = 0; dx <= 1; dx++) {
        const xx = clamp(x + dx, 0, cols - 1);
        if (y >= 0 && y < rows) grid[y][xx] = TILE.FLOOR;
      }
    }
  }

  function collectRooms(node, out) {
    if (node.children) {
      collectRooms(node.children[0], out);
      collectRooms(node.children[1], out);
    } else if (node.room) {
      out.push(node.room);
    }
  }

  // BFS over floor tiles from a start cell -> distance map (for finding
  // the tile farthest from the player, used to place the exit).
  function bfsDistances(grid, startX, startY) {
    const rows = grid.length, cols = grid[0].length;
    const dist = makeGrid(cols, rows, -1);
    dist[startY][startX] = 0;
    const queue = [[startX, startY]];
    let head = 0;
    const dirs = [[1, 0], [-1, 0], [0, 1], [0, -1]];
    while (head < queue.length) {
      const [x, y] = queue[head++];
      for (const [dx, dy] of dirs) {
        const nx = x + dx, ny = y + dy;
        if (nx < 0 || ny < 0 || nx >= cols || ny >= rows) continue;
        if (grid[ny][nx] === TILE.WALL) continue;
        if (dist[ny][nx] !== -1) continue;
        dist[ny][nx] = dist[y][x] + 1;
        queue.push([nx, ny]);
      }
    }
    return dist;
  }

  function randomFloorTileInRoom(grid, room, avoid) {
    for (let tries = 0; tries < 30; tries++) {
      const x = randInt(room.x, room.x + room.w - 1);
      const y = randInt(room.y, room.y + room.h - 1);
      if (grid[y][x] === TILE.FLOOR && (!avoid || Math.hypot(x - avoid.x, y - avoid.y) > 2)) {
        return { x, y };
      }
    }
    return roomCenter(room);
  }

  // --- Difficulty scaling ---------------------------------------------
  function enemyBudgetForFloor(floor) {
    return Math.round(4 + floor * 1.6);
  }

  function eliteChanceForFloor(floor) {
    return clamp(0.04 + floor * 0.015, 0.04, 0.4);
  }

  // --- Public: generate a full floor -----------------------------------
  function generateFloor(floor) {
    const isBossFloor = floor % 5 === 0;
    const dim = isBossFloor
      ? clamp(19 + Math.floor(floor / 10) * 2, 19, 29)
      : clamp(23 + Math.floor(floor / 4) * 2, 23, 45);
    const cols = dim, rows = dim;
    const grid = makeGrid(cols, rows, TILE.WALL);
    const maxDepth = isBossFloor ? 2 : clamp(4 + Math.floor(floor / 6), 4, 6);

    // Regenerate until the split actually produced at least 2 rooms — a lone
    // root room would make start === exit and the floor would auto-skip.
    let root, rooms;
    for (let attempt = 0; attempt < 20; attempt++) {
      for (let y = 0; y < rows; y++) grid[y].fill(TILE.WALL);
      root = splitNode(1, 1, cols - 2, rows - 2, 0, maxDepth, attempt >= 3);
      carveRoomsAndConnect(root, grid);
      rooms = [];
      collectRooms(root, rooms);
      if (rooms.length >= 2) break;
    }

    // Sort rooms roughly by area, biggest first — first room = start room.
    rooms.sort((a, b) => (b.w * b.h) - (a.w * a.h));
    const startRoom = rooms[0];
    const start = roomCenter(startRoom);
    const dist = bfsDistances(grid, start.x, start.y);

    // Exit = farthest reachable room center from the start.
    let exitRoom = startRoom, exitDist = -1;
    for (const r of rooms) {
      const c = roomCenter(r);
      const d = dist[c.y] ? dist[c.y][c.x] : -1;
      if (d > exitDist) { exitDist = d; exitRoom = r; }
    }
    const exit = roomCenter(exitRoom);

    // Populate rooms with enemies / chests, skipping the start room.
    const enemySpawns = [];
    const chestSpawns = [];
    let budget = enemyBudgetForFloor(floor);
    const populatable = rooms.filter(r => r !== startRoom);
    const eliteChance = eliteChanceForFloor(floor);

    if (isBossFloor) {
      const bossTile = randomFloorTileInRoom(grid, exitRoom, exit);
      enemySpawns.push({ x: bossTile.x, y: bossTile.y, boss: true });
      chestSpawns.push({ x: exit.x, y: exit.y - 1 >= exitRoom.y ? exit.y - 1 : exit.y, guaranteed: true });
    } else {
      for (const room of populatable) {
        if (budget <= 0) break;
        const area = room.w * room.h;
        const count = clamp(Math.round(area / 26) + randInt(0, 1), 0, 4);
        for (let i = 0; i < count && budget > 0; i++) {
          const tile = randomFloorTileInRoom(grid, room);
          enemySpawns.push({ x: tile.x, y: tile.y, elite: Math.random() < eliteChance });
          budget--;
        }
        if (Math.random() < 0.35) {
          const tile = randomFloorTileInRoom(grid, room);
          chestSpawns.push({ x: tile.x, y: tile.y });
        }
      }
      // Guarantee at least one chest per floor.
      if (chestSpawns.length === 0 && populatable.length) {
        const room = choice(populatable);
        const tile = randomFloorTileInRoom(grid, room);
        chestSpawns.push({ x: tile.x, y: tile.y });
      }
    }

    return {
      floor, cols, rows, grid, rooms,
      start, exit, isBossFloor,
      enemySpawns, chestSpawns,
      tileWall: TILE.WALL, tileFloor: TILE.FLOOR,
    };
  }

  global.KXDungeon = { generateFloor, TILE };
})(typeof window !== 'undefined' ? window : globalThis);

if (typeof module !== 'undefined' && module.exports) {
  module.exports = globalThis.KXDungeon;
}
