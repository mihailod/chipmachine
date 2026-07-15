#!/usr/bin/env python3
"""
Fill screenshot gaps by propagating an existing shot to the same tune stored
under a different name.

Screenshots are keyed by the exact song path (getSongScreenshots does a strict
map lookup in data/<collection>_screenshots.txt), so when the same tune appears
twice under different names only the harvested copy gets art:

    Protracker/Buzzer/sunstone.mod   -> media.demozoo.org/...   (harvested)
    AHX/Buzzer/sunstone.ahx          -> nothing                 (same tune)

This is a post-pass over already-built screenshot files -- it invents no new
art, it only copies a shot onto songs that are the same tune. It is idempotent:
a propagated row is indistinguishable from a harvested one, so re-running after
a re-harvest simply re-derives whatever is still missing.

Two rules, both deliberately strict (a wrong shot is worse than none):

  modland (intra-collection, +527)
      Same composer directory + identical normalized title, different format
      directory. Modland keeps one tune in several format dirs and the Demozoo
      augment only matched one of them -- e.g. it harvested
      "HVSC/MUSICIANS/V/V-12/Love_Is_an_Ocean.sid" but left the same tune's
      "RealSID/V-12/love is an ocean.rsid" blank. The composer is the PARENT dir,
      not path component 1 -- Ad Lib and a few others nest a sub-format dir
      ("Ad Lib/HSC AdLib Composer/<composer>/x.hsc"), and keying on component 1
      cross-matches unrelated composers.

  amp <- modland (cross-collection, +612)
      AMP has 58k songs but only ~5.4k shots, and shares 5411 composers with
      modland. AMP titles are noisy ("highway to hell.dd94", "May I own You_.v1.1")
      so exact matching yields literally zero; this matches on token containment
      instead, requiring >= MIN_TOKENS tokens and >= MIN_OVERLAP similarity to
      keep coincidental one-word hits ("intro", "chip") out.

Both rules key on the composer, so both depend on the composer being a real
identity (see ANON_COMPOSERS) and on the title being read correctly out of the
filename (see MIN_EXT_COMPOSERS) -- get either wrong and unrelated tunes share
one screenshot.

Deliberately NOT done: stripping trailing part/version markers so "song 2"
inherits "song"'s shot. It gains ~3.2k more on modland but attaches the wrong
production's art often enough to matter, and on HVSC it would hand Commando_2
the shot for Commando -- a different game.

Groups whose existing shots disagree are resolved by majority vote; a genuine
tie is skipped and logged to the .review sidecar rather than guessed at.

Usage:
    python3 propagate_screenshots.py [--dry-run]
"""

import argparse
import collections
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
DATA = HERE.parent / "data"

MODLAND_SHOTS = DATA / "modland_screenshots.txt"
MODLAND_SONGS = DATA / "allmods.txt"
AMP_SHOTS = DATA / "amp_screenshots.txt"
AMP_SONGS = DATA / "amp.txt"
REVIEW_FILE = HERE / "propagate_screenshots.review.txt"

# amp <- modland token match thresholds.
MIN_TOKENS = 2
MIN_OVERLAP = 0.6

# Anonymous buckets, not composers -- modland files 17494 songs under "- unknown"
# alone, so two "ninja.mod" in there are unrelated tunes by different people and
# must never share art. Matched on the NORMALIZED name, which folds "- unknown",
# "_unknown" and "UNKNOWN" together while leaving real handles ("Hyperunknown",
# "The Unknown", "Unknown World in Farland") alone.
ANON_COMPOSERS = {
    "unknown", "unknown composer", "unknown artist", "unknown demo",
    "various", "va",
}

# A dot in a modland basename does NOT reliably mean an extension follows: the
# companion formats put the tag FIRST ("TFMX/Chris Huelsbeck/mdat.cyberswats",
# "smpl.cyberswats"). Blindly taking the part before the last dot reads those as
# the title "mdat"/"smpl", collapsing a composer's entire TFMX output into one
# group that then shares a single wrong screenshot.
#
# So a tail only counts as an extension if the corpus uses it like one, on either
# of two independent signals -- each catches formats the other misses, and a title
# tail ("cyberswats") clears neither, being one file by one composer:
#   * spread across many composers: rare-but-real formats ("hvl", "dsm", "mon",
#     "mgb") are used by few people each but by many people in total.
#   * sheer frequency: single-author formats are concentrated in one composer and
#     fail the spread test -- Jochen Hippel wrote essentially every ".hip"/".soc"
#     there is -- but they are plentiful.
MIN_EXT_COMPOSERS = 5
MIN_EXT_USES = 50


def norm(text):
    """Lowercase, punctuation-to-space, collapse: 'Cosmos-Theme' -> 'cosmos theme'."""
    text = text.lower()
    text = re.sub(r"[_\-.]+", " ", text)
    text = re.sub(r"[^a-z0-9 ]", "", text)
    return re.sub(r"\s+", " ", text).strip()


def toks(text):
    return set(norm(text).split())


