#!/bin/zsh
set -e

# Rebuild bin/ytdlp -- the yt-dlp helper the plus build ships and the dev build
# runs. package_app.sh copies this tree verbatim into
# Contents/Resources/bin/ytdlp (see its "yt-dlp: PyInstaller *onedir* bundle"
# step); the MAS build ships no helper at all, so this script is plus-only.
#
# WHY THIS SCRIPT EXISTS
# ----------------------
# main.cpp PREPENDS the bundled tree to PATH, ahead of anything the user has
# installed, so a stale bin/ytdlp silently wins over a perfectly current
# system yt-dlp. That failure is invisible from a shell -- every hand-run of
# the same command resolves the Homebrew binary and works -- and it surfaces
# only as YouTube tracks that never play. Refreshing this tree is therefore a
# real maintenance task, not a one-off, and it needs to be one command.
#
# It has bitten once already: YouTube began requiring a GVS PO Token for the
# android_vr client, which yt-dlp >= 2026.08 handles by refusing that client
# outright. The frozen 2026.03.17 tree did not know the replacement client
# name, silently fell back to android_vr, and every track longer than ~1MB
# (~66s at itag 140) came back HTTP 403 while short clips still played.
#
# USAGE
#   ./make-ytdlp.sh                 # latest yt-dlp, with live smoke test
#   ./make-ytdlp.sh 2026.8.19       # pin an exact version
#   ./make-ytdlp.sh --no-network    # skip the live YouTube check (offline)
#
# Does NOT sign anything and does NOT run package_app.sh -- packaging and
# signing stay a separate, manual step.

SECONDS=0
SCRIPT_DIR="${0:A:h}"
YTDLP_DEST="${SCRIPT_DIR}/bin/ytdlp"

# A video deliberately LARGER than the ~1MB window an unauthorised URL is
# granted, so the smoke test exercises the exact 403 described above. A short
# clip would pass even with a broken client pin. Override if it ever 404s.
: ${SMOKE_VIDEO:=dQw4w9WgXcQ}
: ${SMOKE_MIN_CLEN:=1500000}

YTDLP_VERSION=""
RUN_SMOKE=1
for arg in "$@"; do
    case "$arg" in
        --no-network) RUN_SMOKE=0 ;;
        -h|--help)    sed -n '3,32p' "$0"; exit 0 ;;
        -*)           echo "Unknown option: $arg" >&2; exit 1 ;;
        *)            YTDLP_VERSION="$arg" ;;
    esac
done

# ---------------------------------------------------------------------------
# 1. Build interpreter. Must be arm64: PyInstaller freezes whatever CPython it
#    runs under, and an x86_64 interpreter would produce a Rosetta-only helper
#    that Apple Silicon will eventually refuse to launch.
# ---------------------------------------------------------------------------
PYTHON="${PYTHON:-/opt/homebrew/bin/python3}"
if [ ! -x "$PYTHON" ]; then
    echo "ERROR: no python3 at $PYTHON (set PYTHON=... to override)." >&2
    exit 1
fi
PY_ARCH="$("$PYTHON" -c 'import platform; print(platform.machine())')"
if [ "$PY_ARCH" != "arm64" ]; then
    echo "ERROR: $PYTHON is $PY_ARCH, need arm64." >&2
    echo "       An x86_64 interpreter yields an x86_64 helper. Use Homebrew's" >&2
    echo "       ARM python (/opt/homebrew/bin/python3)." >&2
    exit 1
fi
echo "-> Build interpreter: $PYTHON ($("$PYTHON" -c 'import platform;print(platform.python_version())'), $PY_ARCH)"

# ---------------------------------------------------------------------------
# 2. Which player client will the app actually ask for? Read it out of
#    lua/init.lua rather than hardcoding it here, so the smoke test below can
#    never drift from what on_parse_youtube() really sends. This is the whole
#    point of the check: proving the freeze understands THAT client.
# ---------------------------------------------------------------------------
INIT_LUA="${SCRIPT_DIR}/lua/init.lua"
PLAYER_CLIENT="$(sed -n 's/.*player_client=\([a-z_0-9]*\).*/\1/p' "$INIT_LUA" | head -1)"
if [ -z "$PLAYER_CLIENT" ]; then
    echo "ERROR: could not read player_client from $INIT_LUA" >&2
    exit 1
fi
echo "-> Player client pinned in lua/init.lua: $PLAYER_CLIENT"

# ---------------------------------------------------------------------------
# 3. Throwaway venv. Kept out of the source tree and removed on exit (including
#    on failure) so a half-built attempt cannot be mistaken for a good one.
# ---------------------------------------------------------------------------
BUILD_ROOT="$(mktemp -d /tmp/make-ytdlp.XXXXXX)"
trap 'rm -rf "$BUILD_ROOT"' EXIT

