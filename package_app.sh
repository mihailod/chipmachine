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
        APP_CATEGORY="$PLUS_APP_CATEGORY"
        ENT_APP="${MACNATIVE_DIR}/entitlements-app.plist" # Developer ID entitlements
        ;;
    mas)
        ARTIFACT="$MAS_ARTIFACT"; BUNDLE_ID="$MAS_BUNDLE_ID"; DISPLAY_NAME="$MAS_DISPLAY_NAME"
        BUILD_DIR="${WORKSPACE_ROOT}/build-mas"           # CM_VARIANT=mas build dir
        APP_CATEGORY="$MAS_APP_CATEGORY"
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
# Apple's bundle layout reserves Contents/MacOS/ for the main executable; every
# shared library belongs in Contents/Frameworks/ (loose .dylib files are fine
# there -- a full .framework wrapper is not required). See step 5 for how the
# link commands are rewritten so dyld finds them at the new location.
FRAMEWORKS_DIR="${TARGET_DIR}/Contents/Frameworks"
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
mkdir -p "${FRAMEWORKS_DIR}"
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
# The extension list is the union of every extension the plugins advertise, so
# the built binary is the only real source of truth -- we dump it here with
# `--dump-extensions` and feed that to gen_info_plist.sh.
#
# The dump goes into the BUILD DIR, never back into the source tree. It is
# VARIANT-SPECIFIC (mas has no aoplugin/uadeplugin/pokeynoiseplugin, so it
# advertises several hundred fewer extensions than plus), and this step used to
# overwrite the tracked src/macnative/extensions.txt in place -- which meant
# every package run left a large diff in git, and whichever variant you packaged
# LAST silently decided what was checked in. Writing per-variant into
# ${BUILD_DIR} keeps both lists correct and the working tree clean.
#
# src/macnative/extensions.txt stays tracked as the fallback, and as the input
# for dev_update_doctypes.sh (the no-recompile loop, which has no build dir to
# read). Refresh it by hand from a PLUS build when plugin coverage changes:
#     build/chipmachine --dump-extensions > src/macnative/extensions.txt
# MACNATIVE_DIR is defined near the top (needed by the signing step too).
GEN_PLIST="${MACNATIVE_DIR}/gen_info_plist.sh"
EXTS_FILE="${MACNATIVE_DIR}/extensions.txt"      # tracked fallback
EXTS_BUILT="${BUILD_DIR}/extensions.txt"         # generated, untracked

echo "-> Dumping playable-extension list from built binary..."
if "${BUILD_DIR}/chipmachine" --dump-extensions > "${EXTS_BUILT}.tmp" 2>/dev/null \
        && [ -s "${EXTS_BUILT}.tmp" ]; then
    mv "${EXTS_BUILT}.tmp" "${EXTS_BUILT}"
    EXTS_FILE="${EXTS_BUILT}"
    echo "   ${EXTS_BUILT}: $(wc -l < "${EXTS_FILE}" | tr -d '[:space:]') extensions (${VARIANT})"
else
    rm -f "${EXTS_BUILT}.tmp"
    echo "   WARNING: --dump-extensions failed; using checked-in extensions.txt"
fi

echo "-> Creating Info.plist (with macOS file associations)..."

# LSMinimumSystemVersion is READ OFF THE BINARY, never hardcoded. CMakeLists.txt
# derives CMAKE_OSX_DEPLOYMENT_TARGET from the build host's `sw_vers` unless it
# is overridden, so the real floor changes with whatever Mac did the build. The
# plist used to claim a fixed "11.0" regardless, which let users on older
# systems install a bundle whose binary refused to launch, and gave App Store
# ingestion two disagreeing minimum-OS values.
#
# LC_BUILD_VERSION/minos is the modern load command; LC_VERSION_MIN_MACOSX is
# the pre-10.14 form, kept as a fallback so this keeps working if the target is
# ever lowered far enough for the linker to emit the old command instead.
MIN_OS=$(otool -l "${BUILD_DIR}/chipmachine" 2>/dev/null \
    | awk '/LC_BUILD_VERSION/{f=1} f&&/minos/{print $2; exit}')
