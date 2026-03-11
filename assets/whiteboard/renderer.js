import * as state from './state.js';
import * as RectShape      from './shapes/rectangle.js';
import * as EllipseShape   from './shapes/ellipse.js';
import * as LineShape      from './shapes/line.js';
import * as ArrowShape     from './shapes/arrow.js';
import * as PenShape       from './shapes/pen.js';
import * as TextShape      from './shapes/text.js';
import * as StickyShape    from './shapes/stickynote.js';
import * as ConnectorShape from './shapes/connector.js';
import * as viewport       from './viewport.js';
import { getBounds, unionBounds, register as registerBounds } from './shapebounds.js';

const NS = 'http://www.w3.org/2000/svg';
const HANDLE_SIZE = 8;

const creators = {
  rectangle:  RectShape,
  ellipse:    EllipseShape,
  line:       LineShape,
  arrow:      ArrowShape,
  pen:        PenShape,
  text:       TextShape,
  stickynote: StickyShape,
  connector:  ConnectorShape,
};

// Register new shapes in shapebounds for align / bounds queries
registerBounds('stickynote', StickyShape);
registerBounds('connector',  ConnectorShape);

let shapesLayer    = null;
let selectionLayer = null;

export function init(svgEl) {
  shapesLayer    = svgEl.querySelector('#shapes-layer');
  selectionLayer = svgEl.querySelector('#selection-layer');
  viewport.init(svgEl);
  _ensureShadowFilter(svgEl);
}

// Full re-render
export function render() {
  const shapes = state.getShapes();
  const existing = new Map();
  for (const el of shapesLayer.children) {
    existing.set(el.dataset.shapeId, el);
  }

  for (const [id, el] of existing) {
    if (!shapes.find(s => s.id === id)) el.remove();
  }

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

  viewport.apply();
  renderSelection();
}

export function renderSelection() {
  while (selectionLayer.firstChild) selectionLayer.firstChild.remove();
  document.dispatchEvent(new CustomEvent('selectionchange'));

  const ids = state.getSelectedIds();
  if (ids.size === 0) return;

  const zoom = viewport.getZoom();
  const handleSize = HANDLE_SIZE / zoom;
  const handleHalf = handleSize / 2;
  const strokeW    = 1 / zoom;

  if (ids.size === 1) {
    const id = ids.values().next().value;
    const shape = state.getShape(id);
    if (!shape) return;
    const b = getBounds(shape);
    if (b.width === 0 && b.height === 0) return;

    _drawBox(b, strokeW, zoom);

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
      const hx = b.x + pos.fx * b.width  - handleHalf;
      const hy = b.y + pos.fy * b.height - handleHalf;
      const h = document.createElementNS(NS, 'rect');
      h.setAttribute('class', 'resize-handle');
      h.setAttribute('data-handle', pos.key);
      h.setAttribute('x', hx);
      h.setAttribute('y', hy);
      h.setAttribute('width',  handleSize);
      h.setAttribute('height', handleSize);
      h.setAttribute('stroke-width', strokeW);
      selectionLayer.appendChild(h);
    }
  } else {
    // Multi: individual boxes, plus union box
    const allBounds = [];
    for (const id of ids) {
      const shape = state.getShape(id);
      if (!shape) continue;
      const b = getBounds(shape);
      _drawBox(b, strokeW, zoom);
      allBounds.push(b);
    }
    const ub = unionBounds(allBounds);
    if (ub) {
      const box = document.createElementNS(NS, 'rect');
      box.setAttribute('class', 'selection-box union-box');
      box.setAttribute('x', ub.x - 4 / zoom);
      box.setAttribute('y', ub.y - 4 / zoom);
      box.setAttribute('width',  ub.width  + 8 / zoom);
      box.setAttribute('height', ub.height + 8 / zoom);
      box.setAttribute('stroke-width', strokeW);
      box.setAttribute('stroke-dasharray', `${6 / zoom} ${4 / zoom}`);
      selectionLayer.appendChild(box);
    }
  }
}

function _drawBox(b, strokeW, zoom) {
  const box = document.createElementNS(NS, 'rect');
  box.setAttribute('class', 'selection-box');
  box.setAttribute('x', b.x - 1 / zoom);
  box.setAttribute('y', b.y - 1 / zoom);
  box.setAttribute('width',  b.width  + 2 / zoom);
  box.setAttribute('height', b.height + 2 / zoom);
  box.setAttribute('stroke-width', strokeW);
  box.setAttribute('stroke-dasharray', `${4 / zoom} ${3 / zoom}`);
  selectionLayer.appendChild(box);
}

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

function _ensureShadowFilter(svgEl) {
  let defs = svgEl.querySelector('defs');
  if (!defs) {
    defs = document.createElementNS(NS, 'defs');
    svgEl.insertBefore(defs, svgEl.firstChild);
  }
  if (defs.querySelector('#sticky-shadow')) return;
  const filter = document.createElementNS(NS, 'filter');
  filter.setAttribute('id', 'sticky-shadow');
  filter.setAttribute('x', '-10%'); filter.setAttribute('y', '-10%');
  filter.setAttribute('width', '120%'); filter.setAttribute('height', '120%');
  const shadow = document.createElementNS(NS, 'feDropShadow');
  shadow.setAttribute('dx', '2'); shadow.setAttribute('dy', '2');
  shadow.setAttribute('stdDeviation', '3');
  shadow.setAttribute('flood-opacity', '0.18');
  filter.appendChild(shadow);
  defs.appendChild(filter);
}
