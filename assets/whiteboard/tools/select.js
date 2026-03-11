import * as state     from '../state.js';
import * as renderer  from '../renderer.js';
import * as history   from '../history.js';
import * as clipboard from '../clipboard.js';
import { getBounds }  from '../shapebounds.js';

const NS = 'http://www.w3.org/2000/svg';

let _dragging   = false;
let _resizing   = false;
let _marquee    = false;
let _handle     = null;
let _startPt    = null;
let _origShapes = null;  // { id: cloned-shape-obj }
let _svgEl      = null;
let _marqueeEl  = null;

export function init(svg) {
  _svgEl = svg;
}

export function onDown(pt, e) {
  const target = e.target;

  // Resize handle
  if (target.classList.contains('resize-handle')) {
    const id = state.getSelected();
    if (!id) return;
    _handle = target.dataset.handle;
    _origShapes = _snapshot([id]);
    _startPt = pt;
    _resizing = true;
    history.push(state.getShapes());
    return;
  }

  // Shape element
  const shapeEl = _findShapeEl(target);
  if (shapeEl) {
    const id = shapeEl.dataset.shapeId;
    if (e.shiftKey || e.ctrlKey || e.metaKey) {
      state.toggleSelected(id);
    } else {
      if (!state.isSelected(id)) state.setSelected(id);
    }
    _origShapes = _snapshot([...state.getSelectedIds()]);
    _startPt = pt;
    _dragging = true;
    history.push(state.getShapes());
    renderer.renderSelection();
    return;
  }

  // Empty canvas → deselect + start rubber-band
  if (!e.shiftKey && !e.ctrlKey && !e.metaKey) state.clearSelection();
  renderer.renderSelection();
  _startPt = pt;
  _marquee = true;
  _marqueeEl = _createMarqueeEl(pt);
  const selLayer = _svgEl.querySelector('#selection-layer');
  if (selLayer) selLayer.appendChild(_marqueeEl);
}

export function onMove(pt, e) {
  if (!_startPt) return;
  const dx = pt.x - _startPt.x;
  const dy = pt.y - _startPt.y;

  if (_dragging && _origShapes) {
    for (const [id, orig] of Object.entries(_origShapes)) {
      const updates = {};
      if (orig.x != null) updates.x = orig.x + dx;
      if (orig.y != null) updates.y = orig.y + dy;
      if (orig.x2 != null) updates.x2 = orig.x2 + dx;
      if (orig.y2 != null) updates.y2 = orig.y2 + dy;
      if (orig.points) updates.points = orig.points.map(p => ({ x: p.x + dx, y: p.y + dy }));
      state.updateShape(id, updates);
    }
    renderer.render();
    return;
  }

  if (_resizing && _origShapes) {
    const id = state.getSelected();
    if (id) { _applyResize(id, dx, dy); renderer.render(); }
    return;
  }

  if (_marquee && _marqueeEl) {
    _updateMarqueeEl(_marqueeEl, _startPt, pt);
  }
}

export function onUp(pt, e) {
  if (_marquee) {
    _marquee = false;
    if (_marqueeEl) {
      _selectInMarquee(_startPt, pt);
      _marqueeEl.remove();
      _marqueeEl = null;
    }
  }
  _dragging   = false;
  _resizing   = false;
  _startPt    = null;
  _origShapes = null;
  _handle     = null;
}

export function onKey(e) {
  const ctrl = e.ctrlKey || e.metaKey;

  if (ctrl && e.key === 'z' && !e.shiftKey) { e.preventDefault(); _doUndo(); return; }
  if (ctrl && (e.key === 'y' || (e.key === 'z' && e.shiftKey))) { e.preventDefault(); _doRedo(); return; }
  if (ctrl && e.key === 'c') { e.preventDefault(); _doCopy(); return; }
  if (ctrl && e.key === 'v') { e.preventDefault(); _doPaste(); return; }
  if (ctrl && e.key === 'd') { e.preventDefault(); _doDuplicate(); return; }
  if (ctrl && e.key === 'a') {
    e.preventDefault();
    for (const s of state.getShapes()) state.addSelected(s.id);
    renderer.renderSelection();
    return;
  }

  if (e.key === 'Escape') {
    state.clearSelection();
    renderer.renderSelection();
    return;
  }

  if (e.key === 'Delete' || e.key === 'Backspace') {
    const ids = [...state.getSelectedIds()];
    if (ids.length === 0) return;
    history.push(state.getShapes());
    for (const id of ids) state.removeShape(id);
    state.clearSelection();
    renderer.render();
  }
}