if [ -z "${MIN_OS}" ]; then
    MIN_OS=$(otool -l "${BUILD_DIR}/chipmachine" 2>/dev/null \
        | awk '/LC_VERSION_MIN_MACOSX/{f=1} f&&/version/{print $2; exit}')
fi
if [ -z "${MIN_OS}" ]; then
    echo "ERROR: could not read the deployment target from ${BUILD_DIR}/chipmachine."
    echo "       Refusing to guess -- a wrong LSMinimumSystemVersion ships an app"
    echo "       that installs on systems it cannot run on."
    exit 1
fi
echo "   LSMinimumSystemVersion: ${MIN_OS} (from the built binary)"

# Build args as an array (zsh does not word-split unquoted ${:+...}); append the
# app category only when the variant defines one in variants.conf. Both variants
# set it today (mas requires it, plus carries it for Finder/Launchpad grouping),
# but the guard stays so clearing *_APP_CATEGORY omits the key cleanly.
GEN_ARGS=(--version "${VERSION_STR}" --bundle-id "${BUNDLE_ID}" --display-name "${DISPLAY_NAME}" --exts "${EXTS_FILE}" --min-os "${MIN_OS}")
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

# App Store privacy manifest. Apple reads this ONLY at the Resources root, so it
# is copied flat -- not via the `cp -R data` above, which is also why the source
# lives in src/macnative/ next to the entitlements plists rather than in data/
# (a copy under data/ would be duplicated into Resources/data/ and sealed into
# the signature twice). Shipped in BOTH variants: mas needs it for App Store
# review, and carrying the same accurate manifest in plus costs nothing and
# keeps the two bundles from drifting.
#
# The declarations inside are derived from the built binary's actual imports --
# see the comment header in the file before editing it. Missing manifest = App
# Store rejection, so this is a hard error rather than a warning.
PRIVACY_SRC="${MACNATIVE_DIR}/PrivacyInfo.xcprivacy"
if [ -f "${PRIVACY_SRC}" ]; then
    cp "${PRIVACY_SRC}" "${RESOURCES_DIR}/PrivacyInfo.xcprivacy"
    plutil -lint "${RESOURCES_DIR}/PrivacyInfo.xcprivacy" >/dev/null || {
        echo "ERROR: PrivacyInfo.xcprivacy is not a valid plist"; exit 1; }
    echo "   PrivacyInfo.xcprivacy -> Contents/Resources/"
else
    echo "ERROR: PrivacyInfo.xcprivacy not found at ${PRIVACY_SRC}"
    exit 1
fi

# 4-bis. Build Contents/Resources/Credits.rtf from a SINGLE SOURCE OF TRUTH.
#
# macOS's standard About panel renders Contents/Resources/Credits.rtf verbatim,
# so that file has to be a complete, valid RTF document. It used to be a
# hand-maintained copy of the licence list that drifted out of sync with LEGAL
# and README.md, and the SAME copy shipped in both variants -- including the
# GPL-only components the mas build does not contain.
#
# It is now assembled here from three tracked plain-text inputs:
#   data/misc/Credits.rtf  the human-written preamble ONLY. It is a valid RTF on
#                          its own and its last line is "Here is the attribution
#                          for the individual emulators, ...".
#   LEGAL                  the standalone licence notice. Everything in it is in
#                          BOTH builds; it mentions no variant and no build gate.
#   LEGAL-PLUS             the addendum listing what only the plus build links
#                          (the copyleft engines + their data payloads). Repeats
#                          nothing from LEGAL.
#
#   mas  -> preamble + LEGAL                 -> CreditsAndLicences.rtf
#   plus -> preamble + LEGAL + LEGAL-PLUS    -> CreditsAndLicencesPlus.rtf
#
# Whichever one this run produces is installed as Resources/Credits.rtf, so no
# C++/Info.plist change is needed. Both intermediates are written into the BUILD
# DIR, never back into the source tree (same rule as extensions.txt above).
#
# The plain-text inputs are appended as PREFORMATTED monospace paragraphs: the
# RTF metacharacters \ { } are escaped, tabs become \tab, every non-ASCII
# codepoint becomes a \uN ? escape (so the .rtf stays 7-bit ASCII and TextEdit /
# NSDocumentController parse it identically), and each source line becomes one
# RTF paragraph. \f8 is the Menlo-Regular entry added to the preamble's font
# table -- LEGAL is column-aligned and only reads correctly in a fixed pitch.
CREDITS_PREAMBLE="${CHIPMACHINE_DIR}/data/misc/Credits.rtf"
LEGAL_FILE="${CHIPMACHINE_DIR}/LEGAL"
LEGAL_PLUS_FILE="${CHIPMACHINE_DIR}/LEGAL-PLUS"

