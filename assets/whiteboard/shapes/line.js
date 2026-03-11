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
  const startType = shape.startMarker || 'none';
  const endType   = shape.endMarker   || 'none';

  const { startId, endId } = syncMarkers(el, shape.id, { ...shape, startMarker: startType, endMarker: endType });

  let x1 = shape.x, y1 = shape.y;
  let x2 = shape.x2, y2 = shape.y2;

  const dx = x2 - x1, dy = y2 - y1;
  const len = Math.sqrt(dx * dx + dy * dy) || 1;
  const ux = dx / len, uy = dy / len;

  if (endId) {
    const off = Math.min(12, len * 0.1);
    x2 = shape.x2 - ux * off; y2 = shape.y2 - uy * off;
  }
  if (startId) {
    const off = Math.min(12, len * 0.1);
    x1 = shape.x + ux * off; y1 = shape.y + uy * off;
  }

  const line = el.querySelector('line');
  line.setAttribute('x1', x1); line.setAttribute('y1', y1);
  line.setAttribute('x2', x2); line.setAttribute('y2', y2);
  line.setAttribute('stroke', shape.stroke);
  line.setAttribute('stroke-width', shape.strokeWidth);
  line.setAttribute('fill', 'none');

  if (startId) line.setAttribute('marker-start', `url(#${startId})`);
  else line.removeAttribute('marker-start');
  if (endId) line.setAttribute('marker-end', `url(#${endId})`);
  else line.removeAttribute('marker-end');
}

export function bounds(shape) {
  const x = Math.min(shape.x, shape.x2);
  const y = Math.min(shape.y, shape.y2);
  const w = Math.abs(shape.x2 - shape.x);
  const h = Math.abs(shape.y2 - shape.y);
  return { x, y, width: Math.max(w, 1), height: Math.max(h, 1) };
}
