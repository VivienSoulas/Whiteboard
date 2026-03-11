import * as viewport from './viewport.js';
import * as renderer from './renderer.js';

let _getToolFn  = null;
let _svgEl      = null;
let _activeTool = null;

// Pan state
let _spaceDown = false;
let _panning   = false;
let _panStart  = null;

export function init(svg, getToolFn) {
  _svgEl     = svg;
  _getToolFn = getToolFn;

  svg.addEventListener('mousedown', _onMouseDown);
  svg.addEventListener('mousemove', _onMouseMove);
  window.addEventListener('mouseup', _onMouseUp);
  svg.addEventListener('dblclick', _onDblClick);
  window.addEventListener('keydown', _onKeyDown);
  window.addEventListener('keyup', _onKeyUp);
  svg.addEventListener('wheel', _onWheel, { passive: false });
}

function _onMouseDown(e) {
  // Middle mouse or Space+left → pan
  if (e.button === 1 || (e.button === 0 && _spaceDown)) {
    _panning  = true;
    _panStart = { x: e.clientX, y: e.clientY };
    e.preventDefault();
    return;
  }
  if (e.button !== 0) return;
  const tool = _getToolFn();
  if (!tool) return;
  _activeTool = tool;
  tool.onDown && tool.onDown(svgPoint(e), e);
}

function _onMouseMove(e) {
  if (_panning && _panStart) {
    viewport.pan(e.clientX - _panStart.x, e.clientY - _panStart.y);
    _panStart = { x: e.clientX, y: e.clientY };
    // Trigger selection re-render so handles stay correct
    renderer.renderSelection();
    return;
  }
  const tool = _getToolFn();
  if (!tool) return;
  tool.onMove && tool.onMove(svgPoint(e), e);
}

function _onMouseUp(e) {
  if (_panning) { _panning = false; _panStart = null; return; }
  if (!_activeTool) return;
  _activeTool.onUp && _activeTool.onUp(svgPoint(e), e);
  _activeTool = null;
}

function _onDblClick(e) {
  const tool = _getToolFn();
  if (!tool) return;
  tool.onDblClick && tool.onDblClick(svgPoint(e), e);
}

function _onKeyDown(e) {
  if (e.code === 'Space' && !e.target.isContentEditable) {
    _spaceDown = true;
    if (_svgEl) _svgEl.classList.add('panning');
    e.preventDefault();
    return;
  }
  if (e.target && e.target.isContentEditable) return;
  const tool = _getToolFn();
  if (tool && tool.onKey) tool.onKey(e);
}

function _onKeyUp(e) {
  if (e.code === 'Space') {
    _spaceDown = false;
    if (_svgEl) _svgEl.classList.remove('panning');
  }
}

function _onWheel(e) {
  e.preventDefault();
  const factor = e.deltaY > 0 ? -0.08 : 0.08;
  viewport.zoomAtPoint(factor, e.clientX, e.clientY);
  renderer.renderSelection();
}

// Convert client coordinates to SVG canvas coordinates (correct under any viewBox)
export function svgPoint(e) {
  return viewport.clientToSvg(e.clientX, e.clientY);
}

export function isSpaceDown() { return _spaceDown; }