if [ "$VARIANT" = "mas" ]; then
    CREDITS_BUILT="${BUILD_DIR}/CreditsAndLicences.rtf"
    CREDITS_PARTS=("${LEGAL_FILE}")
else
    CREDITS_BUILT="${BUILD_DIR}/CreditsAndLicencesPlus.rtf"
    CREDITS_PARTS=("${LEGAL_FILE}" "${LEGAL_PLUS_FILE}")
fi

echo "-> Generating ${CREDITS_BUILT:t} from the Credits preamble + ${CREDITS_PARTS[@]:t}..."
for CREDITS_INPUT in "${CREDITS_PREAMBLE}" "${CREDITS_PARTS[@]}"; do
    [ -f "${CREDITS_INPUT}" ] || { echo "CRITICAL: missing Credits input ${CREDITS_INPUT}"; exit 1; }
done

# The preamble minus its final closing brace -- we re-add it after the body.
perl -0777 -pe 's/\}\s*\z//' "${CREDITS_PREAMBLE}" > "${CREDITS_BUILT}"
# 10pt Menlo, no paragraph spacing, left aligned.
printf '\\pard\\pardeftab720\\sa0\\ql\\partightenfactor0\n\\f8\\b0\\i0\\fs20 \\cf2 \\\n' >> "${CREDITS_BUILT}"
for CREDITS_INPUT in "${CREDITS_PARTS[@]}"; do
    perl -CSD -ne '
        chomp;
        s/\\/\\\\/g; s/\{/\\{/g; s/\}/\\}/g;
        s/\t/\\tab /g;
        s/([^\x00-\x7f])/sprintf("\\u%d ?", ord($1))/ge;
        print $_ . "\\\n";
    ' "${CREDITS_INPUT}" >> "${CREDITS_BUILT}"
    printf '\\\n' >> "${CREDITS_BUILT}"
done
printf '}\n' >> "${CREDITS_BUILT}"

# Fail the build on a malformed document rather than shipping an About panel
# that renders raw RTF markup (or nothing at all). textutil is the same parser
# AppKit uses, and it is silent on success.
if ! textutil -convert txt -stdout "${CREDITS_BUILT}" > /dev/null 2>&1; then
    echo "CRITICAL: generated ${CREDITS_BUILT} is not a valid RTF document. Aborting."
    exit 1
fi

echo "-> Packaging Credits into bundle..."
cp "${CREDITS_BUILT}" "${RESOURCES_DIR}/Credits.rtf"

# The recursive `cp -R data` above also dropped the PREAMBLE-ONLY stub at
# Resources/data/misc/Credits.rtf. Nothing reads it, and shipping a file that
# looks like the credits but stops mid-sentence is worse than not shipping it.
rm -f "${RESOURCES_DIR}/data/misc/Credits.rtf"

if [ -d "${CHIPMACHINE_DIR}/lua" ]; then
    echo "-> Packaging Lua subsystem files into bundle..."
    cp -R "${CHIPMACHINE_DIR}/lua" "${RESOURCES_DIR}/"
else
    echo "WARNING: Lua folder not found at ${CHIPMACHINE_DIR}/lua. Scripting features may fail."
fi

