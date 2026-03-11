const MAX_HISTORY = 50;

let undoStack = [];
let redoStack = [];

function clone(shapes) {
  return structuredClone(shapes);
}

export function push(shapes) {
  undoStack.push(clone(shapes));
  if (undoStack.length > MAX_HISTORY) undoStack.shift();
  redoStack = [];
  document.dispatchEvent(new CustomEvent('historychange'));
}

export function undo(currentShapes) {
  if (undoStack.length === 0) return null;
  redoStack.push(clone(currentShapes));
  return undoStack.pop();
}

export function redo(currentShapes) {
  if (redoStack.length === 0) return null;
  undoStack.push(clone(currentShapes));
  return redoStack.pop();
}

export function canUndo() { return undoStack.length > 0; }
export function canRedo() { return redoStack.length > 0; }

export function clear() {
  undoStack = [];
  redoStack = [];
}
