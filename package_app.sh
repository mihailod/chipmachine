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
BUILD_DIR="${WORKSPACE_ROOT}/build"
APP_NAME="ChipMachineAS.app"
TARGET_DIR="${WORKSPACE_ROOT}/${APP_NAME}"
ICON_PATH="${CHIPMACHINE_DIR}/data/misc/icon.png"

# Target payload directories. Defined unconditionally (not inside the build
# block) because the signing step reads them too, including on the
# --reusebuiltapp path where the build/assemble steps are skipped.
MAC_OS_DIR="${TARGET_DIR}/Contents/MacOS"
RESOURCES_DIR="${TARGET_DIR}/Contents/Resources"
MACNATIVE_DIR="${CHIPMACHINE_DIR}/src/macnative"
YTDLP_DEST="${RESOURCES_DIR}/bin/ytdlp"

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
ChipMachineAS -- app bundle packaging & signing

Usage:
  package_app.sh --buildapponly [--releaseit]
  package_app.sh --applesign --signid="Developer ID Application: Name (TEAMID)" \
                 [--notaryprofile=NAME] [--reusebuiltapp] [--releaseit]

Actions (at least one of --buildapponly / --applesign required):
  --buildapponly         Build ChipMachineAS.app + zip, ad-hoc self-signed, and
                         stop. This is the previous default behavior.
  --applesign            Build, then sign with a Developer ID identity and enable
                         Hardened Runtime. Requires --signid.

Signing options (used with --applesign):
  --signid="..."         Developer ID signing identity, e.g.
                         "Developer ID Application: Your Name (ABCDE12345)".
                         List yours: security find-identity -v -p codesigning
  --notaryprofile=NAME   Also notarize with Apple and staple the ticket, so
                         downloaded copies open with no Gatekeeper warning.
                         NAME is a stored notarytool credential profile, created
                         once (credentials live in the keychain, not on the CLI):
                           xcrun notarytool store-credentials NAME \
                               --apple-id you@example.com --team-id TEAMID
  --reusebuiltapp        Do NOT rebuild -- sign the ChipMachineAS.app that already
                         exists on disk (from a prior --buildapponly/--applesign
                         run). Use this to re-sign or re-notarize without a slow
                         rebuild. Cannot be combined with --buildapponly.

Other options:
  --releaseit            After packaging, interactively create a GitHub release.
  -h, --help             Show this help.

Examples:
  package_app.sh --buildapponly
  package_app.sh --applesign --signid="Developer ID Application: Jane (ABCDE12345)"
  package_app.sh --applesign --signid="Developer ID Application: Jane (ABCDE12345)" \
                 --notaryprofile=chipmachine-notary
  # re-sign the existing bundle only (no rebuild):
  package_app.sh --applesign --signid="Developer ID Application: Jane (ABCDE12345)" \
                 --notaryprofile=chipmachine-notary --reusebuiltapp
USAGE
}

DO_BUILD=false          # run the build/assemble steps (0-6)?
APPLE_SIGN=false        # Developer ID sign at the end?
REUSE_BUILT=false       # skip the build and sign the existing .app?
RELEASE_IT=false        # create a GitHub release afterwards? (modifier)
ACTION=false            # was any actionable flag (build/sign) supplied?
SIGN_ID=""
NOTARY_PROFILE=""

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
        -buildapponly|--buildapponly)       DO_BUILD=true; ACTION=true ;;
        -applesign|--applesign)             APPLE_SIGN=true; ACTION=true ;;
        -reusebuiltapp|--reusebuiltapp)     REUSE_BUILT=true ;;
        -signid|--signid)                   SIGN_ID="$val" ;;
        -notaryprofile|--notaryprofile)     NOTARY_PROFILE="$val" ;;
        -releaseit|--releaseit)             RELEASE_IT=true ;;
        *) echo "ERROR: unknown argument '$arg'"; echo; print_usage; exit 1 ;;
    esac
