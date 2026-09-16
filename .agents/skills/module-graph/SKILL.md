---
name: module-graph
description: Build an interactive HTML module/dataflow graph of a codebase — drill-in sub-graphs, hover-highlighted dependency arrows with labels, and a side panel listing the source files behind each module. Takes one argument, maxLevels, giving the deepest sub-graph level to build. Use when asked for a module map, architecture graph, dependency graph, module atlas, or "how do the modules of this project connect".
argument-hint: "maxLevels=<N>"
allowed-tools:
  - Read
  - Glob
  - Grep
  - Write
  - Edit
  - Bash(python *)
  - Bash(ls *)
  - Bash(find *)
  - Bash(git ls-files *)
  - Artifact
---

# Module Graph

Argument: `$ARGUMENTS`

Build an interactive module graph of the current repository and publish it.

## 1. Resolve `maxLevels`

Read `maxLevels` from `$ARGUMENTS` (`maxLevels=3`, or a bare `3`). Default to `3`
when absent. Clamp to `1..5`. Level 1 is the top-level graph, so `maxLevels=3`
means two drill-in steps below the top.

State the resolved value before you start, then never build a sub-graph deeper
than it — do not author data the page will refuse to open.

## 2. Survey the codebase

Do not guess the structure. Gather three things:

```bash
git ls-files | sed 's|/[^/]*$||' | sort -u
```

```bash
python ${CLAUDE_SKILL_DIR}/scripts/include_deps.py <source-roots> --repo .
```

`include_deps.py` prints `count  consumer_dir  ->  provider_dir   [top headers]`
— that is **include** direction. Graph edges run the other way (provider →
consumer), so read those arrows backwards. It ignores `*_fwd.h` forward
declaration headers, which carry no real coupling. For non-C/C++ projects, use
`Grep` over the language's import syntax instead.

Then read the headers or entry points of the largest directories to learn what
each one actually is — the main class, and what it hands to its consumers. The
descriptions and edge labels have to be true; a graph of plausible-sounding
guesses is worse than no graph.

## 3. Define the modules

A module is a **unit of responsibility**, not a directory. Merging two small
sibling directories, or splitting one large directory into several modules, is
normal and expected.

Per graph:

- **6–14 nodes.** Past that, the level should have been split.
- `name` — 2–4 words, from the module's main class (`Frame Orchestration` for
  `RenderingTasks`) or from its composite function when no single class leads.
- `sig` — the real identifier(s) a reader would grep for: `class Device`,
  `class Scene, Mesh, Material`, `generate_dll_interfaces.py`. This is the
  detail that proves the graph came from the code.
- `desc` — one sentence, ≤ 140 characters, on what the module *does*. Never
  restate the name; never describe how it is implemented.
- `files` — the source files implementing it, **repo-relative with forward
  slashes**. The page turns each one into an editor URI when clicked, so a path
  that is absolute, Windows-style or wrong opens nothing — always build with
  `--check-files`. Every **leaf** node must list files; a parent may leave
  `files` out and the page aggregates its descendants' files automatically.
