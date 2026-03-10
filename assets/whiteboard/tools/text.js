import * as state from '../state.js';
import * as renderer from '../renderer.js';

let currentId = null;

export function onDown(pt, e) {
  // If clicking on an existing text shape's foreignObject, let it focus naturally
  if (e.target && e.target.closest && e.target.closest('.text-fo')) return;

  const style = state.currentStyle();
  const shape = {
    id: state.nextId(),
    type: 'text',
    x: pt.x, y: pt.y,
    width: 200, height: 40,
    text: '',
    ...style,
  };
  currentId = shape.id;
  state.addShape(shape);
  state.setSelected(shape.id);
  renderer.render();

  // Focus the newly created text element
  setTimeout(() => {
    const el = document.querySelector(`[data-shape-id="${shape.id}"] div`);
    if (el) el.focus();
  }, 0);
}

export function onMove(pt, e) {}
export function onUp(pt, e) {}

// Wire up text input tracking after renderer creates the element
export function attachTextInput(shapeId, onInput) {
  const el = document.querySelector(`[data-shape-id="${shapeId}"] div`);
  if (!el) return;
  el.addEventListener('input', () => {
    onInput(shapeId, el.innerText);
  });
}
