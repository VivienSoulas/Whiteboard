import * as state from './state.js';
import * as viewport from './viewport.js';

const BOARD_VERSION = 1;

// ── Local JSON save / load ────────────────────────────────────────────────────

export function exportToFile() {
  const doc = {
    version: BOARD_VERSION,
    shapes: state.getShapes(),
    viewport: viewport.get(),
  };
  const blob = new Blob([JSON.stringify(doc, null, 2)], { type: 'application/json' });
  _download(blob, `board-${_timestamp()}.json`);
}

export function importFromFile(onLoad) {
  const input = document.createElement('input');
  input.type = 'file';
  input.accept = '.json,application/json';
  input.onchange = () => {
    const file = input.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = e => {
      try {
        const doc = JSON.parse(e.target.result);
        onLoad(doc);
      } catch (err) {
        alert('Failed to parse JSON file: ' + err.message);
      }
    };
    reader.readAsText(file);
  };
  input.click();
}

export function validateAndLoad(doc) {
  if (!doc || typeof doc !== 'object') return 'Invalid document';
  if (doc.version !== BOARD_VERSION) return `Unsupported version: ${doc.version}`;
  if (!Array.isArray(doc.shapes)) return 'Missing shapes array';
  for (const s of doc.shapes) {
    if (!s || typeof s !== 'object') return 'Shape is not an object';
    if (!s.id || typeof s.id !== 'string') return 'Shape missing id';
    if (!KNOWN_TYPES.has(s.type)) return `Unknown shape type: ${s.type}`;
  }
  return null; // ok
}

// ── Server-side board API ─────────────────────────────────────────────────────

const API_BASE = '/api/boards.py';

export async function serverList() {
  const res = await fetch(`${API_BASE}?action=list`);
  if (!res.ok) throw new Error(`Server error: ${res.status}`);
  return res.json();
}

export async function serverLoad(name) {
  const res = await fetch(`${API_BASE}?action=load&name=${encodeURIComponent(name)}`);
  if (!res.ok) throw new Error(`Server error: ${res.status}`);
  return res.json();
}

export async function serverSave(name, doc) {
  const res = await fetch(`${API_BASE}?action=save&name=${encodeURIComponent(name)}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(doc),
  });
  if (!res.ok) throw new Error(`Server error: ${res.status}`);
  return res.json();
}

export async function serverDelete(name) {
  const res = await fetch(`${API_BASE}?action=delete&name=${encodeURIComponent(name)}`, {
    method: 'POST',
  });
  if (!res.ok) throw new Error(`Server error: ${res.status}`);
  return res.json();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

function _download(blob, filename) {
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url; a.download = filename;
  a.click();
  setTimeout(() => URL.revokeObjectURL(url), 5000);
}

function _timestamp() {
  return new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
}