- Give a node a sub-graph (a `graphs` entry keyed by that node's id) only when
  it genuinely decomposes and the level budget allows it.

Edges, as `[from, to, "label"]`:

- Direction is **provider → consumer**: `A → B` means B is downstream of A and
  consumes what A produces.
- The label names *what* flows, in ≤ 6 words: `"recorded command lists"`,
  `"decoded images & mip chains"`. Not `"depends on"`.
- Only edges inside one graph; both endpoints must be nodes of that graph.
- Keep a real back-edge when the coupling is genuinely bidirectional — the
  router draws it — but do not manufacture edges for symmetry.
- Prefer a connected level. An isolated node usually means it belongs one level
  up, or its consumer is missing.

Layout is hand-placed on a grid via `col` and `row` (floats allowed, but never
negative). Put providers left and consumers right so the flow reads
left-to-right, and stagger `row` to keep arrows off each other.

Do not agonise over the first placement. Boxes are draggable in the page, and
**Save** writes the arranged positions straight back into `col`/`row` — see
*Rearranging the graph* below. Getting the modules and edges right matters far
more than getting the grid right on the first pass.

## 4. Write the model

Write `<output-dir>/model.json`:

```json
{
  "title": "Lexgine Module Atlas",
  "brand": "Lexgine",
  "subtitle": "D3D12 renderer &middot; module atlas",
  "maxLevels": 3,
  "editor": { "name": "VS Code", "uriTemplate": "vscode://file/{root}/{path}" },
  "graphs": {
    "root": {
      "label": "lexgine",
      "kind": "Engine",
      "sig": "C++20 &middot; Direct3D 12",
      "desc": "Shown in the side panel when a module box is deselected.",
      "nodes": [
        { "id": "d3d12-device", "name": "D3D12 Device Layer",
          "sig": "class Device, DxResourceFactory", "col": 2, "row": 2.06,
          "desc": "The engine's Direct3D 12 abstraction: device, heaps, resources, descriptors." }
      ],
      "edges": [["core-foundation", "d3d12-device", "Globals, settings, descriptors"]]
    },
    "d3d12-device": {
      "nodes": [
        { "id": "dd-device", "name": "Device & Capabilities", "sig": "class Device",
          "col": 1, "row": 0.8, "desc": "ID3D12Device wrapper over every feature tier.",
          "files": ["engine/core/dx/d3d12/device.h", "engine/core/dx/d3d12/device.cpp"] }
      ],
      "edges": []
    }
  }
}
```

`editor` is optional. `{root}` is filled with the absolute path of `--repo` at
build time and `{path}` with the clicked file; the default targets VS Code.
Use `uriTemplate` (or `--editor-uri`) for another editor — for example
`cursor://file/{root}/{path}`, `vscode-insiders://file/{root}/{path}` or
`idea://open?file={root}/{path}`. `name` only labels the affordance.

Rules the builder enforces: `root` must exist; node ids are globally unique; a
sub-graph key must match a node id somewhere; every edge endpoint must be a node
of its own graph; every edge needs a label; leaves need `files`. `sig`, `desc`,
`label` and `subtitle` are inserted as HTML, so write `&` as `&amp;` there.

## 5. Build and check

```bash
python ${CLAUDE_SKILL_DIR}/scripts/build_graph.py <output-dir>/model.json \
  -o <output-dir>/index.html --repo . --check-files
```

`--check-files` fails the build on any source path that does not exist, which is
the cheapest way to catch invented file names. Fix every error; nothing is
written while one remains. Warnings about levels past `maxLevels` are only worth
acting on if you authored data you meant to be reachable.

## 6. Rearranging the graph

Boxes are dragged in the page, not edited by hand. The round trip is:

1. Drag boxes on any level. Edges re-route live, moved boxes get a corner dot,
   and the positions survive a reload via `localStorage` for that viewer.
2. **Save** re-serializes the whole model with every moved box written back into
   its `col`/`row`, downloads it under the model's own file name, and copies the
   same JSON to the clipboard.
3. Overwrite the model file with what came down, then rebuild (step 5). The
   export uses 2-space JSON and preserves key order, so the diff shows only the
   `col`/`row` lines that actually moved.

**Reset** restores the authored positions for the level on screen.

When the user asks for a rearranged graph to be regenerated, expect the saved
model in the repo already — rebuild from it rather than re-deriving the layout.
`{root}` in the editor URI is absolute, so rebuild on the machine that will open
the page.

## 7. Publish

Publish `<output-dir>/index.html` with the `Artifact` tool and give the user the
link. Keep `model.json` next to it so the graph can be regenerated and amended
without re-deriving the model.

## What the generated page does

The template in `assets/graph_template.html` already implements all of this —
supply data, do not rewrite the page:

- Module boxes with name, `sig` and description; a pill shows the sub-module
  count when a sub-graph exists.
- Dependency arrows are grey at rest, with a label beside each one. Hovering a
  box turns its adjacent arrows electric blue and outlines the modules on the
  other end; each adjacent label becomes a solid electric chip whose text
  switches to the contrasting `--electric-ink` token, so it stays readable where
  it crosses a highlighted arrow.
- Clicking a box opens the side panel with its description, its sub-modules and
  its source files, grouped by directory.
- Clicking a source file asks the local editor to open it via the configured URI
  scheme, and copies the file's absolute path as a fallback for viewers whose
  host blocks custom schemes. A published artifact runs sandboxed, so the copied
  path may be all a remote viewer gets; opening the built HTML directly in a
  browser on the machine that holds the checkout is the reliable path.
- Right-clicking a box shows an **Expand module** item — only when a sub-graph
  exists within `maxLevels`. Double-click and `→` do the same.
- Breadcrumbs, a **Back** button and `Esc` return to any ancestor level.
- Clicking empty sheet selects the module owning the current sub-graph; at the
  top level it hides the panel instead.
- **Default** (the opening view) frames the graph at full size for reading and
  expects panning; **Fit** scales the whole graph on screen. Panning or zooming
  by hand clears both, and `d` / `f` are shortcuts.
- Dragging a box moves it; dragging the sheet pans. **Save** exports the moved
  layout as a model, **Reset** restores the authored positions for the current
  level, and both are disabled until something has actually moved.
- Wheel to zoom, **Labels** to toggle labels between always-on and hover-only.
- A theme button cycling Auto / Light / Dark — Auto follows the host, an explicit
  choice overrides it and is remembered per viewer.
- A phone layout where the side panel becomes a bottom sheet.
