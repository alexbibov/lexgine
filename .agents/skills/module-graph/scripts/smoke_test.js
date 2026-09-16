#!/usr/bin/env node
/* Boots a built module-graph page against a minimal DOM stub and drives the
 * real interaction paths. `node --check` only parses; this catches runtime
 * faults such as a shadowed binding in a pointer handler that would leave a
 * drag stuck to the cursor.
 *
 *     node smoke_test.js path/to/index.html
 */

const fs = require("fs");

const htmlPath = process.argv[2];
if (!htmlPath) {
  console.error("usage: node smoke_test.js <built index.html>");
  process.exit(2);
}
const html = fs.readFileSync(htmlPath, "utf8");
const script = /<script>([\s\S]*)<\/script>/.exec(html);
if (!script) {
  console.error("no <script> block found in " + htmlPath);
  process.exit(2);
}

/* ── DOM stub ─────────────────────────────────────────────────────────── */
const NODE_SIZE = { w: 208, h: 104 };
const LABEL_SIZE = { w: 92, h: 13 };

class Style {
  constructor() { this._ = {}; }
}
const styleProxy = () => new Proxy(new Style(), {
  get: (t, k) => (k in t ? t[k] : (t._[k] === undefined ? "" : t._[k])),
  set: (t, k, v) => { t._[k] = String(v); return true; },
});

let uid = 0;
class El {
  constructor(tag) {
    this.tag = tag;
    this.uid = ++uid;
    this.className = "";
    this.style = styleProxy();
    this.dataset = {};
    this.children = [];
    this.parent = null;
    this.attrs = {};
    this.listeners = {};
    this.textContent = "";
    this.hidden = false;
    this.disabled = false;
    this.title = "";
    this.tabIndex = -1;
    this.scrollTop = 0;
    this._html = "";
    const self = this;
    this.classList = {
      add: (...c) => c.forEach((x) => { if (!self._cls().includes(x)) self.className = (self.className + " " + x).trim(); }),
      remove: (...c) => { self.className = self._cls().filter((x) => !c.includes(x)).join(" "); },
      contains: (c) => self._cls().includes(c),
      toggle: (c, on) => { if (on === undefined) on = !self._cls().includes(c); on ? self.classList.add(c) : self.classList.remove(c); },
    };
  }
  _cls() { return this.className.split(/\s+/).filter(Boolean); }
  get innerHTML() { return this._html; }
  set innerHTML(v) { this._html = String(v); this.children = []; }
  appendChild(c) {
    if (c.parent) c.parent.children = c.parent.children.filter((x) => x !== c);
    c.parent = this;
    this.children = this.children.filter((x) => x !== c);
    this.children.push(c);
    return c;
  }
  remove() { if (this.parent) this.parent.children = this.parent.children.filter((x) => x !== this); this.parent = null; }
  setAttribute(k, v) { this.attrs[k] = String(v); if (k === "class") this.className = String(v); }
  getAttribute(k) { return k in this.attrs ? this.attrs[k] : null; }
  removeAttribute(k) { delete this.attrs[k]; }
  addEventListener(t, fn) { (this.listeners[t] = this.listeners[t] || []).push(fn); }
  removeEventListener(t, fn) { this.listeners[t] = (this.listeners[t] || []).filter((f) => f !== fn); }
  dispatchEvent() { return true; }
  click() { (this.listeners.click || []).forEach((f) => f({ target: this })); }
  scrollIntoView() {}
  getBoundingClientRect() { return { left: 0, top: 0, width: 200, height: 40, right: 200, bottom: 40 }; }
  get offsetWidth() { return this._cls().includes("node") ? NODE_SIZE.w : this._cls().includes("elabel") ? LABEL_SIZE.w : 100; }
  get offsetHeight() { return this._cls().includes("node") ? NODE_SIZE.h : this._cls().includes("elabel") ? LABEL_SIZE.h : 20; }
  get clientWidth() { return this === el.canvas ? 1400 : 300; }
  get clientHeight() { return this === el.canvas ? 800 : 600; }
  matches(sel) {
    if (sel.startsWith(".")) return this._cls().includes(sel.slice(1));
    const attr = /^\[([\w-]+)\]$/.exec(sel);
    if (attr) return this.dataset[camel(attr[1].replace(/^data-/, ""))] !== undefined;
    return false;
  }
  closest(sel) {
    let cur = this;
    while (cur) { if (cur.matches && cur.matches(sel)) return cur; cur = cur.parent; }
    return null;
  }
  querySelector(sel) {
    const m = /^\[data-id="(.+)"\]$/.exec(sel);
    const walk = (e) => {
      for (const c of e.children) {
        if (m && c.dataset.id === m[1]) return c;
        const r = walk(c); if (r) return r;
      }
      return null;
    };
    return walk(this);
  }
}
const camel = (s) => s.replace(/-([a-z])/g, (_, c) => c.toUpperCase());

