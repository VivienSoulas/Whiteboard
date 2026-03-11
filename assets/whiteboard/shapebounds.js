// Centralized bounds and anchor utilities for all shape types.
// Does NOT import connector to avoid circular deps - connector bounds uses raw x/y/x2/y2.

import * as RectShape    from './shapes/rectangle.js';
import * as EllipseShape from './shapes/ellipse.js';
import * as LineShape    from './shapes/line.js';
import * as ArrowShape   from './shapes/arrow.js';
import * as PenShape     from './shapes/pen.js';
import * as TextShape    from './shapes/text.js';

const _map = {
  rectangle: RectShape,
  ellipse:   EllipseShape,
  line:      LineShape,
  arrow:     ArrowShape,
  pen:       PenShape,
  text:      TextShape,
};

// Register additional shape modules (called from renderer after loading new shapes)
export function register(type, mod) {
  _map[type] = mod;
}

export function getBounds(shape) {
  const mod = _map[shape.type];
  if (mod && mod.bounds) return mod.bounds(shape);
  // Fallback for connector: use resolved x/y/x2/y2
  if (shape.type === 'connector') {
    const x1 = shape.x || 0, y1 = shape.y || 0;
    const x2 = shape.x2 || 0, y2 = shape.y2 || 0;
    return {
      x: Math.min(x1, x2), y: Math.min(y1, y2),
      width: Math.max(Math.abs(x2 - x1), 1),
      height: Math.max(Math.abs(y2 - y1), 1),
    };
  }
  return { x: shape.x || 0, y: shape.y || 0, width: shape.width || 0, height: shape.height || 0 };
}

export function getAnchorPoint(shape, anchor) {
  const b = getBounds(shape);
  switch (anchor) {
    case 'top':    return { x: b.x + b.width / 2, y: b.y };
    case 'right':  return { x: b.x + b.width, y: b.y + b.height / 2 };
    case 'bottom': return { x: b.x + b.width / 2, y: b.y + b.height };
    case 'left':   return { x: b.x, y: b.y + b.height / 2 };
    default:       return { x: b.x + b.width / 2, y: b.y + b.height / 2 };
  }
}

export function nearestAnchor(shape, pt) {
  const anchors = ['top', 'right', 'bottom', 'left'];
  let best = 'center', bestDist = Infinity;
  for (const a of anchors) {
    const ap = getAnchorPoint(shape, a);
    const d = Math.hypot(ap.x - pt.x, ap.y - pt.y);
    if (d < bestDist) { bestDist = d; best = a; }
  }
  return best;
}

export function unionBounds(boundsList) {
  if (!boundsList || boundsList.length === 0) return null;
  let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
  for (const b of boundsList) {
    minX = Math.min(minX, b.x);
    minY = Math.min(minY, b.y);
    maxX = Math.max(maxX, b.x + b.width);
    maxY = Math.max(maxY, b.y + b.height);
  }
  return { x: minX, y: minY, width: maxX - minX, height: maxY - minY };
}
