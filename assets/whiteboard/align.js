import * as state from './state.js';
import { getBounds } from './shapebounds.js';

export function alignLeft(shapes) {
  if (shapes.length < 2) return;
  const minX = Math.min(...shapes.map(s => getBounds(s).x));
  for (const s of shapes) {
    const b = getBounds(s);
    const dx = minX - b.x;
    if (Math.abs(dx) > 0.5) _shift(s, dx, 0);
  }
}

export function alignRight(shapes) {
  if (shapes.length < 2) return;
  const maxR = Math.max(...shapes.map(s => { const b = getBounds(s); return b.x + b.width; }));
  for (const s of shapes) {
    const b = getBounds(s);
    const dx = maxR - (b.x + b.width);
    if (Math.abs(dx) > 0.5) _shift(s, dx, 0);
  }
}

export function alignTop(shapes) {
  if (shapes.length < 2) return;
  const minY = Math.min(...shapes.map(s => getBounds(s).y));
  for (const s of shapes) {
    const b = getBounds(s);
    const dy = minY - b.y;
    if (Math.abs(dy) > 0.5) _shift(s, 0, dy);
  }
}

export function alignBottom(shapes) {
  if (shapes.length < 2) return;
  const maxB = Math.max(...shapes.map(s => { const b = getBounds(s); return b.y + b.height; }));
  for (const s of shapes) {
    const b = getBounds(s);
    const dy = maxB - (b.y + b.height);
    if (Math.abs(dy) > 0.5) _shift(s, 0, dy);
  }
}

export function alignHCenter(shapes) {
  if (shapes.length < 2) return;
  const centers = shapes.map(s => { const b = getBounds(s); return b.x + b.width / 2; });
  const avg = centers.reduce((a, b) => a + b, 0) / centers.length;
  for (const s of shapes) {
    const b = getBounds(s);
    const dx = avg - (b.x + b.width / 2);
    if (Math.abs(dx) > 0.5) _shift(s, dx, 0);
  }
}

export function alignVCenter(shapes) {
  if (shapes.length < 2) return;
  const centers = shapes.map(s => { const b = getBounds(s); return b.y + b.height / 2; });
  const avg = centers.reduce((a, b) => a + b, 0) / centers.length;
  for (const s of shapes) {
    const b = getBounds(s);
    const dy = avg - (b.y + b.height / 2);
    if (Math.abs(dy) > 0.5) _shift(s, 0, dy);
  }
}

function _shift(shape, dx, dy) {
  const updates = {};
  if (shape.x != null) updates.x = shape.x + dx;
  if (shape.y != null) updates.y = shape.y + dy;
  if (shape.x2 != null) updates.x2 = shape.x2 + dx;
  if (shape.y2 != null) updates.y2 = shape.y2 + dy;
  if (shape.points) updates.points = shape.points.map(p => ({ x: p.x + dx, y: p.y + dy }));
  state.updateShape(shape.id, updates);
}