done

# Supplying a signing parameter implies the signing action.
if [ -n "$SIGN_ID" ] || [ -n "$NOTARY_PROFILE" ]; then
    APPLE_SIGN=true
    ACTION=true
fi

if $APPLE_SIGN && [ -z "$SIGN_ID" ]; then
    echo "ERROR: --applesign / --notaryprofile require --signid=\"Developer ID Application: ... (TEAMID)\""
    echo
    print_usage
    exit 1
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
echo "Target App Bundle: ${TARGET_DIR}"
echo "Detected Version: ${VERSION_STR}"
if $RELEASE_IT; then
    echo "Release Mode: Enabled (--releaseit Flag Detected)"
else
    echo "Release Mode: Disabled (Dry Run/Local Build Only)"
fi
if $APPLE_SIGN; then
    echo "Signing Mode: Developer ID (${SIGN_ID})"
    if [ -n "$NOTARY_PROFILE" ]; then
        echo "Notarization: Enabled (profile: ${NOTARY_PROFILE})"
    else
        echo "Notarization: Disabled (add --notaryprofile=NAME to notarize + staple)"
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
"${GEN_PLIST}" --version "${VERSION_STR}" > "${TARGET_DIR}/Contents/Info.plist"
if ! plutil -lint "${TARGET_DIR}/Contents/Info.plist" >/dev/null; then
    echo "CRITICAL ERROR: generated Info.plist failed plutil -lint. Aborting."
    exit 1
fi

# 4. Copy the asset and Lua payloads from the chipmachine source tree
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

if [ -d "${CHIPMACHINE_DIR}/bin" ]; then
    echo "-> Packaging helper binaries into bundle (arm64 only)..."
    # ffmpeg: single arm64 Mach-O executable. A lone binary is legal in MacOS/
    # and signs without issue; main.cpp finds it via exeDir on PATH.
    cp -L "${CHIPMACHINE_DIR}/bin/ffmpeg" "${MAC_OS_DIR}/"
    chmod +x "${MAC_OS_DIR}/ffmpeg"

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

# *** 4b. Bundle .nsfe music tracks into Contents/Resources/music/Console/ ***
#
# Strategy:
#   - Source: chipmachine/music/Console/*.nsfe  (and any sub-structure beneath it)
#   - Destination: ChipMachineAS.app/Contents/Resources/music/Console/
#
# The destination path intentionally mirrors the relative layout the C++ runtime
# expects so that CFBundleCopyResourcesDirectoryURL() + "/music/Console/" resolves
# to exactly these files in production. In local dev the binary reads the live
# source tree directly (see get_music_resource_path() in the C++ layer).
# -----------------------------------------------------------------
MUSIC_SRC="${CHIPMACHINE_DIR}/music/Console"
MUSIC_DEST="${RESOURCES_DIR}/music/Console"

if [ -d "${MUSIC_SRC}" ]; then
    # Count .nsfe files so we can emit a meaningful diagnostic
    NSFE_COUNT=$(find "${MUSIC_SRC}" -maxdepth 1 -name "*.nsfe" | wc -l | tr -d '[:space:]')
    if [ "${NSFE_COUNT}" -eq 0 ]; then
        echo "WARNING: music/Console/ exists but contains no .nsfe files. Bundle music will be empty."
    else
        echo "-> Bundling ${NSFE_COUNT} .nsfe track(s) into ${MUSIC_DEST} ..."
    fi

    mkdir -p "${MUSIC_DEST}"

    # Use cp -R to preserve any sub-directory hierarchy that may exist under Console/
    # (e.g. Console/Famicom/, Console/GameBoy/) while still being safe for a flat layout.
    cp -R "${MUSIC_SRC}/." "${MUSIC_DEST}/"

    # Verify the copy succeeded and at least one .nsfe landed in the bundle
    BUNDLED_COUNT=$(find "${MUSIC_DEST}" -name "*.nsfe" | wc -l | tr -d '[:space:]')
    echo "   Verified ${BUNDLED_COUNT} .nsfe file(s) present inside bundle."
