// Shape store, selection state, current tool/color

let shapes = [];
let selectedId = null;
let currentTool = 'select';
let currentStroke = '#000000';
let currentFill = '#ffffff';
let currentFillNone = false;
let currentStrokeWidth = 2;

let _nextId = 1;

export function nextId() {
  return 's' + (_nextId++);
}

// Shape CRUD
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
}

export function getShapes() {
  return shapes;
}

export function clearShapes() {
  shapes = [];
  selectedId = null;
}

// Selection
export function setSelected(id) {
  selectedId = id;
}

export function getSelected() {
  return selectedId;
}

export function clearSelection() {
  selectedId = null;
}

// Tool
export function setTool(tool) {
  currentTool = tool;
}

export function getTool() {
  return currentTool;
}

// Color / style
export function setStroke(color) {
  currentStroke = color;
}

export function getStroke() {
  return currentStroke;
}

export function setFill(color) {
  currentFill = color;
}

export function getFill() {
  return currentFill;
}

export function setFillNone(val) {
  currentFillNone = val;
}

export function getFillNone() {
  return currentFillNone;
}

export function setStrokeWidth(w) {
  currentStrokeWidth = w;
}

export function getStrokeWidth() {
  return currentStrokeWidth;
}

export function currentStyle() {
  return {
    stroke: currentStroke,
    fill: currentFillNone ? 'none' : currentFill,
    strokeWidth: currentStrokeWidth,
  };
}
