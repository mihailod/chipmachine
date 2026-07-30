#!/bin/zsh
set -e

# Wall-clock timer for the whole packaging run. zsh's $SECONDS auto-increments;
# resetting it here means it reads the elapsed seconds at the end (see the
# final "Total packaging time" line).
SECONDS=0

# Establish precise absolute paths independent of execution context
# ${0:A:h} is zsh-native: :A resolves to absolute path, :h strips the filename.
# BASH_SOURCE[0] is a bash-ism and is undefined (empty) in zsh — do not use it.
SCRIPT_DIR="${0:A:h}"
CHIPMACHINE_DIR="${SCRIPT_DIR}"
WORKSPACE_ROOT="$(cd "${CHIPMACHINE_DIR}/.." && pwd)"
ICON_PATH="${CHIPMACHINE_DIR}/data/misc/icon.png"
MACNATIVE_DIR="${CHIPMACHINE_DIR}/src/macnative"

# NOTE: the variant-dependent paths (APP_NAME, TARGET_DIR, MAC_OS_DIR,
# RESOURCES_DIR, YTDLP_DEST, BUILD_DIR) and identity (BUNDLE_ID, DISPLAY_NAME,
# ...) are resolved AFTER argument parsing, once the --plus/--mas variant is
# known -- see the "Resolve variant identity" block below. They are still
# defined before any build/sign step (including the --reusebuiltapp path).

# -----------------------------------------------------------------
# Parse Arguments
# -----------------------------------------------------------------
# The script does nothing without an explicit action flag: run with no args (or
# --help) and it prints usage and exits. Building and signing are separable:
#   --buildapponly            build the ad-hoc self-signed bundle and stop
#                             (this is the previous default behavior).
#   --applesign               build, THEN Developer ID sign (+ optional notarize).
#   --applesign --reusebuiltapp
#                             skip the build and (re)sign the .app already on disk
#                             -- the fast re-sign / re-notarize loop.
# Flags accept a single or double dash and are case-insensitive; value flags use
# --key=value.
print_usage() {
    cat <<'USAGE'
ChipMachine -- app bundle packaging & signing

Two product variants (default: --plus). Identity/names live in variants.conf:
  --plus   ChipMachinePlus  -- full build (incl. YouTube), Developer ID / GitHub.
                              Built from ./build (CM_VARIANT=plus).
  --mas    ChipMachine       -- Mac App Store build (no YouTube, App-Sandboxed).
                              Built from ./build-mas (must be configured with
                              -DCM_VARIANT=mas). Distributed as a signed .pkg.

Usage:
  package_app.sh [--plus] --buildapponly [--releaseit]
  package_app.sh [--plus] --applesign --signid="Developer ID Application: Name (TEAMID)" \
                 [--notaryprofile=NAME] [--reusebuiltapp] [--releaseit]
  package_app.sh --mas --buildapponly            # ad-hoc sandboxed .app, LOCAL test
  package_app.sh --mas --applesign --distribid="Apple Distribution: Name (TEAMID)" \
                 --installerid="3rd Party Mac Developer Installer: Name (TEAMID)" \
                 --provision=/path/to/ChipMachine.provisionprofile

Actions (at least one of --buildapponly / --applesign required):
  --buildapponly         Build the .app, ad-hoc self-signed, and stop. For --mas
                         this yields a sandboxed local-test app (no .pkg).
  --applesign            Build, then sign for real. --plus: Developer ID +
                         Hardened Runtime (requires --signid). --mas: Apple
                         Distribution + App Sandbox + a .pkg (requires
                         --distribid, --installerid, --provision).

Signing options -- plus / Developer ID (used with --applesign):
  --signid="..."         Developer ID signing identity, e.g.
                         "Developer ID Application: Your Name (ABCDE12345)".
                         List yours: security find-identity -v -p codesigning
  --notaryprofile=NAME   Also notarize with Apple and staple the ticket, so
                         downloaded copies open with no Gatekeeper warning.
                         NAME is a stored notarytool credential profile, created
                         once (credentials live in the keychain, not on the CLI):
                           xcrun notarytool store-credentials NAME \
                               --apple-id you@example.com --team-id TEAMID

Signing options -- mas / Mac App Store (used with --mas --applesign):
  --distribid="..."      "Apple Distribution: Name (TEAMID)" (app signature).
  --installerid="..."    "3rd Party Mac Developer Installer: Name (TEAMID)" (.pkg).
  --provision=PATH       Mac App Store provisioning profile embedded into the app.
                         (All three require the $99 Apple Developer Program.)

Other options:
  --reusebuiltapp        Do NOT rebuild -- sign the .app already on disk. Cannot
                         be combined with --buildapponly.
  --releaseit            After packaging, interactively create a GitHub release
                         (plus variant only).
  -h, --help             Show this help.

Examples:
  package_app.sh --buildapponly                               # plus, ad-hoc
  package_app.sh --mas --buildapponly                         # mas, local test
  package_app.sh --applesign --signid="Developer ID Application: Jane (ABCDE12345)" \
                 --notaryprofile=chipmachine-notary
USAGE
}

DO_BUILD=false          # run the build/assemble steps (0-6)?
APPLE_SIGN=false        # real (non-ad-hoc) sign at the end?
REUSE_BUILT=false       # skip the build and sign the existing .app?
RELEASE_IT=false        # create a GitHub release afterwards? (modifier)
ACTION=false            # was any actionable flag (build/sign) supplied?
VARIANT="plus"          # product variant: plus (full/GitHub) | mas (App Store)
SIGN_ID=""              # plus: Developer ID Application identity
NOTARY_PROFILE=""       # plus: notarytool keychain profile

# --- Mac App Store signing inputs (PLACEHOLDERS) ------------------------------
# Fill these in once enrolled in the Apple Developer Program ($99/yr) and the
# App Store Connect record + certs + provisioning profile exist. Until then,
# `--mas --buildapponly` produces an ad-hoc-signed, sandboxed ChipMachine.app for
# LOCAL testing, and `--mas --applesign` errors out asking for these.
DISTRIB_ID=""           # "Apple Distribution: Name (TEAMID)"  (app signature)
INSTALLER_ID=""         # "3rd Party Mac Developer Installer: Name (TEAMID)" (.pkg)
PROVISION=""            # path to the Mac App Store .provisionprofile to embed

