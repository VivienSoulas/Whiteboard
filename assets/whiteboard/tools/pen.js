import * as state from '../state.js';
import * as renderer from '../renderer.js';
import * as history from '../history.js';

let drawing = false;
let currentId = null;

export function onDown(pt, e) {
  history.push(state.getShapes());
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
  if (s) {
    if (s.points.length < 2) {
      state.removeShape(currentId);
    } else {
      state.updateShape(currentId, { points: _simplify(s.points, 1.5) });
    }
  }
  renderer.render();
  currentId = null;
}

// Ramer-Douglas-Peucker polyline simplification
function _simplify(pts, tol) {
  if (pts.length <= 2) return pts;
  let maxDist = 0, maxIdx = 0;
  const x1 = pts[0].x, y1 = pts[0].y;
  const x2 = pts[pts.length - 1].x, y2 = pts[pts.length - 1].y;
  const dx = x2 - x1, dy = y2 - y1;
  const len = Math.sqrt(dx * dx + dy * dy);
  for (let i = 1; i < pts.length - 1; i++) {
    const d = len === 0
      ? Math.hypot(pts[i].x - x1, pts[i].y - y1)
      : Math.abs(dy * pts[i].x - dx * pts[i].y + x2 * y1 - y2 * x1) / len;
    if (d > maxDist) { maxDist = d; maxIdx = i; }
  }
  if (maxDist > tol) {
    const left  = _simplify(pts.slice(0, maxIdx + 1), tol);
    const right = _simplify(pts.slice(maxIdx), tol);
    return [...left.slice(0, -1), ...right];
  }
  return [pts[0], pts[pts.length - 1]];
}