// ── Public helpers (called from toolbar.js) ───────────────────────────────────

export function doUndo()     { _doUndo(); }
export function doRedo()     { _doRedo(); }
export function doCopy()     { _doCopy(); }
export function doPaste()    { _doPaste(); }
export function doDuplicate() { _doDuplicate(); }
export function onDblClick(pt, e) {
  const shapeEl = _findShapeEl(e.target);
  if (!shapeEl) return;
  const s = state.getShape(shapeEl.dataset.shapeId);
  if (!s) return;
  if (s.type === 'text') {
    const div = document.querySelector(`[data-shape-id="${s.id}"] div`);
    if (div) div.focus();
  } else if (s.type === 'stickynote') {
    const div = document.querySelector(`[data-shape-id="${s.id}"] .sticky-fo div`);
    if (div) div.focus();
  }
}
function _doUndo() {
  const prev = history.undo(state.getShapes());
  if (prev) { state.setShapes(prev); renderer.render(); }
  document.dispatchEvent(new CustomEvent('historychange'));
}

function _doRedo() {
  const next = history.redo(state.getShapes());
  if (next) { state.setShapes(next); renderer.render(); }
  document.dispatchEvent(new CustomEvent('historychange'));
}

function _doCopy() {
  const sel = state.getSelectedShapes();
  if (sel.length) clipboard.copy(sel);
}

function _doPaste() {
  if (!clipboard.hasContent()) return;
  history.push(state.getShapes());
  const pasted = clipboard.paste(() => state.nextId());
  state.clearSelection();
  for (const s of pasted) { state.addShape(s); state.addSelected(s.id); }
  renderer.render();
}

function _doDuplicate() {
  const sel = state.getSelectedShapes();
  if (sel.length === 0) return;
  clipboard.copy(sel);
  _doPaste();
}

function _applyResize(id, dx, dy) {
  const s = state.getShape(id);
  const o = _origShapes[id];
  if (!s || !o) return;

  if (s.type === 'line' || s.type === 'arrow' || s.type === 'connector') {
    if (_handle === 'nw') state.updateShape(id, { x: o.x + dx, y: o.y + dy });
    else if (_handle === 'se') state.updateShape(id, { x2: o.x2 + dx, y2: o.y2 + dy });
    return;
  }

  const MIN_SIZE = 2;
  let { x, y, width, height } = o;
  if (_handle.includes('e')) width  = Math.max(MIN_SIZE, width  + dx);
  if (_handle.includes('s')) height = Math.max(MIN_SIZE, height + dy);
  if (_handle.includes('w')) { const nw = Math.max(MIN_SIZE, width  - dx); x += width  - nw; width  = nw; }
  if (_handle.includes('n')) { const nh = Math.max(MIN_SIZE, height - dy); y += height - nh; height = nh; }
  state.updateShape(id, { x, y, width, height });
}

function _snapshot(ids) {
  const map = {};
  for (const id of ids) {
    const s = state.getShape(id);
    if (!s) continue;
    map[id] = Object.assign({}, s);
    if (s.points) map[id].points = s.points.map(p => ({ ...p }));
  }
  return map;
}

function _findShapeEl(target) {
  let el = target;
  while (el && el !== _svgEl) {
    if (el.dataset && el.dataset.shapeId) return el;
    el = el.parentElement;
  }
  return null;
}

function _createMarqueeEl(pt) {
  const el = document.createElementNS(NS, 'rect');
  el.setAttribute('class', 'marquee-box');
  el.setAttribute('x', pt.x); el.setAttribute('y', pt.y);
  el.setAttribute('width', 0); el.setAttribute('height', 0);
  return el;
}

function _updateMarqueeEl(el, start, cur) {
  el.setAttribute('x',      Math.min(start.x, cur.x));
  el.setAttribute('y',      Math.min(start.y, cur.y));
  el.setAttribute('width',  Math.abs(cur.x - start.x));
  el.setAttribute('height', Math.abs(cur.y - start.y));
}

function _selectInMarquee(start, end) {
  const mx = Math.min(start.x, end.x), mw = Math.abs(end.x - start.x);
  const my = Math.min(start.y, end.y), mh = Math.abs(end.y - start.y);
  if (mw < 3 && mh < 3) return;

  for (const s of state.getShapes()) {
    const b = getBounds(s);
    if (b.x >= mx && b.y >= my && b.x + b.width <= mx + mw && b.y + b.height <= my + mh) {
      state.addSelected(s.id);
    }
  }
  renderer.renderSelection();
}
