// Connector: a line between two shapes (or free endpoints).
// Endpoints are resolved from source/target shape bounds on every render.
// Only connects to box-like shapes and sticky notes (not pen, not other connectors).

import * as state from '../state.js';
import { getAnchorPoint } from '../shapebounds.js';
import { syncMarkers } from '../markers.js';

const NS = 'http://www.w3.org/2000/svg';

export function create(shape) {
  const g = document.createElementNS(NS, 'g');
  const line = document.createElementNS(NS, 'line');
  g.appendChild(line);
  update(g, shape);
  return g;
}

export function update(el, shape) {
  const { x1, y1, x2, y2 } = _resolveEndpoints(shape);

  const { startId, endId } = syncMarkers(el, shape.id, shape);

  const line = el.querySelector('line');
  line.setAttribute('x1', x1);
  line.setAttribute('y1', y1);

  // Shorten line end slightly when there's an end marker to avoid overlap
  if (endId) {
    const dx = x2 - x1, dy = y2 - y1;
    const len = Math.sqrt(dx * dx + dy * dy) || 1;
    const offset = Math.min(12, len * 0.1);
    line.setAttribute('x2', x2 - (dx / len) * offset);
    line.setAttribute('y2', y2 - (dy / len) * offset);
  } else {
    line.setAttribute('x2', x2);
    line.setAttribute('y2', y2);
  }

  // Shorten line start slightly when there's a start marker
  if (startId) {
    const dx = x1 - x2, dy = y1 - y2;
    const len = Math.sqrt(dx * dx + dy * dy) || 1;
    const offset = Math.min(12, len * 0.1);
    line.setAttribute('x1', x1 - (dx / len) * offset);
    line.setAttribute('y1', y1 - (dy / len) * offset);
  }

  line.setAttribute('stroke', shape.stroke || '#000000');
  line.setAttribute('stroke-width', shape.strokeWidth || 2);
  line.setAttribute('fill', 'none');
  if (startId) line.setAttribute('marker-start', `url(#${startId})`);
  else line.removeAttribute('marker-start');
  if (endId) line.setAttribute('marker-end', `url(#${endId})`);
  else line.removeAttribute('marker-end');
}

export function bounds(shape) {
  const { x1, y1, x2, y2 } = _resolveEndpoints(shape);
  return {
    x: Math.min(x1, x2), y: Math.min(y1, y2),
    width:  Math.max(Math.abs(x2 - x1), 1),
    height: Math.max(Math.abs(y2 - y1), 1),
  };
}

function _resolveEndpoints(shape) {
  let x1 = shape.x || 0, y1 = shape.y || 0;
  let x2 = shape.x2 || 0, y2 = shape.y2 || 0;

  if (shape.sourceId) {
    const src = state.getShape(shape.sourceId);
    if (src) {
      const pt = getAnchorPoint(src, shape.sourceAnchor || 'right');
      x1 = pt.x; y1 = pt.y;
    }
  }
  if (shape.targetId) {
    const tgt = state.getShape(shape.targetId);
    if (tgt) {
      const pt = getAnchorPoint(tgt, shape.targetAnchor || 'left');
      x2 = pt.x; y2 = pt.y;
    }
  }
  return { x1, y1, x2, y2 };
}
