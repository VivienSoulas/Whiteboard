import * as state       from './state.js';
import * as renderer    from './renderer.js';
import * as input       from './input.js';
import * as toolbar     from './toolbar.js';
import * as history     from './history.js';
import * as persistence from './persistence.js';
import * as exportMod   from './export.js';
import * as align       from './align.js';
import * as viewport    from './viewport.js';
import { getBounds as _getBounds } from './shapebounds.js';

import * as SelectTool    from './tools/select.js';
import * as RectTool      from './tools/rectangle.js';
import * as EllipseTool   from './tools/ellipse.js';
import * as LineTool      from './tools/line.js';
import * as ArrowTool     from './tools/arrow.js';
import * as PenTool       from './tools/pen.js';
import * as TextTool      from './tools/text.js';
import * as StickyTool    from './tools/stickynote.js';
import * as ConnectorTool from './tools/connector.js';

const tools = {
  select:    SelectTool,
  rectangle: RectTool,
  ellipse:   EllipseTool,
  line:      LineTool,
  arrow:     ArrowTool,
  pen:       PenTool,
  text:      TextTool,
  stickynote: StickyTool,
  connector:  ConnectorTool,
};

let currentToolName = 'select';

function getActiveTool() {
  return tools[currentToolName] || null;
}

function onToolChange(toolName) {
  currentToolName = toolName;
  if (toolName !== 'select') {
    state.clearSelection();
    renderer.renderSelection();
  }
}

function onDelete() {
  const ids = [...state.getSelectedIds()];
  if (ids.length === 0) return;
  history.push(state.getShapes());
  for (const id of ids) state.removeShape(id);
  state.clearSelection();
  renderer.render();
}

function onClear() {
  if (confirm('Clear all shapes?')) {
    history.push(state.getShapes());
    state.clearShapes();
    renderer.render();
  }
}

function onUndo() { SelectTool.doUndo(); }
function onRedo() { SelectTool.doRedo(); }

function onAlign(type) {
  const shapes = state.getSelectedShapes();
  if (shapes.length < 2) return;
  history.push(state.getShapes());
  switch (type) {
    case 'left':    align.alignLeft(shapes);    break;
    case 'right':   align.alignRight(shapes);   break;
    case 'top':     align.alignTop(shapes);     break;
    case 'bottom':  align.alignBottom(shapes);  break;
    case 'hcenter': align.alignHCenter(shapes); break;
    case 'vcenter': align.alignVCenter(shapes); break;
  }
  renderer.render();
}

function onSaveLocal() {
  persistence.exportToFile();
}

function onLoadLocal() {
  persistence.importFromFile(doc => {
    const err = persistence.validateAndLoad(doc);
    if (err) { alert('Cannot load board: ' + err); return; }
    if (state.getShapes().length > 0) {
      if (!confirm('Replace the current board?')) return;
    }
    history.push(state.getShapes());
    state.importBoard(doc);
    renderer.render();
  });
}

function onExportSVG() { exportMod.exportSVG(); }
function onExportPNG() { exportMod.exportPNG(); }

function onZoomIn()    { viewport.zoomAtPoint(0.1, window.innerWidth / 2, window.innerHeight / 2); renderer.renderSelection(); }
function onZoomOut()   { viewport.zoomAtPoint(-0.1, window.innerWidth / 2, window.innerHeight / 2); renderer.renderSelection(); }
function onZoomReset() { viewport.reset(); renderer.renderSelection(); }
function _onZoomFit()  { viewport.fitContent(state.getShapes(), _getBounds); renderer.renderSelection(); }

function onServerSave(name) {
  if (!name) return;
  persistence.serverList().then(names => {
    if (names.includes(name) && !confirm(`Board "${name}" already exists. Overwrite?`)) return;
    const doc = state.exportBoard();
    persistence.serverSave(name, doc).then(() => {
      toolbar.refreshBoardList();
    }).catch(err => alert('Save failed: ' + err.message));
  }).catch(() => {
    // Can't reach server to check — save anyway
    const doc = state.exportBoard();
    persistence.serverSave(name, doc).then(() => {
      toolbar.refreshBoardList();
    }).catch(err => alert('Save failed: ' + err.message));
  });
}

