// SVG marker management for line endings.
// Markers are created per shape (like the existing arrow approach) to support per-shape colors.

const NS = 'http://www.w3.org/2000/svg';

export const MARKER_TYPES = ['none', 'arrow', 'open-arrow', 'circle', 'diamond'];

const DEFS = {
  arrow: {
    viewBox: '0 0 10 7', refX: 10, refY: 3.5,
    markerWidth: 10, markerHeight: 7,
    create(color) {
      const poly = document.createElementNS(NS, 'polygon');
      poly.setAttribute('points', '0 0, 10 3.5, 0 7');
      poly.setAttribute('fill', color);
      return poly;
    },
    update(el, color) { el.querySelector('polygon').setAttribute('fill', color); },
  },
  'open-arrow': {
    viewBox: '0 0 10 7', refX: 10, refY: 3.5,
    markerWidth: 10, markerHeight: 7,
    create(color) {
      const poly = document.createElementNS(NS, 'polyline');
      poly.setAttribute('points', '0 0, 10 3.5, 0 7');
      poly.setAttribute('fill', 'none');
      poly.setAttribute('stroke', color);
      poly.setAttribute('stroke-width', '1.5');
      return poly;
    },
    update(el, color) {
      const p = el.querySelector('polyline');
      p.setAttribute('stroke', color);
    },
  },
  circle: {
    viewBox: '0 0 8 8', refX: 4, refY: 4,
    markerWidth: 8, markerHeight: 8,
    create(color) {
      const c = document.createElementNS(NS, 'circle');
      c.setAttribute('cx', '4'); c.setAttribute('cy', '4'); c.setAttribute('r', '3.5');
      c.setAttribute('fill', color);
      return c;
    },
    update(el, color) { el.querySelector('circle').setAttribute('fill', color); },
  },
  diamond: {
    viewBox: '0 0 12 8', refX: 12, refY: 4,
    markerWidth: 12, markerHeight: 8,
    create(color) {
      const poly = document.createElementNS(NS, 'polygon');
      poly.setAttribute('points', '0 4, 6 0, 12 4, 6 8');
      poly.setAttribute('fill', color);
      return poly;
    },
    update(el, color) { el.querySelector('polygon').setAttribute('fill', color); },
  },
};

function makeMarker(id, type, color, isStart) {
  const def = DEFS[type];
  if (!def) return null;
  const m = document.createElementNS(NS, 'marker');
  m.setAttribute('id', id);
  m.setAttribute('viewBox', def.viewBox);
  m.setAttribute('refX', def.refX);
  m.setAttribute('refY', def.refY);
  m.setAttribute('markerWidth', def.markerWidth);
  m.setAttribute('markerHeight', def.markerHeight);
  m.setAttribute('orient', isStart ? 'auto-start-reverse' : 'auto');
  m.appendChild(def.create(color));
  return m;
}

// Ensure start+end marker elements exist inside a <g> element's embedded <defs>.
// Each shape manages its own markers inside its <g> to keep things self-contained.
export function syncMarkers(gEl, shapeId, shape) {
  const startType = shape.startMarker || 'none';
  // arrow shapes default to end arrow for backward compat
  const endType = shape.endMarker !== undefined ? shape.endMarker : (shape.type === 'arrow' ? 'arrow' : 'none');
  const color = shape.stroke || '#000000';

  const startId = `m-${shapeId}-s`;
  const endId   = `m-${shapeId}-e`;

  let defsEl = gEl.querySelector('defs');
  if (!defsEl) {
    defsEl = document.createElementNS(NS, 'defs');
    gEl.insertBefore(defsEl, gEl.firstChild);
  }

  _syncMarker(defsEl, startId, startType, color, true);
  _syncMarker(defsEl, endId, endType, color, false);

  return { startId: startType !== 'none' ? startId : null, endId: endType !== 'none' ? endId : null };
}

function _syncMarker(defsEl, id, type, color, isStart) {
  let existing = defsEl.querySelector(`#${id}`);
  if (type === 'none') {
    if (existing) existing.remove();
    return;
  }
  const def = DEFS[type];
  if (!def) return;
  if (existing) {
    // Update color
    def.update(existing, color);
  } else {
    const m = makeMarker(id, type, color, isStart);
    if (m) defsEl.appendChild(m);
  }
}