const ids = ["app", "canvas", "stage", "svg", "edge-layer", "labels", "nodes",
  "crumbs", "depth", "panel", "panel-in", "t-back", "t-fit", "t-default",
  "t-labels", "t-theme", "t-save", "t-reset"];
const byId = {};
ids.forEach((i) => { const e = new El("div"); e.setAttribute("id", i); byId[i] = e; });
const el = { canvas: byId.canvas };
byId.stage.appendChild(byId["edge-layer"]);
byId.stage.appendChild(byId.labels);
byId.stage.appendChild(byId.nodes);
byId.canvas.appendChild(byId.stage);

const store = {};
const documentEl = new El("html");
const body = new El("body");

global.window = {
  addEventListener: (t, fn) => { (winL[t] = winL[t] || []).push(fn); },
  matchMedia: () => ({ matches: false }),
  innerWidth: 1400, innerHeight: 900,
  location: { href: "" },
  ResizeObserver: null,
};
const winL = {};
global.document = {
  documentElement: documentEl,
  body,
  getElementById: (i) => byId[i] || null,
  createElement: (t) => new El(t),
  createElementNS: (_ns, t) => new El(t),
  addEventListener: (t, fn) => { (winL[t] = winL[t] || []).push(fn); },
  elementFromPoint: () => hitTarget,
};
global.localStorage = {
  getItem: (k) => (k in store ? store[k] : null),
  setItem: (k, v) => { store[k] = String(v); },
  removeItem: (k) => { delete store[k]; },
};
global.navigator = {};
global.requestAnimationFrame = (fn) => { rafQueue.push(fn); return rafQueue.length; };
global.cancelAnimationFrame = () => {};
global.Blob = class { constructor(p) { this.parts = p; } };
global.URL = { createObjectURL: () => "blob:x", revokeObjectURL: () => {} };
const rafQueue = [];
let hitTarget = null;

/* ── boot ─────────────────────────────────────────────────────────────── */
const failures = [];
const check = (name, cond, extra) => {
  if (cond) console.log("  ok   " + name);
  else { console.log("  FAIL " + name + (extra ? "  (" + extra + ")" : "")); failures.push(name); }
};

let api;
try {
  api = new Function("window", "document", "localStorage", "navigator",
    "requestAnimationFrame", "cancelAnimationFrame", "Blob", "URL",
    script[1] + "\n;return {state, geom, GRAPHS, overrides, saveLayout, modelWithLayout};")(
    global.window, global.document, global.localStorage, global.navigator,
    global.requestAnimationFrame, global.cancelAnimationFrame, global.Blob, global.URL);
} catch (e) {
  console.log("  FAIL boot threw: " + e.message);
  console.log(e.stack.split("\n").slice(0, 4).join("\n"));
  process.exit(1);
}

console.log("boot");
const nodes = byId.nodes.children;
check("boxes rendered", nodes.length > 0, "count=" + nodes.length);
check("edges rendered", api.geom.edges.length > 0, "count=" + api.geom.edges.length);
check("stage sized", parseFloat(byId.stage.style.width) > 0, byId.stage.style.width);