else
    # Treat a missing music directory as a hard error in release mode;
    # warn-only for local/dry-run builds so CI without music assets doesn't break.
    if $RELEASE_IT; then
        echo "CRITICAL ERROR: music/Console source directory not found at ${MUSIC_SRC}!"
        echo "               A release build MUST include bundled .nsfe tracks."
        exit 1
    else
        echo "WARNING: music/Console not found at ${MUSIC_SRC}. Skipping music bundling (dry-run mode)."
        echo "         End-users will have no bundled tracks. Set up the music/ directory before --releaseit."
    fi
fi

# *** 4c. Bundle HVTC .prg tracks into Contents/Resources/music/hvtc/ ***
#
# HVTC (Commodore 16/116/+4 TED music) was pivoted from the flaky online
# plus4world/Wayback mirror to a shipped local store, exactly like music/Console
# for .nsfe. The runtime resolves these via local_dir = "music/hvtc" (db.lua),
# so the destination layout (sub-dirs demos/ games/ musicians/ other/) is
# preserved verbatim. Without this, .prg playback would fall back to the network.
# -----------------------------------------------------------------
HVTC_SRC="${CHIPMACHINE_DIR}/music/hvtc"
HVTC_DEST="${RESOURCES_DIR}/music/hvtc"

if [ -d "${HVTC_SRC}" ]; then
    PRG_COUNT=$(find "${HVTC_SRC}" -name "*.prg" | wc -l | tr -d '[:space:]')
    if [ "${PRG_COUNT}" -eq 0 ]; then
        echo "WARNING: music/hvtc/ exists but contains no .prg files. HVTC bundle will be empty."
    else
        echo "-> Bundling ${PRG_COUNT} HVTC .prg track(s) into ${HVTC_DEST} ..."
    fi

    mkdir -p "${HVTC_DEST}"

    # Preserve the demos/ games/ musicians/ other/ sub-directory hierarchy.
    cp -R "${HVTC_SRC}/." "${HVTC_DEST}/"

    BUNDLED_PRG_COUNT=$(find "${HVTC_DEST}" -name "*.prg" | wc -l | tr -d '[:space:]')
    echo "   Verified ${BUNDLED_PRG_COUNT} .prg file(s) present inside bundle."
else
    if $RELEASE_IT; then
        echo "CRITICAL ERROR: music/hvtc source directory not found at ${HVTC_SRC}!"
        echo "               A release build MUST include bundled HVTC .prg tracks."
        exit 1
    else
        echo "WARNING: music/hvtc not found at ${HVTC_SRC}. Skipping HVTC bundling (dry-run mode)."
        echo "         End-users will have no bundled HVTC tracks. Set up music/hvtc before --releaseit."
    fi
fi

# *** 4d. Bundle Project AY .ay tracks into Contents/Resources/music/projectay/ ***
#
# Project AY (Sergey Bulba / Ironfist raw Z80 .ay rips) is a locally-shipped
# collection, exactly like music/Console (.nsfe) and music/hvtc (.prg). The
# runtime resolves these via local_dir = "music/projectay" (db.lua) with an
# EMPTY source, so if the .ay files are not bundled there is no network fallback
# and every tune fails to load. The ironfist/ bulba/ cpc/ sub-directory
# hierarchy is preserved verbatim (the DB song paths are relative to it).
# -----------------------------------------------------------------
PROJECTAY_SRC="${CHIPMACHINE_DIR}/music/projectay"
PROJECTAY_DEST="${RESOURCES_DIR}/music/projectay"