echo "-> Creating venv in $BUILD_ROOT"
"$PYTHON" -m venv "${BUILD_ROOT}/venv"
"${BUILD_ROOT}/venv/bin/pip" install --quiet --upgrade pip

# zsh does NOT word-split unquoted parameters, so a flat "pkg1 pkg2" string
# would be passed to pip as ONE argument. Build the list as an array and
# expand it with "${arr[@]}". (See the shell notes in CLAUDE.md.)
typeset -a pip_pkgs
if [ -n "$YTDLP_VERSION" ]; then
    pip_pkgs=("yt-dlp==${YTDLP_VERSION}" pyinstaller)
else
    pip_pkgs=(yt-dlp pyinstaller)
fi
echo "-> Installing: ${pip_pkgs[*]}"
"${BUILD_ROOT}/venv/bin/pip" install --quiet "${pip_pkgs[@]}"

BUILT_VERSION="$("${BUILD_ROOT}/venv/bin/yt-dlp" --version | head -1)"
echo "-> yt-dlp $BUILT_VERSION / pyinstaller $("${BUILD_ROOT}/venv/bin/pyinstaller" --version)"

# ---------------------------------------------------------------------------
# 4. Freeze. ONEDIR, never onefile: a onefile build re-extracts its entire
#    runtime to a temp dir on every invocation (~8s), which turned each YouTube
#    resolve into a ~10s stall. main.cpp carries the same warning.
#
#    yt_dlp ships PyInstaller hooks via its "pyinstaller40" entry point, which
#    PyInstaller discovers automatically and uses to collect the ~1800
#    lazily-imported extractor modules. No --collect-all needed.
# ---------------------------------------------------------------------------
cat > "${BUILD_ROOT}/ytdlp_entry.py" <<'EOF'
import yt_dlp
if __name__ == '__main__':
    yt_dlp.main()
EOF

typeset -a pyi_args
pyi_args=(
    --onedir --name yt-dlp --console --noconfirm
    --distpath "${BUILD_ROOT}/dist"
    --workpath "${BUILD_ROOT}/work"
    --specpath "${BUILD_ROOT}/spec"
    --target-arch arm64
    "${BUILD_ROOT}/ytdlp_entry.py"
)
echo "-> Freezing (onedir, arm64)..."
"${BUILD_ROOT}/venv/bin/pyinstaller" "${pyi_args[@]}" > "${BUILD_ROOT}/pyinstaller.log" 2>&1 || {
    echo "ERROR: PyInstaller failed. Tail of log:" >&2
    tail -25 "${BUILD_ROOT}/pyinstaller.log" >&2
    exit 1
}

NEW_TREE="${BUILD_ROOT}/dist/yt-dlp"
[ -x "${NEW_TREE}/yt-dlp" ] || { echo "ERROR: no yt-dlp executable produced." >&2; exit 1; }
[ -d "${NEW_TREE}/_internal" ] || { echo "ERROR: no _internal/ produced (onefile by mistake?)." >&2; exit 1; }

# ---------------------------------------------------------------------------
# 5. Verify the freeze BEFORE it replaces the working tree.
# ---------------------------------------------------------------------------
echo "-> Verifying..."

FROZEN_ARCH="$(lipo -archs "${NEW_TREE}/yt-dlp")"
[ "$FROZEN_ARCH" = "arm64" ] || { echo "ERROR: frozen helper is '$FROZEN_ARCH', expected arm64." >&2; exit 1; }

FROZEN_VERSION="$("${NEW_TREE}/yt-dlp" --version 2>&1 | head -1)"
[ "$FROZEN_VERSION" = "$BUILT_VERSION" ] || {
    echo "ERROR: frozen reports '$FROZEN_VERSION', venv had '$BUILT_VERSION'." >&2; exit 1; }

# Cold start must stay well under a second; a onefile regression shows up here
# as ~8s. Measured on the frozen tree, not the venv entry point.
COLD="$("$PYTHON" - "$NEW_TREE" <<'EOF'
import subprocess, sys, time
exe = sys.argv[1] + "/yt-dlp"
t = time.time()
subprocess.run([exe, "--version"], capture_output=True)
print("%.2f" % (time.time() - t))
EOF
)"
echo "   version   : $FROZEN_VERSION"
echo "   arch      : $FROZEN_ARCH"
echo "   size      : $(du -sh "$NEW_TREE" | cut -f1)"
echo "   cold start: ${COLD}s"
"$PYTHON" -c "import sys; sys.exit(0 if float('$COLD') < 1.0 else 1)" || {
    echo "ERROR: cold start ${COLD}s is too slow -- did this build as onefile?" >&2; exit 1; }

