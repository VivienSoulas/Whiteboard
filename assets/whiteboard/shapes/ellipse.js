const NS = 'http://www.w3.org/2000/svg';

export function create(shape) {
  const el = document.createElementNS(NS, 'ellipse');
  update(el, shape);
  return el;
}

export function update(el, shape) {
  const rx = Math.abs(shape.width) / 2;
  const ry = Math.abs(shape.height) / 2;
  const cx = shape.x + shape.width / 2;
  const cy = shape.y + shape.height / 2;
  el.setAttribute('cx', cx);
  el.setAttribute('cy', cy);
  el.setAttribute('rx', rx);
  el.setAttribute('ry', ry);
  el.setAttribute('stroke', shape.stroke);
  el.setAttribute('fill', shape.fill);
  el.setAttribute('stroke-width', shape.strokeWidth);
}

export function bounds(shape) {
  return {
    x: Math.min(shape.x, shape.x + shape.width),
    y: Math.min(shape.y, shape.y + shape.height),
    width: Math.abs(shape.width),
    height: Math.abs(shape.height),
  };
}
