import * as state from './state.js';
import * as renderer from './renderer.js';
import * as input from './input.js';
import * as toolbar from './toolbar.js';

import * as SelectTool    from './tools/select.js';
import * as RectTool      from './tools/rectangle.js';
import * as EllipseTool   from './tools/ellipse.js';
import * as LineTool      from './tools/line.js';
import * as ArrowTool     from './tools/arrow.js';
import * as PenTool       from './tools/pen.js';
import * as TextTool      from './tools/text.js';

const tools = {
  select:    SelectTool,
  rectangle: RectTool,
  ellipse:   EllipseTool,
  line:      LineTool,
  arrow:     ArrowTool,
  pen:       PenTool,
  text:      TextTool,
};

let currentToolName = 'select';

function getActiveTool() {
  return tools[currentToolName] || null;
}

function onToolChange(toolName) {
  currentToolName = toolName;
  // Deselect when switching away from select tool
  if (toolName !== 'select') {
    state.clearSelection();
    renderer.renderSelection();
  }
}

function onDelete() {
  const id = state.getSelected();
  if (id) {
    state.removeShape(id);
    state.clearSelection();
    renderer.render();
  }
}

function onClear() {
  if (confirm('Clear all shapes?')) {
    state.clearShapes();
    renderer.render();
  }
}

// Wire text input tracking after every render
function attachTextInputs() {
  for (const shape of state.getShapes()) {
    if (shape.type !== 'text') continue;
    const el = document.querySelector(`[data-shape-id="${shape.id}"] div`);
    if (!el || el._wired) continue;
    el._wired = true;
    el.addEventListener('input', () => {
      state.updateShape(shape.id, { text: el.innerText });
    });
    el.addEventListener('blur', () => {
      // Remove empty text shapes
      const s = state.getShape(shape.id);
      if (s && !s.text.trim()) {
        state.removeShape(shape.id);
        state.clearSelection();
        renderer.render();
      }
    });
  }
}

function init() {
  const svg = document.getElementById('canvas');
  renderer.init(svg);
  SelectTool.init(svg);
  input.init(svg, getActiveTool);

  toolbar.init({ onToolChange, onDelete, onClear });

  document.addEventListener('stylechange', () => renderer.render());

  // Initial render
  renderer.render();

  // Patch render to also wire text inputs
  const _origRender = renderer.render;
  // We already imported render; override via module-level wrapper
  // Instead, observe DOM mutations on the shapes layer to wire text inputs
  const shapesLayer = document.getElementById('shapes-layer');
  const obs = new MutationObserver(() => attachTextInputs());
  obs.observe(shapesLayer, { childList: true, subtree: true });
}

init();
