const NS = 'http://www.w3.org/2000/svg';

export function create(shape) {
  const el = document.createElementNS(NS, 'rect');
  update(el, shape);
  return el;
}

export function update(el, shape) {
  const x = Math.min(shape.x, shape.x + shape.width);
  const y = Math.min(shape.y, shape.y + shape.height);
  const w = Math.abs(shape.width);
  const h = Math.abs(shape.height);
  el.setAttribute('x', x);
  el.setAttribute('y', y);
  el.setAttribute('width', w);
  el.setAttribute('height', h);
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