if [ -d "${PROJECTAY_SRC}" ]; then
    AY_COUNT=$(find "${PROJECTAY_SRC}" -name "*.ay" | wc -l | tr -d '[:space:]')
    if [ "${AY_COUNT}" -eq 0 ]; then
        echo "WARNING: music/projectay/ exists but contains no .ay files. Project AY bundle will be empty."
    else
        echo "-> Bundling ${AY_COUNT} Project AY .ay track(s) into ${PROJECTAY_DEST} ..."
    fi

    mkdir -p "${PROJECTAY_DEST}"

    # Preserve the ironfist/ bulba/ cpc/ sub-directory hierarchy.
    cp -R "${PROJECTAY_SRC}/." "${PROJECTAY_DEST}/"

    BUNDLED_AY_COUNT=$(find "${PROJECTAY_DEST}" -name "*.ay" | wc -l | tr -d '[:space:]')
    echo "   Verified ${BUNDLED_AY_COUNT} .ay file(s) present inside bundle."
else
    if $RELEASE_IT; then
        echo "CRITICAL ERROR: music/projectay source directory not found at ${PROJECTAY_SRC}!"
        echo "               A release build MUST include bundled Project AY .ay tracks."
        exit 1
    else
        echo "WARNING: music/projectay not found at ${PROJECTAY_SRC}. Skipping Project AY bundling (dry-run mode)."
        echo "         End-users will have no bundled Project AY tracks. Set up music/projectay before --releaseit."
    fi
fi

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
ENT_APP="${MACNATIVE_DIR}/entitlements-app.plist"
ENT_HELPER="${MACNATIVE_DIR}/entitlements-helper.plist"

if command -v codesign &> /dev/null; then
    if [ "$SIGN_ID" = "-" ]; then
        SIGN_FLAGS=(-f -s -)
        echo "-> Applying ad-hoc code signatures (set SIGN_ID for a Developer ID signature)..."
    else
        SIGN_FLAGS=(-f -s "$SIGN_ID" --options runtime --timestamp)
        echo "-> Applying Developer ID signatures: ${SIGN_ID}"
        [ -f "$ENT_APP" ]    || { echo "CRITICAL: missing entitlements file ${ENT_APP}"; exit 1; }
        [ -f "$ENT_HELPER" ] || { echo "CRITICAL: missing entitlements file ${ENT_HELPER}"; exit 1; }
        # Stray extended attributes (quarantine/FinderInfo) would break the seal.
        xattr -cr "${TARGET_DIR}"
    fi

    if [ -d "${RESOURCES_DIR}/data/python_runtime" ]; then
        find "${RESOURCES_DIR}/data/python_runtime" -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.bundle" \) | while read -r py_ext; do
            codesign "${SIGN_FLAGS[@]}" "$py_ext"
        done
    fi

    # Sign every Mach-O under MacOS/ and the Resources ytdlp tree.
    # Filter to Mach-O only — codesign rejects .py, .pyc, and other data files.
    # The yt-dlp launcher gets the helper entitlements (real signing only).
    find "${MAC_OS_DIR}" "${YTDLP_DEST}" -type f 2>/dev/null | while read -r mf; do
        file "$mf" | grep -q "Mach-O" || continue
        if [ "$SIGN_ID" != "-" ] && [ "$mf" = "${YTDLP_DEST}/yt-dlp" ]; then
            codesign "${SIGN_FLAGS[@]}" --entitlements "$ENT_HELPER" "$mf"
        else
            codesign "${SIGN_FLAGS[@]}" "$mf"
        fi
    done

    # Seal the bundle (no --deep — see header comment). Real signing attaches the
    # app entitlements to the main executable.
    if [ "$SIGN_ID" = "-" ]; then
        codesign "${SIGN_FLAGS[@]}" "${TARGET_DIR}"
    else
        codesign "${SIGN_FLAGS[@]}" --entitlements "$ENT_APP" "${TARGET_DIR}"
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
echo "=== Making the final distribution package... ==="

cd "${WORKSPACE_ROOT}"
rm -f ./ChipMachineAS.zip
zip -r -y ./ChipMachineAS.zip ./${APP_NAME}