if [ $# -eq 0 ]; then
    print_usage
    exit 1
fi

for arg in "$@"; do
    key="${arg%%=*}"                 # portion before '=' (the flag name)
    val="${arg#*=}"                  # portion after  '=' (the value, if any)
    [ "$val" = "$arg" ] && val=""    # no '=' present -> no inline value
    case "${key:l}" in               # ${key:l} = zsh lowercase
        -h|--h|-help|--help)                print_usage; exit 0 ;;
        -plus|--plus)                       VARIANT="plus" ;;
        -mas|--mas)                         VARIANT="mas" ;;
        -buildapponly|--buildapponly)       DO_BUILD=true; ACTION=true ;;
        -applesign|--applesign)             APPLE_SIGN=true; ACTION=true ;;
        -reusebuiltapp|--reusebuiltapp)     REUSE_BUILT=true ;;
        -signid|--signid)                   SIGN_ID="$val" ;;
        -notaryprofile|--notaryprofile)     NOTARY_PROFILE="$val" ;;
        -distribid|--distribid)             DISTRIB_ID="$val" ;;
        -installerid|--installerid)         INSTALLER_ID="$val" ;;
        -provision|--provision)             PROVISION="$val" ;;
        -releaseit|--releaseit)             RELEASE_IT=true ;;
        *) echo "ERROR: unknown argument '$arg'"; echo; print_usage; exit 1 ;;
    esac
done

# Supplying any signing parameter (either variant's) implies the signing action.
if [ -n "$SIGN_ID" ] || [ -n "$NOTARY_PROFILE" ] || \
   [ -n "$DISTRIB_ID" ] || [ -n "$INSTALLER_ID" ] || [ -n "$PROVISION" ]; then
    APPLE_SIGN=true
    ACTION=true
fi

if $APPLE_SIGN; then
    if [ "$VARIANT" = "mas" ]; then
        # Mac App Store distribution signing needs all three inputs (placeholders).
        if [ -z "$DISTRIB_ID" ] || [ -z "$INSTALLER_ID" ] || [ -z "$PROVISION" ]; then
            echo "ERROR: --mas --applesign requires Apple App Store signing inputs:"
            echo "         --distribid=\"Apple Distribution: Name (TEAMID)\""
            echo "         --installerid=\"3rd Party Mac Developer Installer: Name (TEAMID)\""
            echo "         --provision=/path/to/ChipMachine.provisionprofile"
            echo "       These need the \$99 Apple Developer Program + App Store Connect setup."
            echo "       For a local sandboxed test build, use:  package_app.sh --mas --buildapponly"
            echo
            print_usage
            exit 1
        fi
    elif [ -z "$SIGN_ID" ]; then
        echo "ERROR: --applesign / --notaryprofile require --signid=\"Developer ID Application: ... (TEAMID)\""
        echo
        print_usage
        exit 1
    fi
fi

if $REUSE_BUILT && ! $APPLE_SIGN; then
    echo "ERROR: --reusebuiltapp only applies to the signing path; add --applesign --signid=\"...\"."
    exit 1
fi

if $REUSE_BUILT && $DO_BUILD; then
    echo "ERROR: --reusebuiltapp cannot be combined with --buildapponly (one reuses, the other rebuilds)."
    exit 1
fi

if ! $ACTION; then
    print_usage
    exit 1
fi

# Decide whether to run the build/assemble steps:
#   --buildapponly              -> build (DO_BUILD already true)
#   --applesign (no reuse)      -> build, then sign
#   --applesign --reusebuiltapp -> skip build, sign the existing bundle
if $APPLE_SIGN && ! $REUSE_BUILT; then
    DO_BUILD=true
fi
if $REUSE_BUILT; then
    DO_BUILD=false
fi

# Ad-hoc identity when not doing a real Developer ID sign (step 7 reads SIGN_ID).
$APPLE_SIGN || SIGN_ID="-"

# -----------------------------------------------------------------
# Resolve variant identity (single source of truth: variants.conf)
# -----------------------------------------------------------------
# variants.conf defines <PLUS|MAS>_{PROGRAM_NAME,DISPLAY_NAME,BUNDLE_ID,ARTIFACT}
# as plain shell KEY="VALUE". Source it and select the block for $VARIANT. This
# is what drives the .app name, Info.plist identity, build dir, entitlements, and
# (for mas) the App Store signing/packaging path further down.
VARIANTS_CONF="${CHIPMACHINE_DIR}/variants.conf"
if [ ! -f "$VARIANTS_CONF" ]; then
    echo "CRITICAL ERROR: variants.conf not found at $VARIANTS_CONF"
    exit 1
fi
source "$VARIANTS_CONF"

case "$VARIANT" in
    plus)
        ARTIFACT="$PLUS_ARTIFACT"; BUNDLE_ID="$PLUS_BUNDLE_ID"; DISPLAY_NAME="$PLUS_DISPLAY_NAME"
        BUILD_DIR="${WORKSPACE_ROOT}/build"
        APP_CATEGORY=""                                   # no App Store category
        ENT_APP="${MACNATIVE_DIR}/entitlements-app.plist" # Developer ID entitlements
        ;;
    mas)
        ARTIFACT="$MAS_ARTIFACT"; BUNDLE_ID="$MAS_BUNDLE_ID"; DISPLAY_NAME="$MAS_DISPLAY_NAME"
        BUILD_DIR="${WORKSPACE_ROOT}/build-mas"           # CM_VARIANT=mas build dir
        APP_CATEGORY="public.app-category.music"
        ENT_APP="${MACNATIVE_DIR}/entitlements-app-mas.plist"  # App Sandbox
        # Real App Store signing uses the Apple Distribution identity; ad-hoc test
        # builds keep the "-" set above.
        $APPLE_SIGN && SIGN_ID="$DISTRIB_ID"
        ;;
    *)
        echo "CRITICAL ERROR: unknown VARIANT '$VARIANT' (expected plus|mas)"; exit 1 ;;
esac

APP_NAME="${ARTIFACT}.app"
TARGET_DIR="${WORKSPACE_ROOT}/${APP_NAME}"
MAC_OS_DIR="${TARGET_DIR}/Contents/MacOS"
RESOURCES_DIR="${TARGET_DIR}/Contents/Resources"
YTDLP_DEST="${RESOURCES_DIR}/bin/ytdlp"

