import * as state from '../state.js';
import * as renderer from '../renderer.js';
import * as RectShape from '../shapes/rectangle.js';
import * as history from '../history.js';

let drawing = false;
let startPt = null;
let currentId = null;

export function onDown(pt, e) {
  history.push(state.getShapes());
  drawing = true;
  startPt = pt;
  const style = state.currentStyle();
  const shape = {
    id: state.nextId(),
    type: 'rectangle',
    x: pt.x, y: pt.y,
    width: 0, height: 0,
    ...style,
  };
  currentId = shape.id;
  state.addShape(shape);
  state.setSelected(shape.id);
  renderer.render();
}

export function onMove(pt, e) {
  if (!drawing || !currentId) return;
  state.updateShape(currentId, {
    width:  pt.x - startPt.x,
    height: pt.y - startPt.y,
  });
  renderer.render();
}

export function onUp(pt, e) {
  drawing = false;
  const s = state.getShape(currentId);
  if (s && Math.abs(s.width) < 2 && Math.abs(s.height) < 2) {
    state.removeShape(currentId);
    renderer.render();
  }
  currentId = null;
  startPt = null;
}