/* ── pointer driving ──────────────────────────────────────────────────── */
const fire = (type, ev) => {
  const ls = (el.canvas.listeners[type] || []).concat(winL[type] || []);
  ls.forEach((fn) => fn(ev));
};
const pev = (x, y, target, extra) => Object.assign({
  button: 0, buttons: 1, pointerId: 1, clientX: x, clientY: y,
  target: target, preventDefault() {},
}, extra || {});
const flushRaf = () => { while (rafQueue.length) rafQueue.shift()(); };

function drag(target, from, to, opts) {
  fire("pointerdown", pev(from[0], from[1], target));
  fire("pointermove", pev(from[0] + 8, from[1], target));
  fire("pointermove", pev(to[0], to[1], target));
  flushRaf();
  hitTarget = target;
  fire("pointerup", pev(to[0], to[1], target, { buttons: 0 }));
  if (opts && opts.thenMove) fire("pointermove", pev(to[0] + 300, to[1] + 300, target));
  flushRaf();
}

console.log("\ndrag a box");
const box = nodes[0];
const box0 = [parseFloat(box.style.left), parseFloat(box.style.top)];
drag(box, [100, 100], [260, 240], { thenMove: true });
const box1 = [parseFloat(box.style.left), parseFloat(box.style.top)];
check("box moved", box1[0] !== box0[0] || box1[1] !== box0[1], box0 + " -> " + box1);
check("drag released (no drift after pointerup)",
  Math.abs(box1[0] - (box0[0] + 160)) < 2 && Math.abs(box1[1] - (box0[1] + 140)) < 2,
  "expected +160/+140, got " + (box1[0] - box0[0]) + "/" + (box1[1] - box0[1]));
check("move recorded as override", api.overrides[box.dataset.id] !== undefined);
check("box marked moved", box.classList.contains("moved"));
check("moving class cleared", !box.classList.contains("moving"));
check("Save enabled", byId["t-save"].disabled === false);

console.log("\ndrag the sheet");
hitTarget = byId.stage;
const tx0 = api.state.tx;
drag(byId.stage, [600, 400], [700, 460], { thenMove: true });
const tx1 = api.state.tx;
check("sheet panned", Math.abs(tx1 - tx0 - 100) < 2, "dx=" + (tx1 - tx0));
check("pan released (no drift after pointerup)", Math.abs(api.state.tx - tx1) < 0.001);

console.log("\ntap a box");
const box2 = nodes[1];
hitTarget = box2;
fire("pointerdown", pev(300, 300, box2));
fire("pointerup", pev(301, 300, box2, { buttons: 0 }));
check("panel opened on tap", byId.panel.hidden === false);
check("selection recorded", api.state.selected === box2.dataset.id,
  "selected=" + api.state.selected);

console.log("\ntap the sheet");
hitTarget = byId.stage;
fire("pointerdown", pev(900, 600, byId.stage));
fire("pointerup", pev(900, 600, byId.stage, { buttons: 0 }));
check("panel hidden at top level", byId.panel.hidden === true);

console.log("\nsave layout");
let saved = null;
try { global.navigator.clipboard = { writeText: (t) => { saved = t; } }; api.saveLayout(); }
catch (e) { check("saveLayout ran", false, e.message); }
if (saved !== null) {
  let parsed = null;
  try { parsed = JSON.parse(saved); } catch (e) {}
  check("export is valid JSON", parsed !== null);
  if (parsed) {
    const ov = api.overrides[box.dataset.id];
    const found = Object.values(parsed.graphs)
      .flatMap((g) => g.nodes)
      .find((nd) => nd.id === box.dataset.id);
    check("export carries the moved position",
      found && Math.abs(found.col - ov[0]) < 1e-9 && Math.abs(found.row - ov[1]) < 1e-9,
      found ? found.col + "," + found.row + " vs " + ov : "node missing");
  }
}

console.log("\n" + (failures.length ? failures.length + " FAILURE(S)" : "all checks passed"));
process.exit(failures.length ? 1 : 0);
