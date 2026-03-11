// Connector tool: click on a shape to start, drag to another shape to finish.
// Supports box-like shapes and sticky notes as connection targets.

import * as state from '../state.js';
import * as renderer from '../renderer.js';
import * as history from '../history.js';
import { nearestAnchor, getAnchorPoint } from '../shapebounds.js';

let drawing = false;
let currentId = null;
let svgEl = null;

const CONNECTABLE_TYPES = new Set(['rectangle', 'ellipse', 'text', 'stickynote']);

export function init(svg) {
  svgEl = svg;
}

export function onDown(pt, e) {
  // Find a shape to start from
  const src = _findConnectableShape(e.target);
  history.push(state.getShapes());
  const style = state.currentStyle();
  const shape = {
    id: state.nextId(),
    type: 'connector',
    sourceId: src ? src.id : null,
    sourceAnchor: src ? nearestAnchor(src, pt) : 'right',
    targetId: null,
    targetAnchor: 'left',
    x: src ? getAnchorPoint(src, nearestAnchor(src, pt)).x : pt.x,
    y: src ? getAnchorPoint(src, nearestAnchor(src, pt)).y : pt.y,
    x2: pt.x,
    y2: pt.y,
    startMarker: 'none',
    endMarker: 'arrow',
    stroke: style.stroke,
    strokeWidth: style.strokeWidth,
  };
  currentId = shape.id;
  state.addShape(shape);
  state.setSelected(shape.id);
  drawing = true;
  renderer.render();
}

export function onMove(pt, e) {
  if (!drawing || !currentId) return;
  state.updateShape(currentId, { x2: pt.x, y2: pt.y });
  renderer.render();
}

export function onUp(pt, e) {
  if (!drawing || !currentId) return;
  drawing = false;

  // Try to attach to a target shape
  const tgt = _findConnectableShape(e.target);
  const s = state.getShape(currentId);
  if (!s) { currentId = null; return; }

  if (tgt && (!s.sourceId || tgt.id !== s.sourceId)) {
    const anchor = nearestAnchor(tgt, pt);
    state.updateShape(currentId, { targetId: tgt.id, targetAnchor: anchor });
  }

  // Remove if start and end are the same point (too short)
  const final = state.getShape(currentId);
  if (final) {
    const dx = final.x2 - final.x, dy = final.y2 - final.y;
    if (!final.targetId && Math.sqrt(dx * dx + dy * dy) < 5) {
      state.removeShape(currentId);
      state.clearSelection();
    }
  }

  currentId = null;
  renderer.render();
}

function _findConnectableShape(target) {
  let el = target;
  while (el) {
    if (el.dataset && el.dataset.shapeId) {
      const s = state.getShape(el.dataset.shapeId);
      if (s && CONNECTABLE_TYPES.has(s.type)) return s;
      return null;
    }
    el = el.parentElement;
  }
  return null;
}