# -----------------------------------------------------------------
# Dynamically parse the version string from src/version.h
# -----------------------------------------------------------------
VERSION_H_PATH="${CHIPMACHINE_DIR}/src/version.h"
if [ ! -f "$VERSION_H_PATH" ]; then
    echo "CRITICAL ERROR: Version header not found at $VERSION_H_PATH!"
    exit 1
fi

VERSION_STR=$(sed -n 's/#define VERSION_STR "\(.*\)"/\1/p' "$VERSION_H_PATH" | tr -d '[:space:]')

if [ -z "$VERSION_STR" ]; then
    echo "CRITICAL ERROR: Failed to extract VERSION_STR from $VERSION_H_PATH!"
    exit 1
fi

echo "=== Starting Apple Silicon App Bundle Packaging ==="
echo "Workspace Root: ${WORKSPACE_ROOT}"
echo "Variant:        ${VARIANT}  (${DISPLAY_NAME}, ${BUNDLE_ID})"
echo "Build Dir:      ${BUILD_DIR}"
echo "Target App Bundle: ${TARGET_DIR}"
echo "Detected Version: ${VERSION_STR}"
if $RELEASE_IT; then
    echo "Release Mode: Enabled (--releaseit Flag Detected)"
else
    echo "Release Mode: Disabled (Dry Run/Local Build Only)"
fi
if $APPLE_SIGN; then
    if [ "$VARIANT" = "mas" ]; then
        echo "Signing Mode: Mac App Store (${SIGN_ID}); installer ${INSTALLER_ID}"
        echo "Provisioning: ${PROVISION}"
    else
        echo "Signing Mode: Developer ID (${SIGN_ID})"
        if [ -n "$NOTARY_PROFILE" ]; then
            echo "Notarization: Enabled (profile: ${NOTARY_PROFILE})"
        else
            echo "Notarization: Disabled (add --notaryprofile=NAME to notarize + staple)"
        fi
    fi
else
    echo "Signing Mode: Ad-hoc self-signed (--buildapponly)"
fi
if $REUSE_BUILT; then
    echo "Build Mode:   Skipped (--reusebuiltapp; re-signing the existing bundle)"
fi

# =================================================================
# BUILD + ASSEMBLE (steps 0-6). Skipped entirely under --reusebuiltapp,
# which jumps straight to signing (step 7) using the .app already on disk.
# =================================================================
if $DO_BUILD; then

# 0. Build the binary first (incremental).
#
# Packaging is a release-time action, so we always (re)build the chipmachine
# target before packaging to guarantee the bundled binary matches the current
# source -- no more "forgot to rebuild" stale-binary releases. This is an
# INCREMENTAL ninja build: if nothing changed it is ~1s ("no work to do"); only
# a first-ever/post-clean build is slow, which is unavoidable regardless.
#
# Done BEFORE step 1 wipes the previous .app, so a compile failure aborts (via
# `set -e`) while the last good bundle is still intact. We only build when the
# build dir is already CMake-configured -- we deliberately do NOT run `cmake`
# configure here, because that chooses the build type (Release vs Debug) and the
# generator, which is the developer's decision, not the packager's.
if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    echo "-> Building chipmachine target (incremental)..."
    cmake --build "${BUILD_DIR}" --target chipmachine
else
    echo "WARNING: ${BUILD_DIR} is not CMake-configured; skipping build step."
    echo "         Packaging whatever binary already exists. Configure the build"
    echo "         dir (e.g. cmake -S chipmachine -B build -DCMAKE_BUILD_TYPE=Release)"
    echo "         to have package_app.sh rebuild automatically."
fi

# 1. Clean previous packaging attempts and set up pristine directories
rm -rf "${TARGET_DIR}"
mkdir -p "${MAC_OS_DIR}"
mkdir -p "${RESOURCES_DIR}"

# 2. Copy compiled binary as the primary bundle entry point
if [ ! -f "${BUILD_DIR}/chipmachine" ]; then
    echo "CRITICAL ERROR: Compiled binary not found at ${BUILD_DIR}/chipmachine!"
    exit 1
fi
echo "-> Copying executable binary..."
cp "${BUILD_DIR}/chipmachine" "${MAC_OS_DIR}/chipmachine"
chmod +x "${MAC_OS_DIR}/chipmachine"

# 3. Create Info.plist (incl. macOS file associations)
#
# The whole plist -- base keys AND the file-association document types -- is
# emitted by src/macnative/gen_info_plist.sh, the single source shared with the
# fast no-recompile test loop (dev_update_doctypes.sh). It reads the playable
# extension list (extensions.txt) and the two hand-editable list files
# (MacOSHandlerDenyList.txt, MacOSSystemTypeExtensions.txt) next to it.
#
# extensions.txt is the union of every extension the plugins advertise; it is
# the single source of truth, so we refresh it here from the freshly-built
# binary (`--dump-extensions`) before generating the plist. If that fails for
# any reason we fall back to the checked-in copy rather than shipping an empty
# association list.
# MACNATIVE_DIR is defined near the top (needed by the signing step too).
GEN_PLIST="${MACNATIVE_DIR}/gen_info_plist.sh"
EXTS_FILE="${MACNATIVE_DIR}/extensions.txt"

echo "-> Refreshing playable-extension list from built binary..."
if "${BUILD_DIR}/chipmachine" --dump-extensions > "${EXTS_FILE}.tmp" 2>/dev/null \
        && [ -s "${EXTS_FILE}.tmp" ]; then
    mv "${EXTS_FILE}.tmp" "${EXTS_FILE}"
    echo "   extensions.txt: $(wc -l < "${EXTS_FILE}" | tr -d '[:space:]') extensions"
else
    rm -f "${EXTS_FILE}.tmp"
    echo "   WARNING: --dump-extensions failed; using checked-in extensions.txt"
fi

echo "-> Creating Info.plist (with macOS file associations)..."
# Build args as an array (zsh does not word-split unquoted ${:+...}); append the
# App Store category only for the mas variant (APP_CATEGORY empty for plus).
GEN_ARGS=(--version "${VERSION_STR}" --bundle-id "${BUNDLE_ID}" --display-name "${DISPLAY_NAME}")
[ -n "${APP_CATEGORY}" ] && GEN_ARGS+=(--app-category "${APP_CATEGORY}")
"${GEN_PLIST}" "${GEN_ARGS[@]}" > "${TARGET_DIR}/Contents/Info.plist"
if ! plutil -lint "${TARGET_DIR}/Contents/Info.plist" >/dev/null; then
    echo "CRITICAL ERROR: generated Info.plist failed plutil -lint. Aborting."
    exit 1
