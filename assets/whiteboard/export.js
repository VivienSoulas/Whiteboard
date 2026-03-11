import { getBounds, unionBounds } from './shapebounds.js';
import * as state from './state.js';

// ── SVG export ────────────────────────────────────────────────────────────────

export function exportSVG() {
  const svgEl = document.getElementById('canvas');
  const clone = svgEl.cloneNode(true);

  // Remove selection layer content
  const selLayer = clone.querySelector('#selection-layer');
  if (selLayer) { while (selLayer.firstChild) selLayer.firstChild.remove(); }

  // Compute content bounding box and set viewBox to fit
  const shapes = state.getShapes();
  if (shapes.length > 0) {
    const bounds = shapes.map(s => getBounds(s));
    const ub = unionBounds(bounds);
    if (ub) {
      const pad = 20;
      clone.setAttribute('viewBox', `${ub.x - pad} ${ub.y - pad} ${ub.width + pad * 2} ${ub.height + pad * 2}`);
      clone.setAttribute('width', ub.width + pad * 2);
      clone.setAttribute('height', ub.height + pad * 2);
    }
  }

  // Inline text-fo div styles (SVG serializer doesn't carry stylesheet)
  const textFos = clone.querySelectorAll('.text-fo div, .sticky-fo div');
  textFos.forEach(div => {
    div.style.boxSizing = 'border-box';
  });

  clone.removeAttribute('class');

  const serializer = new XMLSerializer();
  const svgStr = serializer.serializeToString(clone);
  const blob = new Blob([svgStr], { type: 'image/svg+xml' });
  _download(blob, `board-${_ts()}.svg`);
}

// ── PNG export ────────────────────────────────────────────────────────────────

export function exportPNG() {
  const svgEl = document.getElementById('canvas');
  const clone = svgEl.cloneNode(true);

  const selLayer = clone.querySelector('#selection-layer');
  if (selLayer) { while (selLayer.firstChild) selLayer.firstChild.remove(); }
  clone.removeAttribute('class');

  const shapes = state.getShapes();
  let vbX = 0, vbY = 0, vbW = svgEl.clientWidth, vbH = svgEl.clientHeight;
  if (shapes.length > 0) {
    const bounds = shapes.map(s => getBounds(s));
    const ub = unionBounds(bounds);
    if (ub) {
      const pad = 20;
      vbX = ub.x - pad; vbY = ub.y - pad;
      vbW = ub.width + pad * 2; vbH = ub.height + pad * 2;
    }
  }
  clone.setAttribute('viewBox', `${vbX} ${vbY} ${vbW} ${vbH}`);
  clone.setAttribute('width', vbW);
  clone.setAttribute('height', vbH);

  const serializer = new XMLSerializer();
  const svgStr = serializer.serializeToString(clone);
  const encoded = btoa(unescape(encodeURIComponent(svgStr)));
  const dataUrl = `data:image/svg+xml;base64,${encoded}`;

  const img = new Image();
  img.onload = () => {
    const canvas = document.createElement('canvas');
    canvas.width = vbW * 2; // @2x
    canvas.height = vbH * 2;
    const ctx = canvas.getContext('2d');
    ctx.scale(2, 2);
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, vbW, vbH);
    ctx.drawImage(img, 0, 0);
    canvas.toBlob(blob => {
      if (blob) _download(blob, `board-${_ts()}.png`);
    }, 'image/png');
  };
  img.onerror = () => alert('PNG export failed. Try SVG export instead.');
  img.src = dataUrl;
}

function _download(blob, filename) {
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url; a.download = filename;
  a.click();
  setTimeout(() => URL.revokeObjectURL(url), 5000);
}

function _ts() {
  return new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
}