# 4-ante. MAS ONLY: drop the stream-resolver hook from the COPIED init.lua.
#
# lua/init.lua defines on_parse_youtube(), which calls cm_execute() -- a binding
# onto fork()+execl("/bin/sh","-c",...) -- to run an external downloader with a
# URL interpolated into the shell string.
#
# In the mas build that function is ALREADY dead: initYoutube() is behind
# #ifndef CM_MAS in src/main.cpp, so nothing ever calls the hook, and step 4b
# below ships no helper binary for it to find. But dead is not the same as
# absent. The script is shipped as PLAIN TEXT in Contents/Resources/lua/, so an
# App Review pass that greps the bundle finds a script that shells out to a
# downloader -- which reads as a direct contradiction of guideline 2.5.2 and of
# this build's own reviewer notes, no matter that the code path is unreachable.
#
# So strip it here, on the COPY only (never chipmachine/lua/, which the plus
# build still needs verbatim). Removes the preceding comment block too, so no
# stray reference to the helper's name survives in the shipped file. The two
# real callbacks (on_layout, on_select_plugin) are untouched.
#
# If a future init.lua adds another shell-out, this will NOT catch it -- the
# guard below is what fails the build in that case.
if [ "$VARIANT" = "mas" ]; then
    INIT_LUA="${RESOURCES_DIR}/lua/init.lua"
    if [ -f "${INIT_LUA}" ]; then
        echo "-> MAS build: removing stream-resolver hook from bundled init.lua..."
        perl -0777 -i -pe 's{(?:^--[^\n]*\n)*^function\s+on_parse_youtube\b.*?^end\n}{-- The stream-resolver hook is intentionally absent from the Mac App Store\n-- build: the plugin that drove it is gated out at compile time (CM_MAS), no\n-- external helper program is bundled, and nothing in this build shells out.\n-- Removed by package_app.sh so the shipped script matches the binary.\n}ms' "${INIT_LUA}"
    fi

    # Guard: no shipped Lua may shell out in the App Store build. Catches both a
    # failed edit above and any NEW shell-out added to another script later.
    # `|| true` is REQUIRED: this script runs under `set -e`, and grep exits 1
    # when it finds nothing -- which is the PASSING case here. Without it the
    # guard aborts the build precisely when the bundle is clean.
    LUA_LEAK=$(grep -rlE 'cm_execute|yt-dlp|os\.execute|io\.popen' "${RESOURCES_DIR}/lua" 2>/dev/null || true)
    if [ -n "${LUA_LEAK}" ]; then
        echo "CRITICAL: shell-out found in bundled Lua for the mas variant:"
        echo "${LUA_LEAK}" | sed 's/^/          /'
        echo "          The App Store build must ship no script that spawns a"
        echo "          process. Gate it out here before packaging."
        exit 1
    fi
    echo "     Verified: no bundled Lua script shells out."
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

# 4a-bis. Drop the marketing screenshots from the COPIED tree (both variants).
#
# data/misc/{search,formats,amegas}.png are web-forum illustrations, not runtime
# assets -- nothing in the app or the Lua layer loads them, they just add ~2.7MB
# of dead weight to every bundle. They cannot be moved to data-notbundled/
# because external forum posts hot-link them at their current paths, so they stay
# in the source tree and are removed here, after the recursive copy.
for DANGLING_PIC in search.png formats.png amegas.png; do
    if [ -f "${RESOURCES_DIR}/data/misc/${DANGLING_PIC}" ]; then
        echo "-> Removing dangling marketing screenshot from bundle: misc/${DANGLING_PIC}"
        rm -f "${RESOURCES_DIR}/data/misc/${DANGLING_PIC}"
    fi
done

# 4b. UADE runtime payload -- plus variant only.
#
# data/uade/ holds UADE's eagleplayer.conf, uaerc, the 68k `score` binary and
# the players/ tree of Amiga replayers. All of it is GPLv2, and the mas build
# does not link uadeplugin at all (CM_HAVE_UADE=OFF in CMakeLists.txt), so
# shipping the payload would put GPL material into an App Store bundle for a
# binary that cannot even load it. Drop it from the COPIED tree; the source
# tree under chipmachine/data/ is never touched.
if [ "$VARIANT" = "mas" ]; then
    if [ -d "${RESOURCES_DIR}/data/uade" ]; then
        echo "-> Removing UADE runtime payload from mas bundle (GPL, plugin not linked)"
        rm -rf "${RESOURCES_DIR}/data/uade"
    fi