fi

# 4. Copy the asset and Lua payloads from the chipmachine source tree
#
# data/ is the SHIPPING asset tree only -- everything under it is copied
# verbatim into Contents/Resources and sealed into the code signature. Anything
# that is merely build-time input (raw scrapes feeding scripts/build_*.py, side
# tables, project docs, marketing screenshots) belongs in the sibling
# chipmachine/data-notbundled/, which mirrors data/'s layout and is deliberately
# NOT referenced here. See the header of scripts/DB_UPDATE_PROCESS.txt.
echo "-> Packaging runtime assets into bundle..."
if [ -d "${CHIPMACHINE_DIR}/data" ]; then
    cp -R "${CHIPMACHINE_DIR}/data" "${RESOURCES_DIR}/"
else
    echo "ERROR: Data folder not found at ${CHIPMACHINE_DIR}/data"
    exit 1
fi

if [ -f "${CHIPMACHINE_DIR}/data/misc/Credits.rtf" ]; then
    echo "-> Packaging Credits into bundle..."
    cp "${CHIPMACHINE_DIR}/data/misc/Credits.rtf" "${RESOURCES_DIR}/"
fi

if [ -d "${CHIPMACHINE_DIR}/lua" ]; then
    echo "-> Packaging Lua subsystem files into bundle..."
    cp -R "${CHIPMACHINE_DIR}/lua" "${RESOURCES_DIR}/"
else
    echo "WARNING: Lua folder not found at ${CHIPMACHINE_DIR}/lua. Scripting features may fail."
fi

# 4a. Prune non-shippable cruft from the COPIED asset trees (never the source).
#
# `cp -R data`/`cp -R lua` copy everything, so a stray build/triage cache left
# under data/ (e.g. a *_cache dir of thousands of JSON files) would silently
# balloon the bundle by tens of MB and get sealed into the code signature. Strip
# any *cache* / __pycache__ directory plus editor cruft. Scoped to data/ and
# lua/ only — the yt-dlp helper payload is signed code and is left untouched.
for PRUNE_ROOT in "${RESOURCES_DIR}/data" "${RESOURCES_DIR}/lua"; do
    [ -d "$PRUNE_ROOT" ] || continue
    CRUFT=$(find "$PRUNE_ROOT" -type d \( -iname '*cache*' -o -name '__pycache__' \) -prune 2>/dev/null)
    if [ -n "$CRUFT" ]; then
        echo "-> Pruning non-shippable caches from bundled assets:"
        echo "$CRUFT" | while read -r d; do
            echo "     $(du -sh "$d" 2>/dev/null | awk '{print $1}')  ${d#${RESOURCES_DIR}/}"
        done
        find "$PRUNE_ROOT" -type d \( -iname '*cache*' -o -name '__pycache__' \) -prune -exec rm -rf {} + 2>/dev/null
    fi
    find "$PRUNE_ROOT" \( -name '.DS_Store' -o -name '*.pyc' \) -delete 2>/dev/null || true
done

if [ -f "/opt/homebrew/etc/openssl@3/cert.pem" ]; then
    echo "-> Packaging OpenSSL certificates for standalone HTTPS..."
    cp -L "/opt/homebrew/etc/openssl@3/cert.pem" "${RESOURCES_DIR}/"
fi

# Destination for the yt-dlp PyInstaller onedir tree.
#
# CRITICAL: it must live in Contents/Resources/, NOT Contents/MacOS/.
# Apple reserves Contents/MacOS/ for Mach-O executables only. The PyInstaller
# onedir payload is hundreds of .py files plus *.dist-info directories;
# codesign treats EVERY file under MacOS/ as nested code and aborts the whole
# bundle signature ("code object is not signed at all" / "bundle format
# unrecognized"). Under Resources/ those same files are sealed as data and the
# bundle signs cleanly. The path Contents/Resources/bin/ytdlp is already on the
# runtime PATH: main.cpp adds work_dir/bin/ytdlp (work_dir == Resources in
# bundle mode), so no C++ change is required.
# (YTDLP_DEST is defined near the top; the signing step references it too.)

if [ "$VARIANT" = "mas" ]; then
    # Mac App Store build ships NO yt-dlp. It is a spawned executable (App Store
    # guideline 2.5.2) with no in-process, MAS-legal form; the CM_MAS binary has
    # no YouTube plugin and drops YouTube catalog rows at index time. Bundling
    # yt-dlp here is exactly the thing that would get the app rejected, so skip it.
    echo "-> MAS build: skipping yt-dlp helper (no YouTube; App Store 2.5.2)."
elif [ -d "${CHIPMACHINE_DIR}/bin" ]; then
    echo "-> Packaging helper binaries into bundle (arm64 only)..."
    # NOTE: the ffmpeg CLI is NO LONGER bundled. FFMPEGPlugin now decodes
    # in-process via the linked libav* libraries (see the LGPL libav step in
    # section 5), so there is no spawned `ffmpeg -i` executable. Removing the
    # ~51MB Contents/MacOS/ffmpeg drops the App-Store 2.5.2 spawned-executable
    # blocker (and the GPL CLI). yt-dlp resolves URLs with --get-url only and
    # does not need the ffmpeg CLI.

    # yt-dlp: PyInstaller *onedir* bundle (fast ~0.1s cold start). Copy the
    # whole directory (yt-dlp exe + _internal/) into Contents/Resources/bin/ytdlp.
    if [ -d "${CHIPMACHINE_DIR}/bin/ytdlp" ]; then
        mkdir -p "${RESOURCES_DIR}/bin"
        rm -rf "${YTDLP_DEST}"
        cp -R "${CHIPMACHINE_DIR}/bin/ytdlp" "${YTDLP_DEST}"
        chmod +x "${YTDLP_DEST}/yt-dlp"
    else
        echo "WARNING: bin/ytdlp onedir not found. YouTube playback will be slow/broken."
    fi

else
    echo "WARNING: bin folder not found at ${CHIPMACHINE_DIR}/bin. YouTube playback will fail."
