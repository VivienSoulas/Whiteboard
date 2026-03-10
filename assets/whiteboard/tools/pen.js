import * as state from '../state.js';
import * as renderer from '../renderer.js';

let drawing = false;
let currentId = null;

export function onDown(pt, e) {
  drawing = true;
  const style = state.currentStyle();
  const shape = {
    id: state.nextId(),
    type: 'pen',
    x: pt.x, y: pt.y,
    points: [{ x: pt.x, y: pt.y }],
    ...style,
  };
  currentId = shape.id;
  state.addShape(shape);
  state.setSelected(shape.id);
  renderer.render();
}

export function onMove(pt, e) {
  if (!drawing || !currentId) return;
  const s = state.getShape(currentId);
  if (!s) return;
  const pts = [...s.points, { x: pt.x, y: pt.y }];
  state.updateShape(currentId, { points: pts });
  renderer.render();
}

export function onUp(pt, e) {
  drawing = false;
  const s = state.getShape(currentId);
  if (s && s.points.length < 2) {
    state.removeShape(currentId);
    renderer.render();
  }
  currentId = null;
}
