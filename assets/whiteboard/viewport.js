let _vp = { x: 0, y: 0, zoom: 1 };
let _svg = null;

const MIN_ZOOM = 0.1;
const MAX_ZOOM = 5;

export function init(svg) {
  _svg = svg;
  apply();
}

export function get() {
  return { ..._vp };
}

export function set(vp) {
  if (vp.zoom != null) _vp.zoom = clamp(vp.zoom, MIN_ZOOM, MAX_ZOOM);
  if (vp.x != null) _vp.x = vp.x;
  if (vp.y != null) _vp.y = vp.y;
  apply();
}

export function apply() {
  if (!_svg) return;
  const w = _svg.clientWidth  / _vp.zoom;
  const h = _svg.clientHeight / _vp.zoom;
  _svg.setAttribute('viewBox', `${_vp.x} ${_vp.y} ${w} ${h}`);
  document.dispatchEvent(new CustomEvent('viewportchange'));
}

export function zoomAtPoint(delta, clientX, clientY) {
  const oldZoom = _vp.zoom;
  const newZoom = clamp(oldZoom + delta * oldZoom, MIN_ZOOM, MAX_ZOOM);
  if (newZoom === oldZoom) return;

  // Point in SVG coords before zoom change
  const svgPt = clientToSvg(clientX, clientY);

  _vp.zoom = newZoom;
  // Re-solve pan so svgPt stays under cursor
  const r = _svg.getBoundingClientRect();
  _vp.x = svgPt.x - (clientX - r.left) / newZoom;
  _vp.y = svgPt.y - (clientY - r.top)  / newZoom;
  apply();
}

export function pan(dx, dy) {
  _vp.x -= dx / _vp.zoom;
  _vp.y -= dy / _vp.zoom;
  apply();
}

export function reset() {
  _vp = { x: 0, y: 0, zoom: 1 };
  apply();
}

export function fitContent(shapes, getBoundsFn) {
  if (!_svg || !shapes || shapes.length === 0) { reset(); return; }
  let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
  for (const s of shapes) {
    const b = getBoundsFn(s);
    minX = Math.min(minX, b.x); minY = Math.min(minY, b.y);
    maxX = Math.max(maxX, b.x + b.width); maxY = Math.max(maxY, b.y + b.height);
  }
  const pad = 40;
  const cw = _svg.clientWidth, ch = _svg.clientHeight;
  const zoom = clamp(Math.min(cw / (maxX - minX + pad * 2), ch / (maxY - minY + pad * 2)), MIN_ZOOM, MAX_ZOOM);
  _vp = { zoom, x: minX - pad, y: minY - pad };
  apply();
}

export function clientToSvg(clientX, clientY) {
  if (!_svg) return { x: clientX, y: clientY };
  const pt = _svg.createSVGPoint();
  pt.x = clientX; pt.y = clientY;
  const ctm = _svg.getScreenCTM();
  if (!ctm) return { x: clientX, y: clientY };
  return pt.matrixTransform(ctm.inverse());
}

export function getZoom() { return _vp.zoom; }

function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }
