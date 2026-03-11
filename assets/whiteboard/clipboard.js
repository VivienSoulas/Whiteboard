const PASTE_OFFSET = 20;

let _clipboard = [];

export function copy(shapes) {
  if (!shapes || shapes.length === 0) return;
  _clipboard = JSON.parse(JSON.stringify(shapes));
}

export function paste(nextIdFn) {
  if (_clipboard.length === 0) return [];
  const idMap = {};
  const pasted = _clipboard.map(s => {
    const c = JSON.parse(JSON.stringify(s));
    const newId = nextIdFn();
    idMap[s.id] = newId;
    c.id = newId;
    return c;
  });
  // remap connector references within pasted group
  for (const s of pasted) {
    if (s.sourceId && idMap[s.sourceId]) s.sourceId = idMap[s.sourceId];
    if (s.targetId && idMap[s.targetId]) s.targetId = idMap[s.targetId];
  }
  // offset so result is visible
  for (const s of pasted) {
    if (s.x != null) s.x += PASTE_OFFSET;
    if (s.y != null) s.y += PASTE_OFFSET;
    if (s.x2 != null) s.x2 += PASTE_OFFSET;
    if (s.y2 != null) s.y2 += PASTE_OFFSET;
    if (s.points) s.points = s.points.map(p => ({ x: p.x + PASTE_OFFSET, y: p.y + PASTE_OFFSET }));
  }
  return pasted;
}

export function hasContent() { return _clipboard.length > 0; }
