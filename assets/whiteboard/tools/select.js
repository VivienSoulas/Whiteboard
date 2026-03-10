import * as state from '../state.js';
import * as renderer from '../renderer.js';
import * as RectShape   from '../shapes/rectangle.js';
import * as EllipseShape from '../shapes/ellipse.js';
import * as LineShape   from '../shapes/line.js';
import * as ArrowShape  from '../shapes/arrow.js';
import * as PenShape    from '../shapes/pen.js';
import * as TextShape   from '../shapes/text.js';

const boundsMap = {
  rectangle: RectShape,
  ellipse:   EllipseShape,
  line:      LineShape,
  arrow:     ArrowShape,
  pen:       PenShape,
  text:      TextShape,
};

let dragging = false;
let resizing = false;
let handle = null;
let startPt = null;
let origShape = null;
let svgEl = null;

export function init(svg) {
  svgEl = svg;
}

export function onDown(pt, e) {
  const target = e.target;

  // Clicked a resize handle?
  if (target.classList.contains('resize-handle')) {
    const id = state.getSelected();
    if (!id) return;
    handle = target.dataset.handle;
    origShape = Object.assign({}, state.getShape(id));
    // copy points array if present
    if (origShape.points) origShape.points = origShape.points.map(p => ({...p}));
    startPt = pt;
    resizing = true;
    return;
  }

  // Clicked a shape?
  const shapeEl = findShapeEl(target);
  if (shapeEl) {
    const id = shapeEl.dataset.shapeId;
    state.setSelected(id);
    origShape = Object.assign({}, state.getShape(id));
    if (origShape.points) origShape.points = origShape.points.map(p => ({...p}));
    startPt = pt;
    dragging = true;
    renderer.renderSelection();
    return;
  }

  // Clicked empty canvas — deselect
  state.clearSelection();
  renderer.renderSelection();
}

export function onMove(pt, e) {
  if (!startPt) return;

  const dx = pt.x - startPt.x;
  const dy = pt.y - startPt.y;
  const id = state.getSelected();
  if (!id) return;

  if (dragging) {
    const s = state.getShape(id);
    if (!s) return;
    const updates = { x: origShape.x + dx, y: origShape.y + dy };
    if (s.type === 'line' || s.type === 'arrow') {
      updates.x2 = origShape.x2 + dx;
      updates.y2 = origShape.y2 + dy;
    }
    if (s.type === 'pen') {
      updates.points = origShape.points.map(p => ({ x: p.x + dx, y: p.y + dy }));
    }
    state.updateShape(id, updates);
    renderer.render();
  }

  if (resizing) {
    applyResize(id, pt, dx, dy);
    renderer.render();
  }
}

export function onUp(pt, e) {
  dragging = false;
  resizing = false;
  startPt = null;
  origShape = null;
  handle = null;
}

export function onKey(e) {
  if (e.key === 'Delete' || e.key === 'Backspace') {
    const id = state.getSelected();
    if (id) {
      state.removeShape(id);
      state.clearSelection();
      renderer.render();
    }
  }
}

function applyResize(id, pt, dx, dy) {
  const s = state.getShape(id);
  if (!s) return;
  const o = origShape;

  if (s.type === 'line' || s.type === 'arrow') {
    if (handle === 'nw') { state.updateShape(id, { x: o.x + dx, y: o.y + dy }); }
    else if (handle === 'se') { state.updateShape(id, { x2: o.x2 + dx, y2: o.y2 + dy }); }
    return;
  }

  let { x, y, width, height } = o;

  if (handle.includes('e')) width += dx;
  if (handle.includes('s')) height += dy;
  if (handle.includes('w')) { x += dx; width -= dx; }
  if (handle.includes('n')) { y += dy; height -= dy; }

  state.updateShape(id, { x, y, width, height });
}

function findShapeEl(target) {
  let el = target;
  while (el && el !== svgEl) {
    if (el.dataset && el.dataset.shapeId) return el;
    el = el.parentElement;
  }
  return null;
}
