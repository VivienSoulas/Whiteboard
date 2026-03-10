import * as state from './state.js';

export function init({ onToolChange, onDelete, onClear }) {
  // Tool buttons
  const toolBtns = document.querySelectorAll('.tool-btn');
  toolBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      toolBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      const tool = btn.dataset.tool;
      state.setTool(tool);
      onToolChange(tool);

      const canvas = document.getElementById('canvas');
      canvas.className = 'tool-' + tool;
    });
  });

  // Keyboard shortcuts
  window.addEventListener('keydown', e => {
    if (e.target && e.target.isContentEditable) return;
    const map = { s: 'select', r: 'rectangle', e: 'ellipse', l: 'line', a: 'arrow', p: 'pen', t: 'text' };
    const tool = map[e.key.toLowerCase()];
    if (tool) {
      const btn = document.querySelector(`.tool-btn[data-tool="${tool}"]`);
      if (btn) btn.click();
    }
  });

  // Stroke color
  const strokeInput = document.getElementById('stroke-color');
  strokeInput.addEventListener('input', () => {
    state.setStroke(strokeInput.value);
    _updateSelectedStyle();
  });

  // Fill color
  const fillInput = document.getElementById('fill-color');
  fillInput.addEventListener('input', () => {
    state.setFill(fillInput.value);
    if (!state.getFillNone()) _updateSelectedStyle();
  });

  // Fill none
  const fillNone = document.getElementById('fill-none');
  fillNone.addEventListener('change', () => {
    state.setFillNone(fillNone.checked);
    _updateSelectedStyle();
  });

  // Stroke width
  const widthInput = document.getElementById('stroke-width');
  const widthVal = document.getElementById('stroke-width-val');
  widthInput.addEventListener('input', () => {
    const w = parseInt(widthInput.value, 10);
    widthVal.textContent = w;
    state.setStrokeWidth(w);
    _updateSelectedStyle();
  });

  // Delete button
  document.getElementById('btn-delete').addEventListener('click', onDelete);

  // Clear button
  document.getElementById('btn-clear').addEventListener('click', onClear);
}

function _updateSelectedStyle() {
  // Will be called by app.js via re-render; just trigger a render event
  const id = state.getSelected();
  if (!id) return;
  const style = state.currentStyle();
  state.updateShape(id, style);
  // Dispatch a custom event so app.js can re-render
  document.dispatchEvent(new CustomEvent('stylechange'));
}
