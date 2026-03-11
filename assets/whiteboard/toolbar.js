import * as state from './state.js';
import * as persistence from './persistence.js';
import { MARKER_TYPES } from './markers.js';
import * as viewport from './viewport.js';
import * as history from './history.js';

let _callbacks = {};

export function init(callbacks) {
  _callbacks = callbacks;

  _initToolButtons();
  _initStyleControls();
  _initActionButtons();
  _initAlignButtons();
  _initSaveLoad();
  _initExport();
  _initZoom();
  _initMarkerPickers();
  _initKeyboardShortcuts();
  _initServerUI();
  _initZOrder();

  // Update alignment button state when selection changes
  document.addEventListener('selectionchange', _updateAlignState);
  document.addEventListener('historychange', _updateAlignState);
}

export function refreshBoardList() {
  _loadBoardList();
}

// ── Tool buttons ──────────────────────────────────────────────────────────────

function _initToolButtons() {
  const toolBtns = document.querySelectorAll('.tool-btn');
  toolBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      toolBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      const tool = btn.dataset.tool;
      state.setTool(tool);
      _callbacks.onToolChange(tool);

      const canvas = document.getElementById('canvas');
      canvas.className = 'tool-' + tool;

      // Show/hide sticky category picker
      const catRow = document.getElementById('sticky-category-row');
      if (catRow) catRow.style.display = tool === 'stickynote' ? 'flex' : 'none';

      // Show/hide marker pickers
      const markerRow = document.getElementById('marker-row');
      if (markerRow) {
        const lineTypes = ['line', 'arrow', 'connector'];
        markerRow.style.display = lineTypes.includes(tool) ? 'flex' : 'none';
      }

      _updateAlignState();
    });
  });
}

// ── Style controls ────────────────────────────────────────────────────────────

function _initStyleControls() {
  const strokeInput = document.getElementById('stroke-color');
  strokeInput.addEventListener('input', () => {
    state.setStroke(strokeInput.value);
    _fireStyleChange();
  });

  const fillInput = document.getElementById('fill-color');
  fillInput.addEventListener('input', () => {
    state.setFill(fillInput.value);
    if (!state.getFillNone()) _fireStyleChange();
  });

  const fillNone = document.getElementById('fill-none');
  fillNone.addEventListener('change', () => {
    state.setFillNone(fillNone.checked);
    _fireStyleChange();
  });

  const widthInput = document.getElementById('stroke-width');
  const widthVal   = document.getElementById('stroke-width-val');
  widthInput.addEventListener('input', () => {
    const w = parseInt(widthInput.value, 10);
    widthVal.textContent = w;
    state.setStrokeWidth(w);
    _fireStyleChange();
  });

  // Sticky note category picker
  const catSelect = document.getElementById('sticky-category');
  if (catSelect) {
    catSelect.addEventListener('change', () => {
      state.setStickyCategory(catSelect.value);
      // Apply to selected sticky note immediately
      const ids = [...state.getSelectedIds()];
      for (const id of ids) {
        const s = state.getShape(id);
        if (s && s.type === 'stickynote') {
          state.updateShape(id, { category: catSelect.value });
        }
      }
      document.dispatchEvent(new CustomEvent('stylechange'));
    });
  }
}

// ── Action buttons ────────────────────────────────────────────────────────────

function _initActionButtons() {
  document.getElementById('btn-delete').addEventListener('click', _callbacks.onDelete);
  document.getElementById('btn-clear').addEventListener('click',  _callbacks.onClear);
  document.getElementById('btn-undo').addEventListener('click',   _callbacks.onUndo);
  document.getElementById('btn-redo').addEventListener('click',   _callbacks.onRedo);
  document.addEventListener('historychange', _updateHistoryState);
  _updateHistoryState();
}

function _updateHistoryState() {
  const btnUndo = document.getElementById('btn-undo');
  const btnRedo = document.getElementById('btn-redo');
  if (btnUndo) btnUndo.disabled = !history.canUndo();
  if (btnRedo) btnRedo.disabled = !history.canRedo();
}

// ── Alignment buttons ─────────────────────────────────────────────────────────

function _initAlignButtons() {
  const types = ['left', 'right', 'top', 'bottom', 'hcenter', 'vcenter'];
  for (const type of types) {
    const btn = document.getElementById('align-' + type);
    if (btn) {
      btn.addEventListener('click', () => _callbacks.onAlign(type));
    }
  }
  _updateAlignState();
}

function _updateAlignState() {
  const count = state.getSelectedIds().size;
  const types = ['left', 'right', 'top', 'bottom', 'hcenter', 'vcenter'];
  for (const type of types) {
    const btn = document.getElementById('align-' + type);
    if (btn) btn.disabled = count < 2;
  }
}

// ── Save / Load (local) ───────────────────────────────────────────────────────

function _initSaveLoad() {
  const btnSave = document.getElementById('btn-save-local');
  const btnLoad = document.getElementById('btn-load-local');
  if (btnSave) btnSave.addEventListener('click', _callbacks.onSaveLocal);
  if (btnLoad) btnLoad.addEventListener('click', _callbacks.onLoadLocal);
}

// ── Export ────────────────────────────────────────────────────────────────────

function _initExport() {
  const btnSVG = document.getElementById('btn-export-svg');
  const btnPNG = document.getElementById('btn-export-png');
  if (btnSVG) btnSVG.addEventListener('click', _callbacks.onExportSVG);
  if (btnPNG) btnPNG.addEventListener('click', _callbacks.onExportPNG);
}

// ── Zoom controls ─────────────────────────────────────────────────────────────

