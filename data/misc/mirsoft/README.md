# mirsoft.info — World of Game MODs

Curated archive of the **tracker modules used in games**, one `.zip` per game
(several `.mod`/`.xm`/`.it`/`.s3m`/`.med` modules + an `info.txt`). Spans many
platforms — overwhelmingly Amiga, then C64, PC (DOS/Windows), NES, SNES,
Macintosh, PlayStation, … — but by policy only mainstream tracker formats (its
FAQ: *"why exotic tracked formats are not in the archive"*). So there is **no new
format**: every module plays via OpenMPT/UADE and the ZIP-by-magic subsong
handler in `MusicPlayerList` extracts them as subsongs, like zophar/vgmrips/
smspower. Shipped as collection **`mirsoft`** (db.lua v86), 1019 net-new games.

## Runtime download

Primary = Internet Archive snapshot, fallback = live mirsoft
(registered in `MusicDatabase::generateIndex`, the `mirsoft` branch):

```
https://archive.org/download/mirsoftJuly2021snapshot/gamemods/<GAME>.zip   (primary)
http://mirsoft.info/gamemods/<GAME>.zip                                    (fallback)
```

`<GAME>` is the URL-encoded game name (the `path` column of `data/mirsoft.txt`).

## Build source (offline — no live crawl)

The whole `gamemods/` tree is mirrored on the Internet Archive as item
**`mirsoftJuly2021snapshot`** — a 982 MB `.tar.xz`. Download + extract it, then:

```
scripts/build_mirsoft.py --build --snapshot <extracted gamemods dir>
```

Every fact (platform, composer, format, track list) is read from each game's
`info.txt`. Dedup is platform-aware (Amiga names vs UnExoticA + modland "Video
Game Music", C64 vs rko, CPC vs cpcpower, consoles vs zophar/vgmrips/smspower)
plus a byte `(stem,size)` content match vs modland + amp. Classification is by
the game's `Platform` field → a platform label in the `format` column that
`MusicDatabase::initFormats` (mirsoft block) maps to a platform byte.

**Top-up:** the 2021 tarball (1601 games) is topped up with games added since,
found via the site's *Newest additions* view (`gamemods-archive.php?order=
timestamp&order_desc=1`) filtered to post-snapshot dates. Each delta zip is
fetched from `/gamemods/<Name>.zip` (the archive *displays* `Game: Subtitle` but
the zip name uses `Game - Subtitle`) and extracted into the `--snapshot` tree
before rebuilding. 1049 net-new games shipped (includes 29 post-2021 additions,
2022–2025).

## Screenshots

mirsoft hosts **no screenshots** of its own (music-only). Best-effort game shots
are matched by game name from sources we already ship offline — **gb64**
(C64, `data/Games.csv`), **Hall of Light / abime.net** (Amiga, via the unexotica
join), and the **zophar / vgmrips / smspower / cpcpower** per-game shots — with a
platform-native source preferred and a cross-title fallback (a same-named game's
shot on another platform, usually the same multiplatform release):

```
scripts/build_mirsoft.py --screenshots      # -> data/mirsoft_screenshots.txt
```

274/1049 games get a shot (mostly the C64-heavy subset via gb64). Keyed by the
`<Game>.zip` song path; consumed by the `mirsoft` branch of
`MusicDatabase::getSongScreenshots`.

## Files

* `mods.list` — the raw A→Z list of game-zip names on the site (reconnaissance).
* `data/mirsoft.txt` — the shipped index (`title  composer  platform  path  ext`).