fi

# SunVox engine: prebuilt, MIT-licensed shared library that SunVoxPlugin
# dlopen()s at runtime from next to the executable (Environment::getExeDir()).
# It is a lone arm64 Mach-O dylib, legal in Contents/MacOS/ and signed
# individually in step 7. The vendored copy is arm64-only so the step 5b
# architecture check passes.
SUNVOX_DYLIB_SRC="${CHIPMACHINE_DIR}/external/musicplayer/src/plugins/sunvoxplugin/sunvox_lib/sunvox.dylib"
if [ -f "${SUNVOX_DYLIB_SRC}" ]; then
    echo "-> Packaging SunVox engine (sunvox.dylib) into bundle..."
    cp "${SUNVOX_DYLIB_SRC}" "${MAC_OS_DIR}/sunvox.dylib"
    chmod +w "${MAC_OS_DIR}/sunvox.dylib"
else
    echo "WARNING: sunvox.dylib not found at ${SUNVOX_DYLIB_SRC}. .sunvox playback will fail."
fi

# *** 4b. (removed) .nsfe music tracks are no longer bundled ***
#
# music/Console used to be copied into Contents/Resources/music/Console/ here.
# db.lua VERSION 130 un-bundled it: no music may ship inside the app for Mac App
# Store submission. The nsfe db.lua entry has no local_dir and a real `source`
# now, so the 1224 Famicompo .nsfe are pulled per song out of an archive.org ZIP
# (https://archive.org/details/famicompo-nsfe) the same way keygenmusic and
# vgmrips already work. Do not re-add a copy step here.
# -----------------------------------------------------------------

# *** 4c. (removed) HVTC .prg tracks are no longer bundled ***
#
# HVTC (Commodore 16/116/+4 TED music) used to be copied into
# Contents/Resources/music/hvtc/ here, mirroring music/Console for .nsfe. db.lua
# VERSION 129 un-bundled it: no music may ship inside the app for Mac App Store
# submission, and the plus4world host that forced the v65 pivot is fast and
# reliable again. The db.lua entry has no local_dir, so .prg tracks are fetched
# on demand (live primary, Wayback fallback). Do not re-add a copy step here.
# -----------------------------------------------------------------

# *** 4d. (removed) Project AY .ay tracks are no longer bundled ***
#
# music/projectay used to be copied into Contents/Resources/music/projectay/ here.
# db.lua VERSION 131 un-bundled it -- the last of the three, after nsfe (130) and
# hvtc (129) -- so THE APP NOW SHIPS NO MUSIC AT ALL and chipmachine/music/ is gone
# entirely. That was the whole point: no bundled music for Mac App Store submission.
# The 613 .ay are pulled per song from https://archive.org/details/bulba-projectay,
# a repack of Bulba's three archives that preserves the paths in data/projectay.txt.
#
# DO NOT add a section 4e for a future collection. Ship its files as an archive.org
# ZIP and give the db.lua entry a `source` instead -- IA serves ZIP members over
# plain HTTP (see nsfe, keygenmusic, vgmrips).
# -----------------------------------------------------------------

# 5. Fix Native ARM64 Dynamic Library Linkages Deeply
echo "-> Resolving recursive dynamic library paths..."

typeset -A PROCESSED_LIBS

discover_and_patch() {
    local TARGET_FILE_PATH="$1"

    if ! file "$TARGET_FILE_PATH" | grep -q "Mach-O"; then
        return 0
    fi

    otool -L "$TARGET_FILE_PATH" | grep -E '/opt/homebrew/|/usr/local/' | awk '{print $1}' | while read -r RAW_LIB; do
        local LIB=$(echo "$RAW_LIB" | tr -d '[:space:]')
        [ -z "$LIB" ] && continue

        local LIB_BASE=$(basename "$LIB")
        local DEST_LIB_PATH="${MAC_OS_DIR}/${LIB_BASE}"

        if [ -z "${PROCESSED_LIBS[$LIB_BASE]}" ]; then
            echo "    Isolating dependency: $LIB_BASE (Required by $(basename "$TARGET_FILE_PATH"))"

            if [ ! -f "$DEST_LIB_PATH" ]; then
                cp "$LIB" "$DEST_LIB_PATH"
                chmod +w "$DEST_LIB_PATH"
            fi

            PROCESSED_LIBS[$LIB_BASE]=1
            discover_and_patch "$DEST_LIB_PATH"
        fi

        echo "    [Patching Executable Linkage] inside $(basename "$TARGET_FILE_PATH"): changing $LIB -> @executable_path/$LIB_BASE"
        install_name_tool -change "$LIB" "@executable_path/$LIB_BASE" "$TARGET_FILE_PATH"
    done

    if [[ "$TARGET_FILE_PATH" == *.dylib ]]; then
        install_name_tool -id "@executable_path/$(basename "$TARGET_FILE_PATH")" "$TARGET_FILE_PATH"
    fi
}

# --- Ship LGPL (not GPL) libav ---
# The build links Homebrew's FFmpeg, which is a GPL build (--enable-gpl + libx264/
# libx265) and incompatible with App Store terms. Overwrite the four libav dylibs
# in MacOS/ with our vendored LGPLv3, decode-only build BEFORE discover_and_patch
# runs. That function skips copying a dylib whose file already exists (so it keeps
# these LGPL ones instead of pulling Homebrew's GPL copies via otool), then still
# rewrites their openssl/sibling refs + id to @executable_path. Sonames match
# (avcodec.62/avformat.62/avutil.60/swresample.6) so it is an ABI drop-in.
# See external/ffmpeg-lgpl/README.md for provenance + license.
FFMPEG_LGPL_DIR="${CHIPMACHINE_DIR}/external/ffmpeg-lgpl/lib"
echo "-> Substituting LGPL libav dylibs (replacing the GPL Homebrew build)..."
for L in libavcodec.62 libavformat.62 libavutil.60 libswresample.6; do
    if [ -f "${FFMPEG_LGPL_DIR}/${L}.dylib" ]; then
        cp -f "${FFMPEG_LGPL_DIR}/${L}.dylib" "${MAC_OS_DIR}/${L}.dylib"
        chmod +w "${MAC_OS_DIR}/${L}.dylib"
    else
        echo "CRITICAL: vendored LGPL ${L}.dylib not found at ${FFMPEG_LGPL_DIR}."
        echo "          Build it via external/ffmpeg-lgpl/build_lgpl_ffmpeg.sh."
        $RELEASE_IT && exit 1
    fi
