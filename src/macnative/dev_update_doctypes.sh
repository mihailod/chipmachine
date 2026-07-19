#!/bin/bash
# dev_update_doctypes.sh -- fast, NO-RECOMPILE test loop for the macOS file
# associations (UTIs, "Open With", Finder document icon, default-handler
# behaviour). See the help text below (or run with --help) for full details.
set -eu

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
CHIPMACHINE_DIR=$(cd "${SCRIPT_DIR}/../.." && pwd)
WORKSPACE_ROOT=$(cd "${CHIPMACHINE_DIR}/.." && pwd)

LSREGISTER="/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister"

print_help() {
    cat <<EOF
dev_update_doctypes.sh -- fast, no-recompile test loop for the macOS file
associations that ChipMachineAS advertises.

USAGE
    dev_update_doctypes.sh [--with-binary] [path/to/ChipMachineAS.app]
    dev_update_doctypes.sh --help | -h | -? | --? | /?

    Default app path: ${WORKSPACE_ROOT}/ChipMachineAS.app

WHAT IT DOES
    Everything about the associations lives in Info.plist + DocIcon.icns +
    LaunchServices -- none of it needs the binary rebuilt. This script, against
    an existing .app bundle:
      1. regenerates Contents/Info.plist with the same generator package_app.sh
         uses (gen_info_plist.sh -> the deny / system-type / umbrella buckets),
      2. ensures DocIcon.icns exists (copies AppIcon.icns if missing),
      3. ad-hoc re-signs the bundle (editing a signed bundle breaks its seal),
      4. re-registers it with LaunchServices (lsregister -f).
    Turnaround is a couple of seconds. This is the fast alternative to a full
    package_app.sh run while iterating on the association lists.

    It exercises the ADVERTISING half only: right-click "Open With", the Finder
    document icon, "Get Info", and default-handler behaviour. It does NOT rebuild
    or replace the binary, so the actual double-click PLAYBACK path (the compiled
    Apple Event handler in FileOpenHandler.mm) is only present if the bundle
    already contains a binary built from current source -- see --with-binary.

OPTIONS
    --with-binary   Copy build/chipmachine into the bundle's Contents/MacOS/
                    before re-signing, so BOTH halves (advertising AND
                    double-click playback) are testable end-to-end without a
                    full package_app.sh. Works because the existing bundle
                    already carries the dylibs/resources from a prior package;
                    this just swaps the executable. Requires
                    ${WORKSPACE_ROOT}/build/chipmachine to exist.
    --help, -h, -?, --?, /?
                    Show this help and exit.

TYPICAL ITERATION
    1. edit MacOSHandlerDenyList.txt / MacOSSystemTypeExtensions.txt / the
       generator (or rebuild the binary if testing playback), then
    2. ./dev_update_doctypes.sh [--with-binary] [path/to/ChipMachineAS.app]
    3. in Finder: right-click a .sid / .mod / .mp3 -> "Open With", check the
       document icon and "Get Info".

VERIFY FROM THE SHELL
    umbrella UTI registered:
      "$LSREGISTER" -dump | grep -i chipmachineas.chiptune
    a file's resolved type:
      mdls -name kMDItemContentType SOMEFILE.sid
    playback path (needs a current binary -- use --with-binary):
      open -a ChipMachineAS.app SOMEFILE.sid

NOTES
    * A one-time "Open With" never changes a file type's default handler; only
      Get Info -> "Change All" (or the Option-key "Always Open With") does.
    * If associations look wrong after installing multiple copies of the app,
      reset the LaunchServices database:
        "$LSREGISTER" -kill -r -domain local -domain user
EOF
}

WITH_BINARY=false
APP=""
while [ $# -gt 0 ]; do
    case "$1" in
        --with-binary) WITH_BINARY=true; shift ;;
        --help|-h|-\?|--\?|/\?) print_help; exit 0 ;;
        -*) echo "error: unknown option '$1'" >&2; echo "run with --help for usage." >&2; exit 2 ;;
        *) APP="$1"; shift ;;
    esac
done
APP="${APP:-${WORKSPACE_ROOT}/ChipMachineAS.app}"
if [ ! -d "$APP" ]; then
    echo "error: app bundle not found: $APP" >&2
    echo "run with --help for usage." >&2
    exit 1
fi

# Version string, parsed the same way package_app.sh does.
VERSION_H="${CHIPMACHINE_DIR}/src/version.h"
VERSION_STR=$(sed -n 's/#define VERSION_STR "\(.*\)"/\1/p' "$VERSION_H" | tr -d '[:space:]')
[ -n "$VERSION_STR" ] || VERSION_STR="0.0.0-dev"

RES_DIR="${APP}/Contents/Resources"
PLIST="${APP}/Contents/Info.plist"

echo "-> Regenerating Info.plist for ${APP##*/} (v${VERSION_STR})..."
"${SCRIPT_DIR}/gen_info_plist.sh" --version "${VERSION_STR}" > "${PLIST}.tmp"
if ! plutil -lint "${PLIST}.tmp" >/dev/null; then
    echo "error: generated plist failed plutil -lint" >&2
    rm -f "${PLIST}.tmp"
    exit 1
fi
mv "${PLIST}.tmp" "${PLIST}"

# Make sure the document icon exists so Finder has something to show.
if [ ! -f "${RES_DIR}/DocIcon.icns" ] && [ -f "${RES_DIR}/AppIcon.icns" ]; then
    echo "-> Installing DocIcon.icns (copied from AppIcon.icns)..."
    cp "${RES_DIR}/AppIcon.icns" "${RES_DIR}/DocIcon.icns"
fi

# --with-binary: swap in the freshly-built executable so the double-click
# PLAYBACK path (the compiled Apple Event handler) is testable too, not just the
# plist-driven advertising. The bundle keeps all its dylibs/resources from the
# prior package; we only replace Contents/MacOS/chipmachine. The bundle re-sign
# below re-seals the new executable.
if $WITH_BINARY; then
    FRESH_BINARY="${WORKSPACE_ROOT}/build/chipmachine"
    if [ ! -f "$FRESH_BINARY" ]; then
        echo "error: --with-binary given but $FRESH_BINARY not found; build first." >&2
        exit 1
    fi
    echo "-> Installing freshly-built binary (--with-binary)..."
    cp "$FRESH_BINARY" "${APP}/Contents/MacOS/chipmachine"
    chmod +x "${APP}/Contents/MacOS/chipmachine"
fi

echo "-> Ad-hoc re-signing the bundle (Info.plist edit broke the seal)..."
codesign -f -s - "${APP}" >/dev/null 2>&1 || {
    echo "   warning: codesign failed; Finder may report the app as damaged." >&2
}

if [ -x "$LSREGISTER" ]; then
    echo "-> Re-registering with LaunchServices..."
    "$LSREGISTER" -f "${APP}"
else
    echo "   warning: lsregister not found at expected path; skipping re-register." >&2
fi

echo ""
echo "Done. Quick checks:"
echo "  umbrella UTI registered:"
echo "    \"$LSREGISTER\" -dump | grep -i chipmachineas.chiptune"
echo "  a file's resolved type:"
echo "    mdls -name kMDItemContentType SOMEFILE.sid"
echo "  playback path (needs built binary):"
echo "    open -a \"${APP##*/}\" SOMEFILE.sid"
