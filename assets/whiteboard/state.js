// Board state: shapes, multi-selection, tool, style, sticky category.

let shapes = [];
let selectedIds = new Set();
let currentTool = 'select';
let currentStroke = '#000000';
let currentFill = '#ffffff';
let currentFillNone = false;
let currentStrokeWidth = 2;
let currentStickyCategory = 'yellow';
let _nextId = 1;

// ── IDs ───────────────────────────────────────────────────────────────────────

export function nextId() {
  return 's' + (_nextId++);
}

// ── Shapes CRUD ───────────────────────────────────────────────────────────────

export function addShape(shape) {
  shapes.push(shape);
}

export function getShape(id) {
  return shapes.find(s => s.id === id) || null;
}

export function updateShape(id, props) {
  const s = getShape(id);
  if (s) Object.assign(s, props);
}

export function removeShape(id) {
  shapes = shapes.filter(s => s.id !== id);
  selectedIds.delete(id);
  // Detach connectors that referenced this shape (convert to free endpoint)
  for (const s of shapes) {
    if (s.type === 'connector') {
      if (s.sourceId === id) s.sourceId = null;
      if (s.targetId === id) s.targetId = null;
    }
  }
}

export function getShapes() {
  return shapes;
}

export function setShapes(arr) {
  shapes = arr;
  selectedIds = new Set();
}

export function clearShapes() {
  shapes = [];
  selectedIds = new Set();
}

// ── Selection (multi-select) ──────────────────────────────────────────────────

export function setSelected(id) {
  selectedIds = new Set();
  if (id) selectedIds.add(id);
}

export function addSelected(id) {
  if (id) selectedIds.add(id);
}

export function toggleSelected(id) {
  if (selectedIds.has(id)) selectedIds.delete(id);
  else selectedIds.add(id);
}

export function isSelected(id) {
  return selectedIds.has(id);
}

// Backward-compat: returns first selected ID or null
export function getSelected() {
  return selectedIds.size > 0 ? selectedIds.values().next().value : null;
}

export function getSelectedIds() {
  return new Set(selectedIds);
}

export function getSelectedShapes() {
  return shapes.filter(s => selectedIds.has(s.id));
}

export function clearSelection() {
  selectedIds = new Set();
}

// ── Tool ──────────────────────────────────────────────────────────────────────

export function setTool(tool) { currentTool = tool; }
export function getTool()     { return currentTool; }

// ── Style ─────────────────────────────────────────────────────────────────────

export function setStroke(color)  { currentStroke = color; }
export function getStroke()       { return currentStroke; }
export function setFill(color)    { currentFill = color; }
export function getFill()         { return currentFill; }
export function setFillNone(val)  { currentFillNone = val; }
export function getFillNone()     { return currentFillNone; }
export function setStrokeWidth(w) { currentStrokeWidth = w; }
export function getStrokeWidth()  { return currentStrokeWidth; }

export function currentStyle() {
  return {
    stroke: currentStroke,
    fill: currentFillNone ? 'none' : currentFill,
    strokeWidth: currentStrokeWidth,
  };
}

// ── Sticky note category ──────────────────────────────────────────────────────

export function setStickyCategory(cat) { currentStickyCategory = cat; }
export function getStickyCategory()    { return currentStickyCategory; }

// ── Board export / import ─────────────────────────────────────────────────────

export function exportBoard() {
  return {
    version: 1,
    shapes: JSON.parse(JSON.stringify(shapes)),
  };
}

export function importBoard(doc) {
  if (!doc || !Array.isArray(doc.shapes)) return false;
  // Remap all IDs to avoid collisions with any future shapes
  const idMap = {};
  for (const s of doc.shapes) {
    const newId = nextId();
    idMap[s.id] = newId;
    s.id = newId;
  }
  for (const s of doc.shapes) {
    if (s.sourceId && idMap[s.sourceId]) s.sourceId = idMap[s.sourceId];
    if (s.targetId && idMap[s.targetId]) s.targetId = idMap[s.targetId];
  }
  shapes = doc.shapes;
  selectedIds = new Set();
  return true;
}