fi

# 4c. VICE runtime payload -- plus variant only.
#
# data/c64/ is VICE's system directory: the KERNAL, BASIC and chargen ROM images
# plus its keymaps and palettes. Two independent reasons it must not ship in the
# mas bundle -- the mas build does not link vicepluginbridge at all
# (CM_HAVE_VICE=OFF in CMakeLists.txt) so nothing can read it, and the three ROM
# images are Commodore-copyrighted binaries redistributed under VICE's GPL
# umbrella, which is its own App Store problem separate from the licence of the
# emulator code. SID playback in the mas build is csidplugin (Hermit's cSID),
# which synthesises the 6581/8580 from scratch and needs no ROMs whatsoever.
# Drop it from the COPIED tree; the source tree under chipmachine/data/ is never
# touched.
if [ "$VARIANT" = "mas" ]; then
    if [ -d "${RESOURCES_DIR}/data/c64" ]; then
        echo "-> Removing VICE C64 ROM payload from mas bundle (GPL + Commodore ROMs, plugin not linked)"
        rm -rf "${RESOURCES_DIR}/data/c64"
    fi
fi

# 4d. sc68 runtime payload -- plus variant only.
#
# data/sc68/ is libsc68's user directory: sc68.cfg plus Replay/, the 95 prebuilt
# 68k replay routines (.bin/.deli) that a .sc68 file names rather than embeds.
# Same reasoning as data/uade above -- they are sc68 project build outputs under
# its GPL-3, and the mas build does not link sc68plugin at all
# (CM_HAVE_SC68=OFF in CMakeLists.txt), so nothing there could read them.
#
# TWO paths, not one. Besides the directory there is a stray data/sc68.cfg
# sitting at the top level of data/ -- libsc68 writes its global config there at
# start-up (it is NOT a copy of data/sc68/sc68.cfg; the two differ, since each
# accumulates its own total-playing-time counters). A rule that only removed the
# directory left that file in the bundle. Both go.
#
# .sndh playback is NOT affected: sndhplugin drives AtariAudio (MIT), which is
# entirely self-contained -- an SNDH file carries its own 68k driver, so there is
# no replay directory and no config to ship. Drop them from the COPIED tree; the
# source tree under chipmachine/data/ is never touched.
if [ "$VARIANT" = "mas" ]; then
    if [ -d "${RESOURCES_DIR}/data/sc68" ]; then
        echo "-> Removing sc68 replay payload from mas bundle (GPL-3, plugin not linked)"
        rm -rf "${RESOURCES_DIR}/data/sc68"
    fi
    if [ -f "${RESOURCES_DIR}/data/sc68.cfg" ]; then
        echo "-> Removing stray sc68.cfg from mas bundle (GPL-3, plugin not linked)"
        rm -f "${RESOURCES_DIR}/data/sc68.cfg"
    fi
fi

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
# dlopen()s at runtime by absolute path. It goes in Contents/Frameworks/ with
# every other dylib; SunVoxPlugin::acquireEngine() looks there (it probes
# exeDir/../Frameworks, exeDir and exeDir/../Resources -- keep that list in sync
# if this path ever changes). Signed individually in step 7. The vendored copy is
# arm64-only so the step 5b architecture check passes.
SUNVOX_DYLIB_SRC="${CHIPMACHINE_DIR}/external/musicplayer/src/plugins/sunvoxplugin/sunvox_lib/sunvox.dylib"
if [ -f "${SUNVOX_DYLIB_SRC}" ]; then
    echo "-> Packaging SunVox engine (sunvox.dylib) into bundle..."
    cp "${SUNVOX_DYLIB_SRC}" "${FRAMEWORKS_DIR}/sunvox.dylib"
    chmod +w "${FRAMEWORKS_DIR}/sunvox.dylib"
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