def real_extensions(songs):
    """The dot-tails the corpus genuinely uses as extensions.

    Numeric tails are excluded: dropping the "2" of "song.2" would fold it into
    "song", which is the loose part-suffix rule we deliberately don't do.
    """
    composers = collections.defaultdict(set)
    uses = collections.Counter()
    for composer, name in songs:
        head, dot, tail = name.rpartition(".")
        if dot and head:
            tail = tail.lower()
            composers[tail].add(composer)
            uses[tail] += 1
    return {ext for ext in uses
            if ext.isalnum() and not ext.isdigit()
            and (len(composers[ext]) >= MIN_EXT_COMPOSERS
                 or uses[ext] >= MIN_EXT_USES)}


def stem(name, exts):
    """Filename -> normalized title, dropping a trailing extension if it is one."""
    head, dot, tail = name.rpartition(".")
    if dot and head and tail.lower() in exts:
        name = head
    return norm(name)


def read_lines(path):
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


def load_shots(path):
    shots = {}
    for line in read_lines(path):
        key, tab, url = line.partition("\t")
        if tab:
            shots[key] = url
    return shots


def write_shots(path, shots, dry_run):
    """Rewrite sorted by key -- both files ship sorted, keep them that way."""
    body = "".join(f"{k}\t{shots[k]}\n" for k in sorted(shots))
    if dry_run:
        return
    path.write_text(body, encoding="utf-8")


def pick(urls, review, label):
    """Majority vote over the shots a group already has; None on a real tie."""
    counts = collections.Counter(urls)
    top = counts.most_common()
    if len(top) > 1 and top[0][1] == top[1][1]:
        review.append(f"TIE\t{label}\t" + "\t".join(sorted(set(urls))))
        return None
    return top[0][0]


def modland_songs():
    """(path, composer, title) for every modland song; composer = parent dir."""
    paths = [line.split("\t")[-1] for line in read_lines(MODLAND_SONGS)]
    paths = [p for p in paths if len(p.split("/")) >= 3]
    exts = real_extensions((p.split("/")[-2], p.split("/")[-1]) for p in paths)

    out = []
    for path in paths:
        parts = path.split("/")
        composer = norm(parts[-2])
        if composer in ANON_COMPOSERS:
            continue
        out.append((path, composer, stem(parts[-1], exts)))
    return out, exts


def propagate_modland(review):
    """Same composer + identical title across format dirs."""
    shots = load_shots(MODLAND_SHOTS)
    before = len(shots)
    songs, exts = modland_songs()

    groups = collections.defaultdict(list)
    for path, composer, title in songs:
        if title:
            groups[(composer, title)].append(path)

    for key, paths in groups.items():
        have = [p for p in paths if p in shots]
        missing = [p for p in paths if p not in shots]
        if not have or not missing:
            continue
        url = pick([shots[p] for p in have], review, "modland " + " / ".join(key))
        if url is None:
            continue
        for path in missing:
            shots[path] = url

    return shots, exts, len(shots) - before


def propagate_amp(modland_shots, exts, review):
    """AMP song <- the same composer's modland tune, matched on title tokens."""
    shots = load_shots(AMP_SHOTS)
    before = len(shots)

    # composer -> [(title tokens, url)] over modland songs that have a shot.
    by_composer = collections.defaultdict(list)
    for path, url in modland_shots.items():
        parts = path.split("/")
        if len(parts) < 3:
            continue
        composer = norm(parts[-2])
        if composer in ANON_COMPOSERS:
            continue
        tokens = set(stem(parts[-1], exts).split())
        if len(tokens) >= MIN_TOKENS:
            by_composer[composer].append((tokens, url))

    for line in read_lines(AMP_SONGS):
        cols = line.split("\t")
        if len(cols) < 5:
            continue
        title, composer, song_id = cols[0], cols[1], cols[3]
        if song_id in shots or norm(composer) in ANON_COMPOSERS:
            continue
        wanted = toks(title)
        if len(wanted) < MIN_TOKENS:
            continue

        best = []
        for tokens, url in by_composer.get(norm(composer), ()):
            if not (tokens <= wanted or wanted <= tokens):
                continue
            overlap = min(len(tokens), len(wanted)) / max(len(tokens), len(wanted))
            if overlap >= MIN_OVERLAP:
                best.append((overlap, url))
        if not best:
            continue

        top = max(o for o, _ in best)
        url = pick([u for o, u in best if o == top], review,
                   f"amp {composer} / {title}")
        if url is not None:
            shots[song_id] = url

    return shots, len(shots) - before


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dry-run", action="store_true",
                    help="report the counts, write nothing")
    args = ap.parse_args()

    review = []
    modland, exts, modland_gain = propagate_modland(review)
    amp, amp_gain = propagate_amp(modland, exts, review)

    write_shots(MODLAND_SHOTS, modland, args.dry_run)
    write_shots(AMP_SHOTS, amp, args.dry_run)
    if review and not args.dry_run:
        REVIEW_FILE.write_text("\n".join(sorted(review)) + "\n", encoding="utf-8")

    what = "Would add" if args.dry_run else "Added"
    print(f"{what} {modland_gain} modland shots -> {len(modland)} total")
    print(f"{what} {amp_gain} amp shots (from modland) -> {len(amp)} total")
    if review:
        print(f"{len(review)} ambiguous group(s) skipped -> {REVIEW_FILE.name}")


if __name__ == "__main__":
    main()
