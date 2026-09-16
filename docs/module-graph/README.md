# Module graph

`lexgine.model.json` describes the engine's modules, their dataflow
dependencies and the sources behind each one. It is hand-authored from the
code; the interactive page is generated from it and is not checked in.

Regenerate:

```
python .claude/skills/module-graph/scripts/build_graph.py \
    docs/module-graph/lexgine.model.json \
    -o build/module-graph/index.html --repo . --check-files
```

`--check-files` rejects the build if any listed source path is missing, so the
model cannot drift silently away from the tree. Clicking a source in the page
opens it through the editor URI baked in at build time, which is why the output
is machine-specific and stays out of git.

To target another editor, pass `--editor-uri 'cursor://file/{root}/{path}'` or
set `editor.uriTemplate` in the model.

The `/module-graph` skill regenerates the model itself for a chosen depth:
`/module-graph maxLevels=3`.