done

for EXE in "${MAC_OS_DIR}/"*; do
    if [ -f "$EXE" ] && [ -x "$EXE" ] && [ ! -L "$EXE" ]; then
        discover_and_patch "$EXE"
    fi
done

echo "-> Patching compiled Python native extensions inside bundle..."
if [ -d "${RESOURCES_DIR}/data/python_runtime" ]; then
    find "${RESOURCES_DIR}/data/python_runtime" -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.bundle" \) | while read -r PYTHON_EXT; do
        discover_and_patch "$PYTHON_EXT"
    done
fi

# 5b. Verify ALL bundled Mach-O binaries are pure arm64 (no Intel slices).
# Runs after dylib bundling so every copied library is covered — not just
# the helper executables that existed before step 5. Scans both Contents/MacOS/
# (main exe, ffmpeg, bundled dylibs) and the yt-dlp tree in Resources/bin
# (its yt-dlp exe + every *.so/*.dylib under _internal/).
# Uses process substitution (< <(...)) so the while loop runs in the current
# shell, not a subshell — this ensures `exit 1` aborts the whole script.
echo "-> Verifying all bundled Mach-O files are arm64-only..."
while read -r MACH_O_CANDIDATE; do
    if file "$MACH_O_CANDIDATE" | grep -q "Mach-O"; then
        if file "$MACH_O_CANDIDATE" | grep -qE "x86_64|i386|Intel"; then
            echo "CRITICAL: $MACH_O_CANDIDATE contains a non-arm64 slice. Aborting."
            exit 1
        fi
    fi
done < <(find "${MAC_OS_DIR}" "${YTDLP_DEST}" -type f 2>/dev/null)

# 6. Build the Icons
if [ -f "${ICON_PATH}" ]; then
    echo "-> Compiling application icon from local icon.png..."
    mkdir -p "${CHIPMACHINE_DIR}/temp.iconset"
    sips -z 16 16     "${ICON_PATH}" --out "${CHIPMACHINE_DIR}/temp.iconset/icon_16x16.png"
    sips -z 32 32     "${ICON_PATH}" --out "${CHIPMACHINE_DIR}/temp.iconset/icon_16x16@2x.png"
    sips -z 32 32     "${ICON_PATH}" --out "${CHIPMACHINE_DIR}/temp.iconset/icon_32x32.png"
    sips -z 64 64     "${ICON_PATH}" --out "${CHIPMACHINE_DIR}/temp.iconset/icon_32x32@2x.png"
    sips -z 128 128   "${ICON_PATH}" --out "${CHIPMACHINE_DIR}/temp.iconset/icon_128x128.png"
    sips -z 256 256   "${ICON_PATH}" --out "${CHIPMACHINE_DIR}/temp.iconset/icon_128x128@2x.png"
    sips -z 256 256   "${ICON_PATH}" --out "${CHIPMACHINE_DIR}/temp.iconset/icon_256x256.png"
    sips -z 512 512   "${ICON_PATH}" --out "${CHIPMACHINE_DIR}/temp.iconset/icon_256x256@2x.png"
    sips -z 512 512   "${ICON_PATH}" --out "${CHIPMACHINE_DIR}/temp.iconset/icon_512x512.png"
    sips -z 1024 1024 "${ICON_PATH}" --out "${CHIPMACHINE_DIR}/temp.iconset/icon_512x512@2x.png"
    iconutil -c icns "${CHIPMACHINE_DIR}/temp.iconset" -o "${RESOURCES_DIR}/AppIcon.icns"
    rm -rf "${CHIPMACHINE_DIR}/temp.iconset"

    # Document icon shown in Finder for files typed as our exported umbrella UTI
    # (org.mihailod.chipmachineas.chiptune). Info.plist references DocIcon.icns
    # via CFBundleTypeIconFile. For now this reuses the app icon; drop a distinct
    # "note on a document" DocIcon.icns here later for a dedicated file look.
    if [ -f "${RESOURCES_DIR}/AppIcon.icns" ]; then
        echo "-> Installing document icon (DocIcon.icns)..."
        cp "${RESOURCES_DIR}/AppIcon.icns" "${RESOURCES_DIR}/DocIcon.icns"
    fi
fi

else
    # --reusebuiltapp: the build/assemble steps above were skipped. Re-sign the
    # bundle already on disk, so it must exist and be a complete .app.
    echo "-> --reusebuiltapp: skipping build; re-signing existing ${TARGET_DIR}"
    if [ ! -d "${TARGET_DIR}" ] || [ ! -x "${MAC_OS_DIR}/chipmachine" ]; then
        echo "CRITICAL ERROR: no built bundle at ${TARGET_DIR} (missing app or main executable)."
        echo "                Build it first, e.g.: ${0:t} --buildapponly"
        exit 1
    fi
fi   # end: if $DO_BUILD (BUILD + ASSEMBLE)

# 7. Apply code signatures (ad-hoc by default, Developer ID when requested)
#
# Identity selection:
#   * Default (SIGN_ID unset): ad-hoc ("-"). Fine for local/dev builds; produces
#     the same self-signed bundle as before.
#   * SIGN_ID="Developer ID Application: Name (TEAMID)": a real, distributable
#     signature. We additionally enable Hardened Runtime (--options runtime) and
#     embed a secure Apple timestamp (--timestamp) — both REQUIRED for
#     notarization — plus the per-target entitlements in src/macnative/.
#     Optionally set NOTARY_PROFILE (see step 7c) to notarize + staple.
#
# Strategy (unchanged): sign each nested Mach-O individually (deepest content
# first), then seal the outer bundle LAST — and WITHOUT --deep.
#
# Why not --deep: --deep recursively descends into the yt-dlp PyInstaller tree
# and tries to interpret its package/*.dist-info directories as nested bundles,
# failing with "bundle format unrecognized, invalid, or unsuitable". Because the
# tree now lives in Contents/Resources/ (not MacOS/), codesign seals it as data
# via the normal resource envelope, so the plain bundle seal handles it
# correctly. We only need to individually sign the actual Mach-O code: the main
# executable, ffmpeg and bundled dylibs in MacOS/, plus yt-dlp and every
# *.so/*.dylib under the Resources ytdlp tree.
#
# Under Hardened Runtime the yt-dlp launcher (a PyInstaller freeze that dlopen()s
# an embedded Python framework + dozens of .so) fails library validation, so it
# alone is signed with entitlements-helper.plist
# (com.apple.security.cs.disable-library-validation). The outer seal signs the
# main executable with entitlements-app.plist.
#
# SIGN_ID and NOTARY_PROFILE were resolved from the CLI flags at the top of this
# script ("-" == ad-hoc when --applesign was not given).
# ENT_APP is resolved per-variant in the identity block above:
#   plus -> entitlements-app.plist  (Developer ID; disable-library-validation)
#   mas  -> entitlements-app-mas.plist  (App Sandbox + network.client)
# ENT_HELPER (yt-dlp) is used only by the plus variant.
ENT_HELPER="${MACNATIVE_DIR}/entitlements-helper.plist"

