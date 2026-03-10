const NS = 'http://www.w3.org/2000/svg';

export function create(shape) {
  const el = document.createElementNS(NS, 'line');
  update(el, shape);
  return el;
}

export function update(el, shape) {
  el.setAttribute('x1', shape.x);
  el.setAttribute('y1', shape.y);
  el.setAttribute('x2', shape.x2);
  el.setAttribute('y2', shape.y2);
  el.setAttribute('stroke', shape.stroke);
  el.setAttribute('stroke-width', shape.strokeWidth);
  el.setAttribute('fill', 'none');
}

export function bounds(shape) {
  const x = Math.min(shape.x, shape.x2);
  const y = Math.min(shape.y, shape.y2);
  const w = Math.abs(shape.x2 - shape.x);
  const h = Math.abs(shape.y2 - shape.y);
  return { x, y, width: Math.max(w, 1), height: Math.max(h, 1) };
}
