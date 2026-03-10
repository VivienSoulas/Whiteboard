const NS = 'http://www.w3.org/2000/svg';

export function create(shape) {
  const el = document.createElementNS(NS, 'polyline');
  update(el, shape);
  return el;
}

export function update(el, shape) {
  const pts = (shape.points || []).map(p => `${p.x},${p.y}`).join(' ');
  el.setAttribute('points', pts);
  el.setAttribute('stroke', shape.stroke);
  el.setAttribute('stroke-width', shape.strokeWidth);
  el.setAttribute('fill', 'none');
  el.setAttribute('stroke-linecap', 'round');
  el.setAttribute('stroke-linejoin', 'round');
}

export function bounds(shape) {
  const pts = shape.points || [];
  if (pts.length === 0) return { x: 0, y: 0, width: 0, height: 0 };
  const xs = pts.map(p => p.x);
  const ys = pts.map(p => p.y);
  const x = Math.min(...xs);
  const y = Math.min(...ys);
  const w = Math.max(...xs) - x;
  const h = Math.max(...ys) - y;
  return { x, y, width: Math.max(w, 1), height: Math.max(h, 1) };
}
