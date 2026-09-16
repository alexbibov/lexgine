#!/usr/bin/env python3
"""Render a module-graph model into the interactive HTML page.

    python build_graph.py model.json -o out/index.html [--repo .] [--check-files]
                         [--editor-uri 'vscode://file/{root}/{path}']

The model is validated before rendering; every problem found is reported and a
non-zero exit code is returned so a broken graph is never published.
"""

import argparse
import json
import os
import re
import sys

TEMPLATE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        os.pardir, "assets", "graph_template.html")

NODE_KEYS = {"id", "name", "sig", "desc", "col", "row", "files"}
MODEL_KEYS = {"title", "brand", "subtitle", "maxLevels", "editor",
              "modelFileName", "graphs"}


def fail(problems, msg):
    problems.append(msg)


def validate(model, repo, check_files):
    problems = []
    warnings = []

    max_levels = model.get("maxLevels")
    if not isinstance(max_levels, int) or max_levels < 1:
        fail(problems, "maxLevels must be an integer >= 1")
    graphs = model.get("graphs")
    if not isinstance(graphs, dict) or "root" not in graphs:
        fail(problems, 'graphs must be an object containing a "root" graph')
        return problems, warnings, {}

    for key in model:
        if key not in MODEL_KEYS:
            warnings.append("model has unknown top-level field '%s'" % key)

    node_owner = {}
    for gid, g in graphs.items():
        nodes = g.get("nodes")
        if not isinstance(nodes, list) or not nodes:
            fail(problems, "graph '%s' has no nodes" % gid)
            continue
        seen = set()
        for n in nodes:
            nid = n.get("id")
            if not nid:
                fail(problems, "graph '%s' has a node without an id" % gid)
                continue
            for key in ("name", "sig", "desc"):
                if not n.get(key):
                    fail(problems, "node '%s' is missing '%s'" % (nid, key))
            for key in ("col", "row"):
                if not isinstance(n.get(key), (int, float)):
                    fail(problems, "node '%s' is missing a numeric '%s'" % (nid, key))
            for key in n:
                if key not in NODE_KEYS:
                    warnings.append("node '%s' has unknown field '%s'" % (nid, key))
            for key in ("col", "row"):
                if isinstance(n.get(key), (int, float)) and n[key] < 0:
                    fail(problems, "node '%s' has a negative '%s'" % (nid, key))
            if nid in node_owner:
                fail(problems, "node id '%s' is used in both '%s' and '%s' "
                               "(ids must be globally unique)" % (nid, node_owner[nid], gid))
            node_owner[nid] = gid
            seen.add(nid)
        for e in g.get("edges", []):
            if not isinstance(e, list) or len(e) != 3:
                fail(problems, "graph '%s' has a malformed edge %r "
                               "(expected [from, to, label])" % (gid, e))
                continue
            src, dst, label = e
            for end in (src, dst):
                if end not in seen:
                    fail(problems, "graph '%s' edge %s -> %s references '%s', "
                                   "which is not a node of that graph" % (gid, src, dst, end))
            if not label:
                fail(problems, "graph '%s' edge %s -> %s has no label" % (gid, src, dst))
            if src == dst:
                fail(problems, "graph '%s' has a self-edge on '%s'" % (gid, src))

    # sub-graph keys must name a node somewhere, and depth must stay in budget
    depth = {"root": 1}

    def walk(gid, d, trail):
        depth[gid] = d
        for n in graphs[gid]["nodes"]:
            if n["id"] in graphs:
                if n["id"] in trail:
                    fail(problems, "sub-graph cycle through '%s'" % n["id"])
                    continue
                walk(n["id"], d + 1, trail | {n["id"]})

    walk("root", 1, {"root"})

    for gid in graphs:
        if gid not in depth:
            fail(problems, "graph '%s' is unreachable from root "
                           "(no node with that id)" % gid)
        elif isinstance(max_levels, int) and depth[gid] > max_levels:
            warnings.append("graph '%s' sits at level %d and stays closed at "
                            "maxLevels=%d" % (gid, depth[gid], max_levels))

    # a leaf node carries the sources; a parent may aggregate from its children
    for gid, g in graphs.items():
        for n in g["nodes"]:
            if n["id"] not in graphs and not n.get("files"):
                fail(problems, "leaf node '%s' lists no source files" % n["id"])

    if check_files:
        for gid, g in graphs.items():
            for n in g["nodes"]:
                for f in n.get("files", []):
                    if not os.path.exists(os.path.join(repo, f)):
                        fail(problems, "node '%s' lists a missing path: %s" % (n["id"], f))

    return problems, warnings, depth


DEFAULT_EDITOR_URI = "vscode://file/{root}/{path}"


def render(model, repo, editor_uri, model_name):
    with open(TEMPLATE, encoding="utf-8") as fh:
        tpl = fh.read()

    editor = model.get("editor") or {}
    uri_template = editor_uri or editor.get("uriTemplate") or DEFAULT_EDITOR_URI
    if "{path}" not in uri_template:
        raise SystemExit("editor URI template must contain a {path} slot: %s" % uri_template)
    root = os.path.abspath(repo).replace(os.sep, "/").rstrip("/")
    uri = uri_template.replace("{root}", root)

    # the page re-serializes this to save a rearranged layout, so hand it the
    # model as authored, with the file name it should suggest on download
    page_model = dict(model)
    page_model.setdefault("modelFileName", model_name)

    out = tpl
    out = out.replace("__MODEL__", json.dumps(page_model, indent=2, ensure_ascii=False))
    out = out.replace("__EDITOR_URI__", json.dumps(uri))
    out = out.replace("__EDITOR_NAME__", json.dumps(editor.get("name", "VS Code")))
    out = out.replace("__REPO_ROOT__", json.dumps(root))
    out = out.replace("__TITLE__", model.get("title", "Module Atlas"))
    out = out.replace("__BRAND__", model.get("brand", "Modules"))
    out = out.replace("__SUBTITLE__", model.get("subtitle", "module atlas"))
    left = re.findall(r"__[A-Z_]+__", out)
    if left:
        raise SystemExit("template placeholders left unfilled: %s" % sorted(set(left)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("model")
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--repo", default=".", help="repository root the file paths are relative to")
    ap.add_argument("--check-files", action="store_true",
                    help="fail if any listed source path does not exist under --repo")
    ap.add_argument("--editor-uri", default=None,
                    help="URI template used when a source file is clicked; {root} is the "
                         "absolute repo path and {path} the repo-relative file "
                         "(default: %s)" % DEFAULT_EDITOR_URI)
    args = ap.parse_args()

    with open(args.model, encoding="utf-8") as fh:
        model = json.load(fh)

    problems, warnings, depth = validate(model, args.repo, args.check_files)
    for w in warnings:
        print("warning: %s" % w, file=sys.stderr)
    if problems:
        for p in problems:
            print("error: %s" % p, file=sys.stderr)
        raise SystemExit("%d problem(s) found; nothing written" % len(problems))

    html = render(model, args.repo, args.editor_uri,
                  os.path.basename(args.model))
    out_dir = os.path.dirname(os.path.abspath(args.out))
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    with open(args.out, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(html)

    levels = max(depth.values()) if depth else 1
    nodes = sum(len(g["nodes"]) for g in model["graphs"].values())
    edges = sum(len(g.get("edges", [])) for g in model["graphs"].values())
    print("wrote %s" % args.out)
    print("  graphs %d  nodes %d  edges %d  deepest level %d (maxLevels %d)"
          % (len(model["graphs"]), nodes, edges, levels, model["maxLevels"]))


if __name__ == "__main__":
    main()
