const NS = 'http://www.w3.org/2000/svg';

export function create(shape) {
  const fo = document.createElementNS(NS, 'foreignObject');
  fo.setAttribute('class', 'text-fo');
  const div = document.createElement('div');
  div.setAttribute('contenteditable', 'true');
  div.setAttribute('spellcheck', 'false');
  fo.appendChild(div);
  update(fo, shape);
  return fo;
}

export function update(el, shape) {
  el.setAttribute('x', shape.x);
  el.setAttribute('y', shape.y);
  el.setAttribute('width', Math.max(shape.width || 200, 20));
  el.setAttribute('height', Math.max(shape.height || 40, 20));
  const div = el.querySelector('div');
  if (div) {
    div.style.color = shape.stroke;
    div.style.fontSize = (shape.strokeWidth * 6 + 10) + 'px';
    // Only set text content if different (avoid disrupting cursor)
    if (div.innerText !== (shape.text || '')) {
      div.innerText = shape.text || '';
    }
  }
}

export function bounds(shape) {
  return {
    x: shape.x,
    y: shape.y,
    width: Math.max(shape.width || 200, 20),
    height: Math.max(shape.height || 40, 20),
  };
}
