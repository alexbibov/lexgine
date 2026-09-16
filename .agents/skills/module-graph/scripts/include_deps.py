#!/usr/bin/env python3
"""Aggregate #include edges into a directory-level dependency matrix.

    python include_deps.py engine api common [--repo .] [--files]

Output lines read `count  consumer_dir  ->  provider_dir   [top headers]`, i.e.
the include direction. Module graph edges run the other way round (provider ->
consumer), so read the arrows backwards when drafting the model.
Forward-declaration headers (`*_fwd.h`) are ignored: they carry no real coupling.
"""

import argparse
import collections
import os
import re
import sys

INCLUDE = re.compile(r'#include\s+[<"]([^>"]+)[>"]')
SOURCE_EXT = (".h", ".hpp", ".hh", ".inl", ".c", ".cc", ".cpp", ".cxx")
SKIP_DIRS = {".git", ".venv", "__pycache__", ".idea", "node_modules", "build", "cache", "3rd_party"}


def norm(p):
    return p.replace(os.sep, "/")


def collect(repo, roots):
    files = []
    for root in roots:
        for dirpath, dirnames, filenames in os.walk(os.path.join(repo, root)):
            dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
            for name in filenames:
                if name.endswith(SOURCE_EXT):
                    files.append(norm(os.path.relpath(os.path.join(dirpath, name), repo)))
    return sorted(files)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("roots", nargs="+")
    ap.add_argument("--repo", default=".")
    ap.add_argument("--files", action="store_true", help="report file-level edges instead")
    args = ap.parse_args()

    files = collect(args.repo, args.roots)
    if not files:
        raise SystemExit("no source files found under %s" % ", ".join(args.roots))
    fileset = set(files)
    by_name = collections.defaultdict(list)
    for f in files:
        by_name[os.path.basename(f)].append(f)

    edges = collections.Counter()
    detail = collections.defaultdict(collections.Counter)

    for f in files:
        try:
            text = open(os.path.join(args.repo, f), encoding="utf-8", errors="ignore").read()
        except OSError as exc:
            print("skipped %s (%s)" % (f, exc), file=sys.stderr)
            continue
        for inc in INCLUDE.findall(text):
            inc = norm(inc)
            target = None
            relative = norm(os.path.normpath(os.path.join(os.path.dirname(f), inc)))
            if relative in fileset:
                target = relative
            elif inc in fileset:
                target = inc
            else:
                base = os.path.basename(inc)
                if len(by_name.get(base, [])) == 1:
                    target = by_name[base][0]
            if not target or "_fwd." in os.path.basename(target):
                continue
            src = f if args.files else os.path.dirname(f)
            dst = target if args.files else os.path.dirname(target)
            if src == dst:
                continue
            edges[(src, dst)] += 1
            detail[(src, dst)][os.path.basename(target)] += 1

    for (a, b), count in sorted(edges.items(), key=lambda kv: (kv[0][0], -kv[1])):
        top = ", ".join(name for name, _ in detail[(a, b)].most_common(6))
        print("%5d  %s  ->  %s   [%s]" % (count, a, b, top))


if __name__ == "__main__":
    main()