# Clean up hardcoded local developer and Homebrew search paths from executable
echo "-> Normalizing LC_RPATH search paths..."
otool -l "${MAC_OS_DIR}/chipmachine" | grep -A 2 LC_RPATH | awk '/path/ {print $2}' | while read -r rpath; do
    if [[ "$rpath" == /Users/* || "$rpath" == /opt/homebrew/* ]]; then
        echo "    Removing local RPATH: $rpath"
        install_name_tool -delete_rpath "$rpath" "${MAC_OS_DIR}/chipmachine" 2>/dev/null || true
    fi
done
install_name_tool -add_rpath "@executable_path/../Frameworks" "${MAC_OS_DIR}/chipmachine" 2>/dev/null || true

typeset -A PROCESSED_LIBS

# How a given file inside the bundle should refer to a dylib in Contents/Frameworks:
#   * a dylib that already lives in Frameworks/  -> @loader_path/<name>
#     (its siblings sit right next to it, so this resolves with no LC_RPATH at
#     all -- more robust than @rpath, which relies on the rpath list of whoever
#     happens to be loading the chain)
#   * anything else (the main executable, stray Mach-O elsewhere in the bundle)
#     -> @rpath/<name>, resolved through the @executable_path/../Frameworks
#     LC_RPATH added to the executable above.
ref_prefix_for() {
    if [[ "$1" == "${FRAMEWORKS_DIR}/"* ]]; then
        echo "@loader_path/"
    else
        echo "@rpath/"
    fi
}

discover_and_patch() {
    local TARGET_FILE_PATH="$1"

    if ! file "$TARGET_FILE_PATH" | grep -q "Mach-O"; then
        return 0
    fi

    local SELF_BASE=$(basename "$TARGET_FILE_PATH")
    local REF_PREFIX=$(ref_prefix_for "$TARGET_FILE_PATH")

    # Three kinds of load command need rewriting:
    #   /opt/homebrew/..., /usr/local/...  -- Homebrew deps to be vendored in.
    #   @executable_path/...               -- ALREADY-relative refs baked in at
    #                                         link time. The vendored LGPL libav
    #                                         dylibs are built with
    #                                         @executable_path install names, so
    #                                         the main executable and the libav
    #                                         libs reference each other that way.
    #                                         Those point at Contents/MacOS/ and
    #                                         MUST be re-pointed at Frameworks/,
    #                                         or the app dies at launch with
    #                                         "Library not loaded". This is the
    #                                         case the naive move misses.
    otool -L "$TARGET_FILE_PATH" | grep -E '/opt/homebrew/|/usr/local/|@executable_path/' | awk '{print $1}' | while read -r RAW_LIB; do
        local LIB=$(echo "$RAW_LIB" | tr -d '[:space:]')
        [ -z "$LIB" ] && continue

        local LIB_BASE=$(basename "$LIB")

        # otool -L prints a dylib's own LC_ID_DYLIB as the first line. Skipping
        # the self-reference avoids infinite recursion (and the -id is rewritten
        # separately at the end of this function).
        [ "$LIB_BASE" = "$SELF_BASE" ] && continue

        local DEST_LIB_PATH="${FRAMEWORKS_DIR}/${LIB_BASE}"

        if [ -z "${PROCESSED_LIBS[$LIB_BASE]}" ]; then
            echo "    Isolating dependency: $LIB_BASE (Required by ${SELF_BASE})"

            if [ ! -f "$DEST_LIB_PATH" ]; then
                if [ -f "$LIB" ]; then
                    cp "$LIB" "$DEST_LIB_PATH"
                    chmod +w "$DEST_LIB_PATH"
                else
                    # A relative (@executable_path/@rpath) ref whose target was
                    # never staged into Frameworks/ -- nothing to copy from and
                    # nothing to point at. Fail loudly rather than ship a bundle
                    # that dies at launch.
                    echo "CRITICAL: ${SELF_BASE} needs ${LIB_BASE} (${LIB}) but it is not in Frameworks/"
                    echo "          and the reference is not an absolute path, so it cannot be staged."
                    exit 1
                fi
            fi

            PROCESSED_LIBS[$LIB_BASE]=1
            discover_and_patch "$DEST_LIB_PATH"
        fi

        echo "    [Patching Executable Linkage] inside ${SELF_BASE}: changing $LIB -> ${REF_PREFIX}${LIB_BASE}"
        install_name_tool -change "$LIB" "${REF_PREFIX}${LIB_BASE}" "$TARGET_FILE_PATH"
    done

    if [[ "$TARGET_FILE_PATH" == *.dylib ]]; then
        # The install name is what OTHER binaries record, so it is always the
        # @rpath form regardless of who is loading it.
        install_name_tool -id "@rpath/${SELF_BASE}" "$TARGET_FILE_PATH"
    fi
}

# --- Ship LGPL (not GPL) libav ---
# The build links Homebrew's FFmpeg, which is a GPL build (--enable-gpl + libx264/
# libx265) and incompatible with App Store terms. Overwrite the four libav dylibs
# in Frameworks/ with our vendored LGPLv3, decode-only build BEFORE discover_and_patch
# runs. That function skips copying a dylib whose file already exists (so it keeps
# these LGPL ones instead of pulling Homebrew's GPL copies via otool), then still
# rewrites their openssl/sibling refs + id off @executable_path. Sonames match
# (avcodec.62/avformat.62/avutil.60/swresample.6) so it is an ABI drop-in.
# See external/ffmpeg-lgpl/README.md for provenance + license.
FFMPEG_LGPL_DIR="${CHIPMACHINE_DIR}/external/ffmpeg-lgpl/lib"
echo "-> Substituting LGPL libav dylibs (replacing the GPL Homebrew build)..."
for L in libavcodec.62 libavformat.62 libavutil.60 libswresample.6; do
    if [ -f "${FFMPEG_LGPL_DIR}/${L}.dylib" ]; then
        cp -f "${FFMPEG_LGPL_DIR}/${L}.dylib" "${FRAMEWORKS_DIR}/${L}.dylib"
        chmod +w "${FRAMEWORKS_DIR}/${L}.dylib"
    else
        echo "CRITICAL: vendored LGPL ${L}.dylib not found at ${FFMPEG_LGPL_DIR}."
        echo "          Build it via external/ffmpeg-lgpl/build_lgpl_ffmpeg.sh."
        $RELEASE_IT && exit 1
    fi
done

# Seed the walk with the executables in MacOS/ FIRST -- that pulls the whole
# dependency graph into Frameworks/ recursively -- then sweep Frameworks/ so the
# pre-staged dylibs nothing links against (sunvox.dylib, which is dlopen()ed) are
# patched too. Globs expand once, before the loop body runs, but that is fine:
# libraries copied in during the walk are patched by the recursion itself.
for EXE in "${MAC_OS_DIR}/"*; do
    if [ -f "$EXE" ] && [ -x "$EXE" ] && [ ! -L "$EXE" ]; then
        discover_and_patch "$EXE"
    fi
done
for LIB in "${FRAMEWORKS_DIR}/"*(N); do
    if [ -f "$LIB" ] && [ ! -L "$LIB" ]; then
        discover_and_patch "$LIB"
    fi
done

echo "-> Patching compiled Python native extensions inside bundle..."
if [ -d "${RESOURCES_DIR}/data/python_runtime" ]; then
    find "${RESOURCES_DIR}/data/python_runtime" -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.bundle" \) | while read -r PYTHON_EXT; do
        discover_and_patch "$PYTHON_EXT"
    done
fi

# 5a-ter. Strip the debug map out of the main executable.
#
# The link leaves a debug map behind: one N_OSO stab per object file, each
# holding the ABSOLUTE path of that .o under the build tree. On a normal build
# that is ~1500 records and ~3000 occurrences of $HOME inside the shipped
# binary, plus ~2MB of weight that gets sealed into the code signature for no
# runtime benefit.
#
# `strip -S` removes debugging symbols ONLY -- the regular symbol table stays,
# so crash reports still symbolicate to function names. (`-x` would also drop
# local symbols and make those reports much worse; don't.)
#
# This must run AFTER all install_name_tool patching above (which rewrites the
# binary) and BEFORE codesign (stripping invalidates any existing signature).
#
# It does NOT catch __FILE__ strings from assert/LOGD macros -- those are
# ordinary string constants in __TEXT, not debug info, and no strip touches
# them. They are handled at compile time by -ffile-prefix-map in CMakeLists.txt.
# The check below is what tells you if that flag stopped working.
#
# Bundled dylibs are deliberately left alone: they carry no debug map (verify
# with `nm -pa <lib> | grep -c ' OSO '`), and sunvox.dylib is third-party
# closed-source that we do not modify.
echo "-> Stripping debug map from main executable..."
MAIN_EXE="${MAC_OS_DIR}/chipmachine"
if [ -f "${MAIN_EXE}" ]; then
    SIZE_BEFORE=$(stat -f%z "${MAIN_EXE}")
    chmod +w "${MAIN_EXE}"
    strip -S "${MAIN_EXE}" || { echo "CRITICAL: strip failed on ${MAIN_EXE}"; exit 1; }
    SIZE_AFTER=$(stat -f%z "${MAIN_EXE}")
    echo "     ${SIZE_BEFORE} -> ${SIZE_AFTER} bytes"

    # Belt-and-braces: report any developer path that survived. Counts raw byte
    # occurrences, because these strings are NUL-separated and `strings` reports
    # them inconsistently.
    LEAKED=$(python3 -c "
import sys
d = open('${MAIN_EXE}', 'rb').read()
print(d.count(b'${HOME}'))
" 2>/dev/null || echo 0)
    if [ "${LEAKED}" != "0" ]; then
        echo "WARNING: ${LEAKED} occurrence(s) of ${HOME} remain in the executable."
        echo "         Expected 0. The -ffile-prefix-map flags in CMakeLists.txt"
        echo "         are missing or the build dir predates them -- reconfigure"
        echo "         from scratch (rm -rf the build dir) and rebuild."
    else
        echo "     No developer paths remain in the executable."
    fi
else
    echo "CRITICAL: main executable not found at ${MAIN_EXE}"
    exit 1
fi

# 5b. Verify ALL bundled Mach-O binaries are pure arm64 (no Intel slices).
# Runs after dylib bundling so every copied library is covered — not just
# the helper executables that existed before step 5. Scans Contents/MacOS/ (main
# exe), Contents/Frameworks/ (every bundled dylib) and the yt-dlp tree in Resources/bin
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
done < <(find "${MAC_OS_DIR}" "${FRAMEWORKS_DIR}" "${YTDLP_DEST}" -type f 2>/dev/null)

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
    # (org.mihailod.chipmachineplus.chiptune). Info.plist references DocIcon.icns
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
    # Strip extended attributes from the whole bundle BEFORE signing, for every
    # variant and both ad-hoc and real signatures.
    #
    # `cp -R` carries source-tree xattrs into the bundle: assets under data/ pick
    # up com.apple.quarantine (downloaded fonts/images) and Finder metadata
    # (kMDItemUserTags / kMDLabel_* from coloured Finder labels). Those are
    # cosmetic in an ad-hoc build but they are real signing hazards -- FinderInfo
    # and resource-fork xattrs make codesign fail outright, and any xattr churn
    # after sealing desyncs the signature, which macOS reports to the end user as
    # "<App> is damaged and can't be opened". Clearing unconditionally keeps the
    # ad-hoc and Developer ID paths byte-comparable instead of only cleaning the
    # one we happen to ship.
    echo "-> Clearing extended attributes from bundle..."
    xattr -cr "${TARGET_DIR}"

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

    # Sign every Mach-O under MacOS/ + Frameworks/ and (plus only) the Resources
    # ytdlp tree.
    # Filter to Mach-O only — codesign rejects .py, .pyc, and other data files.
    # The yt-dlp launcher gets the helper entitlements (plus, real signing only);
    # for the mas variant YTDLP_DEST does not exist and this branch never matches.
    find "${MAC_OS_DIR}" "${FRAMEWORKS_DIR}" "${YTDLP_DEST}" -type f 2>/dev/null | while read -r mf; do
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
