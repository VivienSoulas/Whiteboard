import * as state from '../state.js';
import * as renderer from '../renderer.js';

let drawing = false;
let startPt = null;
let currentId = null;

export function onDown(pt, e) {
  drawing = true;
  startPt = pt;
  const style = state.currentStyle();
  const shape = {
    id: state.nextId(),
    type: 'line',
    x: pt.x, y: pt.y,
    x2: pt.x, y2: pt.y,
    ...style,
  };
  currentId = shape.id;
  state.addShape(shape);
  state.setSelected(shape.id);
  renderer.render();
}

export function onMove(pt, e) {
  if (!drawing || !currentId) return;
  state.updateShape(currentId, { x2: pt.x, y2: pt.y });
  renderer.render();
}

export function onUp(pt, e) {
  drawing = false;
  const s = state.getShape(currentId);
  if (s) {
    const dx = s.x2 - s.x, dy = s.y2 - s.y;
    if (Math.sqrt(dx*dx + dy*dy) < 2) {
      state.removeShape(currentId);
      renderer.render();
    }
  }
  currentId = null;
  startPt = null;
}