function onServerLoad(name) {
  if (!name) return;
  persistence.serverLoad(name).then(doc => {
    const err = persistence.validateAndLoad(doc);
    if (err) { alert('Cannot load board: ' + err); return; }
    if (state.getShapes().length > 0) {
      if (!confirm('Replace the current board with "' + name + '"?')) return;
    }
    history.push(state.getShapes());
    state.importBoard(doc);
    renderer.render();
  }).catch(err => alert('Load failed: ' + err.message));
}

function onServerDelete(name) {
  if (!name || !confirm('Delete board "' + name + '"?')) return;
  persistence.serverDelete(name).then(() => {
    toolbar.refreshBoardList();
  }).catch(err => alert('Delete failed: ' + err.message));
}

// Wire contenteditable inputs for text and stickynote shapes
function attachTextInputs() {
  for (const shape of state.getShapes()) {
    if (shape.type !== 'text' && shape.type !== 'stickynote') continue;
    const selector = shape.type === 'text'
      ? `[data-shape-id="${shape.id}"] div`
      : `[data-shape-id="${shape.id}"] .sticky-fo div`;
    const el = document.querySelector(selector);
    if (!el || el._wired) continue;
    el._wired = true;

    el.addEventListener('focus', () => {
      history.push(state.getShapes());
    });
    el.addEventListener('input', () => {
      state.updateShape(shape.id, { text: el.innerText });
    });
    el.addEventListener('blur', () => {
      const s = state.getShape(shape.id);
      if (s && shape.type === 'text' && !s.text.trim()) {
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
  ConnectorTool.init(svg);
  input.init(svg, getActiveTool);

  toolbar.init({
    onToolChange,
    onDelete,
    onClear,
    onUndo,
    onRedo,
    onAlign,
    onSaveLocal,
    onLoadLocal,
    onExportSVG,
    onExportPNG,
    onZoomIn,
    onZoomOut,
    onZoomReset,
    onZoomFit: _onZoomFit,
    onServerSave,
    onServerLoad,
    onServerDelete,
  });

  document.addEventListener('stylechange', () => {
    // Apply style to all selected shapes
    const ids = [...state.getSelectedIds()];
    if (ids.length > 0) {
      history.push(state.getShapes());
      const style = state.currentStyle();
      for (const id of ids) state.updateShape(id, style);
    }
    renderer.render();
  });

  // Also apply marker type changes
  document.addEventListener('markerchange', e => {
    const { startMarker, endMarker } = e.detail || {};
    const ids = [...state.getSelectedIds()];
    if (ids.length === 0) return;
    history.push(state.getShapes());
    for (const id of ids) {
      const s = state.getShape(id);
      if (!s) continue;
      if (['line', 'arrow', 'connector'].includes(s.type)) {
        const updates = {};
        if (startMarker !== undefined) updates.startMarker = startMarker;
        if (endMarker !== undefined)   updates.endMarker   = endMarker;
        state.updateShape(id, updates);
      }
    }
    renderer.render();
  });

  // Undo/redo keyboard shortcuts globally (work regardless of active tool)
  window.addEventListener('keydown', e => {
    if (e.target && e.target.isContentEditable) return;
    const ctrl = e.ctrlKey || e.metaKey;
    if (ctrl && e.key === 'z' && !e.shiftKey) { e.preventDefault(); onUndo(); }
    if (ctrl && (e.key === 'y' || (e.key === 'z' && e.shiftKey))) { e.preventDefault(); onRedo(); }
  });

  renderer.render();

  // Wire text inputs after DOM changes
  const shapesLayer = document.getElementById('shapes-layer');
  const obs = new MutationObserver(() => attachTextInputs());
  obs.observe(shapesLayer, { childList: true, subtree: true });
}

init();
