import * as state from './state.js';
import * as RectShape from './shapes/rectangle.js';
import * as EllipseShape from './shapes/ellipse.js';
import * as LineShape from './shapes/line.js';
import * as ArrowShape from './shapes/arrow.js';
import * as PenShape from './shapes/pen.js';
import * as TextShape from './shapes/text.js';

const HANDLE_SIZE = 8;
const HANDLE_HALF = HANDLE_SIZE / 2;

const creators = {
  rectangle: RectShape,
  ellipse:   EllipseShape,
  line:      LineShape,
  arrow:     ArrowShape,
  pen:       PenShape,
  text:      TextShape,
};

let shapesLayer = null;
let selectionLayer = null;

export function init(svgEl) {
  shapesLayer   = svgEl.querySelector('#shapes-layer');
  selectionLayer = svgEl.querySelector('#selection-layer');
}

// Full re-render of shapes layer and selection
export function render() {
  const shapes = state.getShapes();
  const existing = new Map();
  for (const el of shapesLayer.children) {
    existing.set(el.dataset.shapeId, el);
  }

  // Remove deleted
  for (const [id, el] of existing) {
    if (!shapes.find(s => s.id === id)) {
      el.remove();
    }
  }

  // Add / update
  for (const shape of shapes) {
    const mod = creators[shape.type];
    if (!mod) continue;
    let el = existing.get(shape.id);
    if (!el) {
      el = mod.create(shape);
      el.dataset.shapeId = shape.id;
      el.style.cursor = 'move';
      shapesLayer.appendChild(el);
    } else {
      mod.update(el, shape);
    }
  }

  renderSelection();
}

export function renderSelection() {
  // Clear old handles
  while (selectionLayer.firstChild) selectionLayer.firstChild.remove();

  const id = state.getSelected();
  if (!id) return;

  const shape = state.getShape(id);
  if (!shape) return;

  const mod = creators[shape.type];
  if (!mod || !mod.bounds) return;

  const b = mod.bounds(shape);
  if (b.width === 0 && b.height === 0) return;

  const NS = 'http://www.w3.org/2000/svg';

  // Dashed bounding box
  const box = document.createElementNS(NS, 'rect');
  box.setAttribute('class', 'selection-box');
  box.setAttribute('x', b.x - 1);
  box.setAttribute('y', b.y - 1);
  box.setAttribute('width', b.width + 2);
  box.setAttribute('height', b.height + 2);
  selectionLayer.appendChild(box);

  // 8 resize handles
  const positions = [
    { key: 'nw', fx: 0,   fy: 0   },
    { key: 'n',  fx: 0.5, fy: 0   },
    { key: 'ne', fx: 1,   fy: 0   },
    { key: 'e',  fx: 1,   fy: 0.5 },
    { key: 'se', fx: 1,   fy: 1   },
    { key: 's',  fx: 0.5, fy: 1   },
    { key: 'sw', fx: 0,   fy: 1   },
    { key: 'w',  fx: 0,   fy: 0.5 },
  ];

  for (const pos of positions) {
    const hx = b.x + pos.fx * b.width - HANDLE_HALF;
    const hy = b.y + pos.fy * b.height - HANDLE_HALF;
    const h = document.createElementNS(NS, 'rect');
    h.setAttribute('class', 'resize-handle');
    h.setAttribute('data-handle', pos.key);
    h.setAttribute('x', hx);
    h.setAttribute('y', hy);
    h.setAttribute('width', HANDLE_SIZE);
    h.setAttribute('height', HANDLE_SIZE);
    selectionLayer.appendChild(h);
  }
}

// Remove preview ghost
export function clearPreview() {
  const p = selectionLayer.querySelector('.drawing-preview');
  if (p) p.remove();
}

export function setPreview(el) {
  clearPreview();
  if (el) {
    el.classList.add('drawing-preview');
    selectionLayer.appendChild(el);
  }
}
