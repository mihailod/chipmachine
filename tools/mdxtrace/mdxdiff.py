#!/usr/bin/env python3
"""
mdxdiff -- compare two MDX driver traces produced by mdxtrace.

The point of a register-level trace is that a correct reimplementation should
be *identical*, not merely similar. So this reports the first divergence with
enough context to debug it, rather than a similarity score.

    mdxdiff.py ref.trace new.trace
    mdxdiff.py --opm-only ref.trace new.trace     ignore PCM8 events
    mdxdiff.py --batch refdir newdir              compare whole directories

Exit codes: 0 identical, 1 differs, 2 usage/IO error.
"""

import argparse
import os
import sys
from collections import Counter


"""PCM8 state-setting events whose repetition with an unchanged value has no
audible effect. mdxmini calls pcm8_set_master_volume every single frame, so a
raw trace is ~25% redundant 'P mvol' lines; a reimplementation that only writes
on change would diverge everywhere despite behaving identically."""
REDUNDANT = {
    "mvol": lambda p: ("mvol",),           # P mvol <val>
    "pan":  lambda p: ("pan",),            # P pan <val>
    "vol":  lambda p: ("vol", p[2]),       # P vol <ch> <val>
    "freq": lambda p: ("freq", p[2]),      # P freq <ch> <hz>
}


def load(path, opm_only=False, drop_redundant=False):
    """Read a trace into a list of (frame, lineno, text) events."""
    events = []
    frame = -1
    last = {}
    with open(path, "r", errors="replace") as fh:
        for lineno, raw in enumerate(fh, 1):
            line = raw.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            if line.startswith("F "):
                parts = line.split()
                if len(parts) >= 2:
                    try:
                        frame = int(parts[1])
                    except ValueError:
                        pass
            elif opm_only and not line.startswith("O "):
                continue
            elif drop_redundant and line.startswith("P "):
                parts = line.split()
                key_fn = REDUNDANT.get(parts[1]) if len(parts) > 1 else None
                if key_fn:
                    key = key_fn(parts)
                    if last.get(key) == line:
                        continue
                    last[key] = line
            events.append((frame, lineno, line))
    return events


def kind(text):
    if text.startswith("O "):
        return "OPM"
    if text.startswith("F "):
        return "frame"
    if text.startswith("P "):
        return "PCM:" + text.split()[1]
    return "other"


def summarize(events):
    return Counter(kind(t) for _, _, t in events)


def compare(ref, new, context, label_a, label_b):
    """Return (identical, first_divergence_index or None)."""
    n = min(len(ref), len(new))
    for i in range(n):
        if ref[i][2] != new[i][2]:
            return False, i
    if len(ref) != len(new):
        return False, n
    return True, None


def report(ref, new, idx, context, label_a, label_b):
    print(f"DIFFER at event {idx}")
    if idx < len(ref):
        print(f"  frame {ref[idx][0]}  ({label_a} line {ref[idx][1]})")
    elif idx < len(new):
        print(f"  frame {new[idx][0]}  ({label_b} line {new[idx][1]})")

    lo = max(0, idx - context)
    hi = idx + context + 1
    print()
    print(f"  {'':>7}  {label_a:<28}  {label_b}")
    for i in range(lo, hi):
        a = ref[i][2] if i < len(ref) else "<end of trace>"
        b = new[i][2] if i < len(new) else "<end of trace>"
        mark = ">>" if i == idx else "  "
        print(f"  {mark}{i:>5}  {a:<28}  {b}")
    print()

    ca, cb = summarize(ref), summarize(new)
    print(f"  events: {len(ref)} vs {len(new)}")
    for k in sorted(set(ca) | set(cb)):
        flag = "" if ca.get(k, 0) == cb.get(k, 0) else "   <-- differs"
        print(f"    {k:<12} {ca.get(k,0):>8} {cb.get(k,0):>8}{flag}")


def run_pair(path_a, path_b, args):
    try:
        ref = load(path_a, args.opm_only, args.ignore_redundant)
        new = load(path_b, args.opm_only, args.ignore_redundant)
    except OSError as exc:
        print(f"mdxdiff: {exc}", file=sys.stderr)
        return 2

    same, idx = compare(ref, new, args.context, path_a, path_b)
    if same:
        if not args.quiet:
            counts = summarize(ref)
            detail = ", ".join(f"{k}={counts[k]}" for k in sorted(counts))
            print(f"IDENTICAL  {len(ref)} events  ({detail})")
        return 0

    if args.quiet:
        print(f"DIFFER at event {idx}")
    else:
        report(ref, new, idx, args.context,
               os.path.basename(path_a), os.path.basename(path_b))
    return 1


def run_batch(dir_a, dir_b, args):
    names = sorted(f for f in os.listdir(dir_a) if f.endswith(".trace"))
    if not names:
        print(f"mdxdiff: no .trace files in {dir_a}", file=sys.stderr)
        return 2

    identical, differ, missing = [], [], []
    for name in names:
        pa, pb = os.path.join(dir_a, name), os.path.join(dir_b, name)
        if not os.path.exists(pb):
            missing.append(name)
            continue
        ref = load(pa, args.opm_only, args.ignore_redundant)
        new = load(pb, args.opm_only, args.ignore_redundant)
        same, idx = compare(ref, new, args.context, pa, pb)
        if same:
            identical.append(name)
        else:
            differ.append((name, idx, ref[idx][0] if idx < len(ref) else -1))

    total = len(identical) + len(differ)
    print(f"identical : {len(identical)}/{total}")
    print(f"differ    : {len(differ)}/{total}")
    if missing:
        print(f"missing   : {len(missing)} (no counterpart in {dir_b})")

    if differ:
        print()
        print("first divergences:")
        for name, idx, frame in differ[: args.max_report]:
            print(f"  {name:<40} event {idx:>8}  frame {frame}")
        if len(differ) > args.max_report:
            print(f"  ... and {len(differ) - args.max_report} more")

    return 1 if differ or missing else 0


def main():
    ap = argparse.ArgumentParser(description="compare two mdxtrace traces")
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--batch", action="store_true",
                    help="treat a and b as directories of .trace files")
    ap.add_argument("--opm-only", action="store_true",
                    help="compare YM2151 writes only, ignoring PCM8 events")
    ap.add_argument("--ignore-redundant", action="store_true",
                    help="collapse repeated PCM8 state writes that do not "
                         "change the value (mdxmini re-sets master volume "
                         "every frame)")
    ap.add_argument("--context", type=int, default=6,
                    help="events of context around a divergence (default 6)")
    ap.add_argument("--max-report", type=int, default=20,
                    help="max divergent files listed in batch mode")
    ap.add_argument("-q", "--quiet", action="store_true")
    args = ap.parse_args()

    if args.batch:
        return run_batch(args.a, args.b, args)
    return run_pair(args.a, args.b, args)


if __name__ == "__main__":
    sys.exit(main())