if command -v codesign &> /dev/null; then
    if [ "$SIGN_ID" = "-" ]; then
        SIGN_FLAGS=(-f -s -)
        echo "-> Applying ad-hoc code signatures (set an identity for a real signature)..."
    else
        SIGN_FLAGS=(-f -s "$SIGN_ID" --options runtime --timestamp)
        if [ "$VARIANT" = "mas" ]; then
            echo "-> Applying Mac App Store signatures: ${SIGN_ID}"
        else
            echo "-> Applying Developer ID signatures: ${SIGN_ID}"
        fi
        # Stray extended attributes (quarantine/FinderInfo) would break the seal.
        xattr -cr "${TARGET_DIR}"
    fi

    # The app entitlements go on the outer seal for any real signature AND for
    # every mas build -- the App Sandbox key must be present even in a local
    # ad-hoc test build for the app to actually run sandboxed. Verify presence.
    if [ "$SIGN_ID" != "-" ] || [ "$VARIANT" = "mas" ]; then
        [ -f "$ENT_APP" ] || { echo "CRITICAL: missing entitlements file ${ENT_APP}"; exit 1; }
    fi
    # The yt-dlp helper (plus, real signing only) needs its own entitlements.
    if [ "$SIGN_ID" != "-" ] && [ "$VARIANT" = "plus" ]; then
        [ -f "$ENT_HELPER" ] || { echo "CRITICAL: missing entitlements file ${ENT_HELPER}"; exit 1; }
    fi

    # Mac App Store: embed the provisioning profile before signing (real sign
    # only; PROVISION was validated non-empty for --mas --applesign up top).
    if [ "$VARIANT" = "mas" ] && [ "$SIGN_ID" != "-" ]; then
        echo "-> Embedding Mac App Store provisioning profile..."
        cp "$PROVISION" "${TARGET_DIR}/Contents/embedded.provisionprofile"
    fi

    if [ -d "${RESOURCES_DIR}/data/python_runtime" ]; then
        find "${RESOURCES_DIR}/data/python_runtime" -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.bundle" \) | while read -r py_ext; do
            codesign "${SIGN_FLAGS[@]}" "$py_ext"
        done
    fi

    # Sign every Mach-O under MacOS/ and (plus only) the Resources ytdlp tree.
    # Filter to Mach-O only — codesign rejects .py, .pyc, and other data files.
    # The yt-dlp launcher gets the helper entitlements (plus, real signing only);
    # for the mas variant YTDLP_DEST does not exist and this branch never matches.
    find "${MAC_OS_DIR}" "${YTDLP_DEST}" -type f 2>/dev/null | while read -r mf; do
        file "$mf" | grep -q "Mach-O" || continue
        if [ "$SIGN_ID" != "-" ] && [ "$VARIANT" = "plus" ] && [ "$mf" = "${YTDLP_DEST}/yt-dlp" ]; then
            codesign "${SIGN_FLAGS[@]}" --entitlements "$ENT_HELPER" "$mf"
        else
            codesign "${SIGN_FLAGS[@]}" "$mf"
        fi
    done

    # Seal the bundle (no --deep — see header comment). App entitlements are
    # attached for real signatures and for every mas build (sandbox even ad-hoc).
    if [ "$SIGN_ID" != "-" ] || [ "$VARIANT" = "mas" ]; then
        codesign "${SIGN_FLAGS[@]}" --entitlements "$ENT_APP" "${TARGET_DIR}"
    else
        codesign "${SIGN_FLAGS[@]}" "${TARGET_DIR}"
    fi
    echo "-> Code signing complete."

    # Hard-verify the finished bundle so a signing regression fails the build.
    if ! codesign --verify --deep --strict "${TARGET_DIR}" 2>/dev/null; then
        echo "CRITICAL: codesign verification of ${TARGET_DIR} failed. Aborting."
        exit 1
    fi
    echo "-> Code signature verified (--deep --strict)."
fi

echo "=== Success: ${APP_NAME} generated cleanly in workspace root! ==="

ZIP_PATH="${WORKSPACE_ROOT}/${ARTIFACT}.zip"
PKG_PATH="${WORKSPACE_ROOT}/${ARTIFACT}.pkg"

if [ "$VARIANT" = "mas" ]; then
    # Mac App Store distribution is a signed .pkg uploaded via Transporter/altool,
    # NOT a zip. Build it only when a real installer identity is available;
    # otherwise the ad-hoc .app is a LOCAL-test artifact only.
    if [ "$SIGN_ID" != "-" ]; then
        echo "=== Building signed .pkg for App Store submission... ==="
        rm -f "${PKG_PATH}"
        productbuild --component "${TARGET_DIR}" /Applications \
            --sign "${INSTALLER_ID}" "${PKG_PATH}"
        echo "-> App Store package: ${PKG_PATH}"
        echo "   Upload with Transporter.app, or:"
        echo "     xcrun altool --upload-app -f \"${PKG_PATH}\" -t macos \\"
        echo "                  --apple-id you@example.com --password <app-specific-pw>"
    else
        echo "=== MAS ad-hoc test build ready: ${TARGET_DIR} ==="
        echo "-> Ad-hoc, sandboxed .app for LOCAL testing only (no .pkg). For an App"
        echo "   Store submission, re-run with real identities:"
        echo "     package_app.sh --mas --applesign --distribid=... --installerid=... --provision=..."
    fi