# The check that actually matters: does this freeze understand the player
# client lua/init.lua pins, and does the URL it hands back cover the WHOLE
# file? A build that has silently fallen back to another client still returns
# a URL here -- it just 403s past the first ~1MB -- so assert on the c=
# parameter and on a clen large enough to be outside the free window.
if [ "$RUN_SMOKE" = "1" ]; then
    echo "-> Smoke test: resolving $SMOKE_VIDEO as '$PLAYER_CLIENT'..."
    SMOKE_URL="$("${NEW_TREE}/yt-dlp" \
        --extractor-args "youtube:player_client=${PLAYER_CLIENT};skip=hls,dash,translated_subs" \
        --no-playlist -f '140/bestaudio' \
        --get-url "https://www.youtube.com/watch?v=${SMOKE_VIDEO}" 2>/dev/null | head -1)"

    if [ -z "$SMOKE_URL" ]; then
        echo "ERROR: '$PLAYER_CLIENT' returned no URL." >&2
        echo "       This client is probably no longer usable (PO Token?)." >&2
        echo "       Re-run yt-dlp by hand and look for a 'requires a GVS PO Token'" >&2
        echo "       warning, then pick a token-free client and update BOTH" >&2
        echo "       lua/init.lua and the User-Agent in FFMPEGPlugin.cpp." >&2
        exit 1
    fi

    eval "$("$PYTHON" - "$SMOKE_URL" <<'EOF'
import sys, urllib.parse as p
q = p.parse_qs(p.urlparse(sys.argv[1]).query)
print("SMOKE_C=%s"    % q.get("c",    ["?"])[0])
print("SMOKE_CLEN=%s" % q.get("clen", ["0"])[0])
EOF
)"
    echo "   client returned: c=$SMOKE_C  clen=$SMOKE_CLEN"

    EXPECT_C="$(echo "$PLAYER_CLIENT" | tr '[:lower:]' '[:upper:]')"
    if [ "$SMOKE_C" != "$EXPECT_C" ]; then
        echo "ERROR: asked for '$PLAYER_CLIENT' but got a c=$SMOKE_C URL." >&2
        echo "       yt-dlp silently fell back -- this build does not know that" >&2
        echo "       client name. Exactly the bug this script exists to catch." >&2
        exit 1
    fi
    if [ "$SMOKE_CLEN" -lt "$SMOKE_MIN_CLEN" ]; then
        echo "WARNING: smoke video is only $SMOKE_CLEN bytes, below the ~1MB" >&2
        echo "         window -- it cannot prove long tracks work. Set" >&2
        echo "         SMOKE_VIDEO to something longer." >&2
    else
        # Fetch past the free window. This is the byte range that 403s when the
        # client is wrong, so it is the definitive proof.
        PROBE_START=$(( SMOKE_MIN_CLEN - 1000 ))
        HTTP="$(curl -s -o /dev/null -w '%{http_code}' --max-time 30 \
                -H "Range: bytes=${PROBE_START}-${SMOKE_MIN_CLEN}" "$SMOKE_URL")"
        if [ "$HTTP" != "206" ] && [ "$HTTP" != "200" ]; then
            echo "ERROR: byte range past the free window returned HTTP $HTTP." >&2
            echo "       Long tracks would fail to play with this build." >&2
            exit 1
        fi
        echo "   deep-range fetch: HTTP $HTTP (full file reachable)"
    fi
fi

# ---------------------------------------------------------------------------
# 6. Install, keeping the previous tree until the new one is in place.
# ---------------------------------------------------------------------------
if [ -d "$YTDLP_DEST" ]; then
    OLD_VERSION="$("${YTDLP_DEST}/yt-dlp" --version 2>/dev/null | head -1 || echo unknown)"
    BACKUP="${YTDLP_DEST}.old-${OLD_VERSION}"
    rm -rf "$BACKUP"
    mv "$YTDLP_DEST" "$BACKUP"
    echo "-> Previous tree ($OLD_VERSION) kept at $(basename "$BACKUP")"
fi
mkdir -p "${SCRIPT_DIR}/bin"
cp -R "$NEW_TREE" "$YTDLP_DEST"
chmod +x "${YTDLP_DEST}/yt-dlp"

echo
echo "OK: bin/ytdlp is now yt-dlp $FROZEN_VERSION ($FROZEN_ARCH) in ${SECONDS}s"
echo "    Delete bin/ytdlp.old-* once you are happy."
echo "    Packaging and signing remain a separate manual step (package_app.sh)."
