const NS = 'http://www.w3.org/2000/svg';

// Each arrow gets its own marker so color can vary
export function create(shape) {
  const g = document.createElementNS(NS, 'g');
  const marker = _makeMarker(shape);
  const line = document.createElementNS(NS, 'line');
  g.appendChild(marker);
  g.appendChild(line);
  update(g, shape);
  return g;
}

export function update(el, shape) {
  const marker = el.querySelector('marker');
  const line = el.querySelector('line');
  const markerId = 'arrow-' + shape.id;

  marker.setAttribute('id', markerId);
  marker.querySelector('polygon').setAttribute('fill', shape.stroke);

  line.setAttribute('x1', shape.x);
  line.setAttribute('y1', shape.y);
  // Shorten line slightly so arrowhead doesn't overlap
  const dx = shape.x2 - shape.x;
  const dy = shape.y2 - shape.y;
  const len = Math.sqrt(dx * dx + dy * dy) || 1;
  const offset = Math.min(12, len * 0.9);
  const ux = dx / len;
  const uy = dy / len;
  line.setAttribute('x2', shape.x2 - ux * offset);
  line.setAttribute('y2', shape.y2 - uy * offset);
  line.setAttribute('stroke', shape.stroke);
  line.setAttribute('stroke-width', shape.strokeWidth);
  line.setAttribute('fill', 'none');
  line.setAttribute('marker-end', `url(#${markerId})`);
}

export function bounds(shape) {
  const x = Math.min(shape.x, shape.x2);
  const y = Math.min(shape.y, shape.y2);
  const w = Math.abs(shape.x2 - shape.x);
  const h = Math.abs(shape.y2 - shape.y);
  return { x, y, width: Math.max(w, 1), height: Math.max(h, 1) };
}

function _makeMarker(shape) {
  const marker = document.createElementNS(NS, 'marker');
  marker.setAttribute('markerWidth', '10');
  marker.setAttribute('markerHeight', '7');
  marker.setAttribute('refX', '10');
  marker.setAttribute('refY', '3.5');
  marker.setAttribute('orient', 'auto');
  const poly = document.createElementNS(NS, 'polygon');
  poly.setAttribute('points', '0 0, 10 3.5, 0 7');
  poly.setAttribute('fill', shape.stroke);
  marker.appendChild(poly);
  return marker;
}
