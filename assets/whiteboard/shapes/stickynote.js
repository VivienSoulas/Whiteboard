const NS = 'http://www.w3.org/2000/svg';

export const CATEGORY_PRESETS = {
  yellow: { fill: '#ffffa0', stroke: '#c8a000' },
  blue:   { fill: '#a0d8ff', stroke: '#0070c0' },
  green:  { fill: '#a0f0b0', stroke: '#00a030' },
  pink:   { fill: '#ffb0d0', stroke: '#d0005a' },
  orange: { fill: '#ffd090', stroke: '#d06000' },
};

export function create(shape) {
  const g = document.createElementNS(NS, 'g');
  const rect = document.createElementNS(NS, 'rect');
  rect.setAttribute('class', 'sticky-bg');
  const fo = document.createElementNS(NS, 'foreignObject');
  fo.setAttribute('class', 'sticky-fo');
  const div = document.createElement('div');
  div.setAttribute('contenteditable', 'true');
  div.setAttribute('spellcheck', 'false');
  fo.appendChild(div);
  g.appendChild(rect);
  g.appendChild(fo);
  update(g, shape);
  return g;
}

export function update(el, shape) {
  const w = Math.max(shape.width || 150, 60);
  const h = Math.max(shape.height || 150, 60);
  const preset = CATEGORY_PRESETS[shape.category] || CATEGORY_PRESETS.yellow;
  const fill = shape.fill && shape.fill !== 'none' ? shape.fill : preset.fill;
  const stroke = shape.stroke || preset.stroke;

  const rect = el.querySelector('.sticky-bg');
  rect.setAttribute('x', shape.x);
  rect.setAttribute('y', shape.y);
  rect.setAttribute('width', w);
  rect.setAttribute('height', h);
  rect.setAttribute('rx', '6');
  rect.setAttribute('ry', '6');
  rect.setAttribute('fill', fill);
  rect.setAttribute('stroke', stroke);
  rect.setAttribute('stroke-width', shape.strokeWidth || 1.5);
  rect.setAttribute('filter', 'url(#sticky-shadow)');

  const fo = el.querySelector('.sticky-fo');
  fo.setAttribute('x', shape.x + 8);
  fo.setAttribute('y', shape.y + 8);
  fo.setAttribute('width', w - 16);
  fo.setAttribute('height', h - 16);

  const div = fo.querySelector('div');
  if (div) {
    div.style.color = '#333333';
    div.style.fontSize = (shape.strokeWidth * 4 + 11) + 'px';
    div.style.lineHeight = '1.4';
    if (div.innerText !== (shape.text || '')) {
      div.innerText = shape.text || '';
    }
  }
}

export function bounds(shape) {
  return {
    x: shape.x,
    y: shape.y,
    width:  Math.max(shape.width  || 150, 60),
    height: Math.max(shape.height || 150, 60),
  };
}