else
    echo "=== Making the final distribution package... ==="
    cd "${WORKSPACE_ROOT}"
    rm -f "${ZIP_PATH}"
    zip -r -y "${ZIP_PATH}" ./${APP_NAME}

    # 7b. Verify the SHIPPED ARTIFACT, not just the on-disk bundle.
    #
    # The codesign check in step 7 validates ${TARGET_DIR} as it sits on disk.
    # That is NOT enough: it cannot catch a desync where the .zip ends up
    # containing files that were never part of the sealed manifest (e.g. extra
    # UADE player files that appear in the bundle after signing). Such a zip
    # passes step 7 yet ships a bundle whose contents do not match its signature
    # — and on macOS 13+ a quarantined download with a mismatched seal is
    # reported as "<App> is damaged and can't be opened", a hard block with no
    # right-click bypass. We extract the real zip to a scratch dir and run the
    # same strict verification against THAT, failing the build on any mismatch.
    echo "-> Verifying the packaged zip artifact (extract + strict codesign)..."
    VERIFY_DIR="$(mktemp -d)"
    ( cd "${VERIFY_DIR}" && unzip -q "${ZIP_PATH}" )
    if ! codesign --verify --deep --strict "${VERIFY_DIR}/${APP_NAME}" 2>/dev/null; then
        echo "CRITICAL: the packaged zip's signature does not match its contents."
        echo "          The shipped bundle would be reported as 'damaged' on download."
        codesign --verify --deep --strict --verbose=2 "${VERIFY_DIR}/${APP_NAME}" 2>&1 | grep -E "file added|missing|invalid" | head
        rm -rf "${VERIFY_DIR}"
        exit 1
    fi
    rm -rf "${VERIFY_DIR}"
    echo "-> Packaged zip artifact verified (--deep --strict)."
fi

# 7c. Notarize with Apple + staple the ticket (Developer ID distribution only).
#
# Signing alone is NOT enough for other Macs: since macOS 10.15 a downloaded
# (quarantined) app must also be notarized by Apple and have the ticket stapled,
# or Gatekeeper blocks it ("cannot be checked for malicious software"). This runs
# only when a real SIGN_ID is used AND NOTARY_PROFILE names a stored credential
# profile (create once via `xcrun notarytool store-credentials <profile>
# --apple-id you@example.com --team-id TEAMID`).
# (Notarization applies to the Developer ID / plus variant only. The mas variant
# is reviewed and signed by Apple through App Store Connect, not notarytool.)
if [ "$VARIANT" = "plus" ] && [ "$SIGN_ID" != "-" ] && [ -n "$NOTARY_PROFILE" ]; then
    echo "-> Notarizing with Apple (profile: ${NOTARY_PROFILE}); this can take a few minutes..."
    if ! xcrun notarytool submit "${ZIP_PATH}" \
            --keychain-profile "${NOTARY_PROFILE}" --wait; then
        echo "CRITICAL: notarization was not accepted. Inspect the log with:"
        echo "          xcrun notarytool history --keychain-profile ${NOTARY_PROFILE}"
        exit 1
    fi
    echo "-> Stapling notarization ticket into the .app..."
    xcrun stapler staple "${TARGET_DIR}"
    xcrun stapler validate "${TARGET_DIR}"

    # Stapling mutates the bundle on disk, so the earlier zip is now stale —
    # re-zip the STAPLED app for distribution.
    echo "-> Re-zipping the stapled app for distribution..."
    cd "${WORKSPACE_ROOT}"
    rm -f "${ZIP_PATH}"
    zip -r -y "${ZIP_PATH}" ./${APP_NAME}
    cd "${CHIPMACHINE_DIR}"

    echo "-> Final Gatekeeper assessment (expect: accepted / Notarized Developer ID):"
    spctl -a -t exec -vvv "${TARGET_DIR}" || true
elif [ "$VARIANT" = "plus" ] && [ "$SIGN_ID" != "-" ]; then
    echo "-> NOTE: signed with Developer ID but NOT notarized (NOTARY_PROFILE unset)."
    echo "         Un-notarized downloads still trip Gatekeeper on other Macs."
fi

cd "${CHIPMACHINE_DIR}"

echo "=== Done! ==="
printf '=== Total packaging time: %dm %02ds ===\n' $((SECONDS / 60)) $((SECONDS % 60))

# The GitHub release flow applies to the plus (Developer ID) variant only; the
# mas variant is distributed through the Mac App Store, not GitHub.
if [ "$VARIANT" = "plus" ]; then
    echo "*** Planned template command details:"
    echo "------------------------------------------------------------"
    echo "gh release create v${VERSION_STR}-as ../${ARTIFACT}.zip \\"
    echo "  --title \"${DISPLAY_NAME} v${VERSION_STR}\" \\"
    echo "  --notes \"Apple Silicon maintenance release v${VERSION_STR}. <short note text to be provided>\" \\"
    echo "  --repo \"mihailod/chipmachine\""
    echo "------------------------------------------------------------"

    # -----------------------------------------------------------------
    # Conditional Interactive Release Verification Block
    # -----------------------------------------------------------------
    if $RELEASE_IT; then
        if ! command -v gh &> /dev/null; then
            echo "ERROR: 'gh' command line tool not found in PATH. Skipping automated execution."
            exit 1
        fi

        printf "Provide release notes and confirm the official release upload to GitHub per command above [Y/N] ? " >&2
        read -r RESPONSE

        if [[ "$RESPONSE" == "y" || "$RESPONSE" == "Y" ]]; then
            printf "Release short note (CTRL+C to abort): " >&2
            read -r SHORT_NOTE

            RELEASE_NOTES="Apple Silicon maintenance release v${VERSION_STR}. ${SHORT_NOTE}"

            echo "-> Initiating deployment via GitHub CLI..."
            gh release create "v${VERSION_STR}-as" "${ZIP_PATH}" \
              --title "${DISPLAY_NAME} v${VERSION_STR}" \
              --notes "${RELEASE_NOTES}" \
              --repo "mihailod/chipmachine"
            echo "=== Deployment Successfully Completed ==="
        else
            echo "-> Deployment aborted by user request."
        fi
    fi
elif $RELEASE_IT; then
    echo "-> NOTE: --releaseit has no effect for the mas variant (App Store distribution)."
fi
