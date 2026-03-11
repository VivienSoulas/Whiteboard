import * as state from '../state.js';
import * as renderer from '../renderer.js';
import * as history from '../history.js';

let currentId = null;

export function onDown(pt, e) {
  // If clicking inside an existing sticky note, let it focus naturally
  if (e.target && e.target.closest && e.target.closest('.sticky-fo')) return;

  history.push(state.getShapes());
  const style = state.currentStyle();
  const category = state.getStickyCategory();
  const shape = {
    id: state.nextId(),
    type: 'stickynote',
    x: pt.x, y: pt.y,
    width: 150, height: 150,
    text: '',
    category,
    ...style,
  };
  currentId = shape.id;
  state.addShape(shape);
  state.setSelected(shape.id);
  renderer.render();

  setTimeout(() => {
    const el = document.querySelector(`[data-shape-id="${shape.id}"] .sticky-fo div`);
    if (el) el.focus();
  }, 0);
}

export function onMove(pt, e) {}
export function onUp(pt, e) {}