function _initZoom() {
  const btnIn    = document.getElementById('btn-zoom-in');
  const btnOut   = document.getElementById('btn-zoom-out');
  const btnReset = document.getElementById('btn-zoom-reset');
  const btnFit   = document.getElementById('btn-zoom-fit');
  if (btnIn)    btnIn.addEventListener('click',    _callbacks.onZoomIn);
  if (btnOut)   btnOut.addEventListener('click',   _callbacks.onZoomOut);
  if (btnReset) btnReset.addEventListener('click', _callbacks.onZoomReset);
  if (btnFit)   btnFit.addEventListener('click',   _callbacks.onZoomFit);

  // Update zoom display on wheel/pan changes
  document.addEventListener('viewportchange', _updateZoomDisplay);
}

function _updateZoomDisplay() {
  const el = document.getElementById('zoom-level');
  if (el) el.textContent = Math.round(viewport.getZoom() * 100) + '%';
}

// ── Marker pickers ────────────────────────────────────────────────────────────

function _initMarkerPickers() {
  const startSel = document.getElementById('marker-start');
  const endSel   = document.getElementById('marker-end');
  if (startSel) {
    MARKER_TYPES.forEach(t => {
      const opt = document.createElement('option');
      opt.value = t; opt.textContent = t;
      startSel.appendChild(opt);
    });
    startSel.addEventListener('change', () => {
      document.dispatchEvent(new CustomEvent('markerchange', { detail: { startMarker: startSel.value } }));
    });
  }
  if (endSel) {
    MARKER_TYPES.forEach(t => {
      const opt = document.createElement('option');
      opt.value = t; opt.textContent = t;
      endSel.appendChild(opt);
    });
    endSel.value = 'arrow'; // default
    endSel.addEventListener('change', () => {
      document.dispatchEvent(new CustomEvent('markerchange', { detail: { endMarker: endSel.value } }));
    });
  }

  // Update pickers when selection changes
  document.addEventListener('selectionchange', _updateMarkerPickers);
}

function _updateMarkerPickers(e) {
  const id = state.getSelected();
  if (!id) return;
  const s = state.getShape(id);
  if (!s || !['line', 'arrow', 'connector'].includes(s.type)) return;
  const startSel = document.getElementById('marker-start');
  const endSel   = document.getElementById('marker-end');
  if (startSel) startSel.value = s.startMarker || 'none';
  if (endSel)   endSel.value   = s.endMarker !== undefined ? s.endMarker : (s.type === 'arrow' ? 'arrow' : 'none');
}

// ── Server board UI ───────────────────────────────────────────────────────────

function _initServerUI() {
  const btnSaveServer = document.getElementById('btn-save-server');
  const btnLoadServer = document.getElementById('btn-load-server');
  const btnDelServer  = document.getElementById('btn-del-server');
  const nameInput     = document.getElementById('server-board-name');
  const listSel       = document.getElementById('server-board-list');

  if (btnSaveServer && nameInput) {
    btnSaveServer.addEventListener('click', () => {
      const name = nameInput.value.trim();
      if (!name) { alert('Enter a board name'); return; }
      _callbacks.onServerSave(name);
    });
  }

  if (btnLoadServer && listSel) {
    btnLoadServer.addEventListener('click', () => {
      const name = listSel.value;
      if (!name) { alert('Select a board to load'); return; }
      _callbacks.onServerLoad(name);
    });
  }

  if (btnDelServer && listSel) {
    btnDelServer.addEventListener('click', () => {
      const name = listSel.value;
      if (!name) return;
      _callbacks.onServerDelete(name);
    });
  }

  _loadBoardList();
}

function _loadBoardList() {
  const listSel = document.getElementById('server-board-list');
  if (!listSel) return;
  persistence.serverList().then(names => {
    listSel.innerHTML = '<option value="">-- select board --</option>';
    names.forEach(n => {
      const opt = document.createElement('option');
      opt.value = n; opt.textContent = n;
      listSel.appendChild(opt);
    });
  }).catch(() => {
    // Server persistence unavailable, silently ignore
  });
}

// ── Keyboard shortcuts ────────────────────────────────────────────────────────

function _initKeyboardShortcuts() {
  window.addEventListener('keydown', e => {
    if (e.target && e.target.isContentEditable) return;
    if (e.ctrlKey || e.metaKey) return; // handled by select tool or app.js
    const map = {
      s: 'select', r: 'rectangle', e: 'ellipse',
      l: 'line', a: 'arrow', p: 'pen', t: 'text',
      n: 'stickynote', k: 'connector',
    };
    const tool = map[e.key.toLowerCase()];
    if (tool) {
      const btn = document.querySelector(`.tool-btn[data-tool="${tool}"]`);
      if (btn) btn.click();
    }
  });
}

// ── Helpers ───────────────────────────────────────────────────────────────────
function _initZOrder() {
  const btnFront = document.getElementById('btn-bring-front');
  const btnBack  = document.getElementById('btn-send-back');
  if (btnFront) btnFront.addEventListener('click', () => _callbacks.onBringToFront && _callbacks.onBringToFront());
  if (btnBack)  btnBack.addEventListener('click',  () => _callbacks.onSendToBack  && _callbacks.onSendToBack());
  document.addEventListener('selectionchange', _updateZOrderState);
  _updateZOrderState();
}

function _updateZOrderState() {
  const count = state.getSelectedIds().size;
  const btnFront = document.getElementById('btn-bring-front');
  const btnBack  = document.getElementById('btn-send-back');
  if (btnFront) btnFront.disabled = count === 0;
  if (btnBack)  btnBack.disabled  = count === 0;
}
function _fireStyleChange() {
  document.dispatchEvent(new CustomEvent('stylechange'));
}
