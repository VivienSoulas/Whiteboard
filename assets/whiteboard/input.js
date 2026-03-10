import * as state from './state.js';

let activeTool = null;
let svgEl = null;

export function init(svg, getToolFn) {
  svgEl = svg;

  svg.addEventListener('mousedown', e => {
    const tool = getToolFn();
    if (!tool) return;
    activeTool = tool;
    const pt = svgPoint(e);
    tool.onDown && tool.onDown(pt, e);
  });

  svg.addEventListener('mousemove', e => {
    const tool = getToolFn();
    if (!tool) return;
    const pt = svgPoint(e);
    tool.onMove && tool.onMove(pt, e);
  });

  window.addEventListener('mouseup', e => {
    if (!activeTool) return;
    const pt = svgPoint(e);
    activeTool.onUp && activeTool.onUp(pt, e);
    activeTool = null;
  });

  svg.addEventListener('dblclick', e => {
    const tool = getToolFn();
    if (!tool) return;
    const pt = svgPoint(e);
    tool.onDblClick && tool.onDblClick(pt, e);
  });

  window.addEventListener('keydown', e => {
    if (e.target && e.target.isContentEditable) return;
    const tool = getToolFn();
    if (tool && tool.onKey) {
      tool.onKey(e);
    }
  });
}

function svgPoint(e) {
  const rect = svgEl.getBoundingClientRect();
  return {
    x: e.clientX - rect.left,
    y: e.clientY - rect.top,
  };
}