# 7b. Verify the SHIPPED ARTIFACT, not just the on-disk bundle.
#
# The codesign check in step 7 validates ${TARGET_DIR} as it sits on disk. That
# is NOT enough: it cannot catch a desync where the .zip ends up containing files
# that were never part of the sealed manifest (e.g. extra UADE player files that
# appear in the bundle after signing). Such a zip passes step 7 yet ships a
# bundle whose contents do not match its signature — and on macOS 13+ a
# quarantined download with a mismatched seal is reported to the user as
# "<App> is damaged and can't be opened", a hard block with no right-click
# bypass. We therefore extract the real zip to a scratch dir and run the same
# strict verification against THAT, failing the build on any mismatch.
echo "-> Verifying the packaged zip artifact (extract + strict codesign)..."
VERIFY_DIR="$(mktemp -d)"
( cd "${VERIFY_DIR}" && unzip -q "${WORKSPACE_ROOT}/ChipMachineAS.zip" )
if ! codesign --verify --deep --strict "${VERIFY_DIR}/${APP_NAME}" 2>/dev/null; then
    echo "CRITICAL: the packaged zip's signature does not match its contents."
    echo "          The shipped bundle would be reported as 'damaged' on download."
    codesign --verify --deep --strict --verbose=2 "${VERIFY_DIR}/${APP_NAME}" 2>&1 | grep -E "file added|missing|invalid" | head
    rm -rf "${VERIFY_DIR}"
    exit 1
fi
rm -rf "${VERIFY_DIR}"
echo "-> Packaged zip artifact verified (--deep --strict)."

# 7c. Notarize with Apple + staple the ticket (Developer ID distribution only).
#
# Signing alone is NOT enough for other Macs: since macOS 10.15 a downloaded
# (quarantined) app must also be notarized by Apple and have the ticket stapled,
# or Gatekeeper blocks it ("cannot be checked for malicious software"). This runs
# only when a real SIGN_ID is used AND NOTARY_PROFILE names a stored credential
# profile (create once via `xcrun notarytool store-credentials <profile>
# --apple-id you@example.com --team-id TEAMID`).
if [ "$SIGN_ID" != "-" ] && [ -n "$NOTARY_PROFILE" ]; then
    echo "-> Notarizing with Apple (profile: ${NOTARY_PROFILE}); this can take a few minutes..."
    if ! xcrun notarytool submit "${WORKSPACE_ROOT}/ChipMachineAS.zip" \
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
    rm -f ./ChipMachineAS.zip
    zip -r -y ./ChipMachineAS.zip ./${APP_NAME}
    cd "${CHIPMACHINE_DIR}"

    echo "-> Final Gatekeeper assessment (expect: accepted / Notarized Developer ID):"
    spctl -a -t exec -vvv "${TARGET_DIR}" || true
elif [ "$SIGN_ID" != "-" ]; then
    echo "-> NOTE: signed with Developer ID but NOT notarized (NOTARY_PROFILE unset)."
    echo "         Un-notarized downloads still trip Gatekeeper on other Macs."
fi

cd "${CHIPMACHINE_DIR}"

echo "=== Done! ==="
printf '=== Total packaging time: %dm %02ds ===\n' $((SECONDS / 60)) $((SECONDS % 60))
echo "*** Planned template command details:"
echo "------------------------------------------------------------"
echo "gh release create v${VERSION_STR}-as ../ChipMachineAS.zip \\"
echo "  --title \"ChipMachineAS v${VERSION_STR}\" \\"
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
        gh release create "v${VERSION_STR}-as" "${WORKSPACE_ROOT}/ChipMachineAS.zip" \
          --title "ChipMachineAS v${VERSION_STR}" \
          --notes "${RELEASE_NOTES}" \
          --repo "mihailod/chipmachine"
        echo "=== Deployment Successfully Completed ==="
    else
        echo "-> Deployment aborted by user request."
    fi
fi
