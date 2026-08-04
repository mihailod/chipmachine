#!/bin/bash
# gen_info_plist.sh -- emit the complete Info.plist for the ChipMachineAS .app,
# including the macOS file-association document types, to stdout.
#
# This is the SINGLE source of the Info.plist. Both the real packaging step
# (package_app.sh) and the fast no-recompile test loop (dev_update_doctypes.sh)
# call it, so the two can never drift.
#
# The document-type model (see the two .txt list files for the full rationale):
#
#   * Umbrella exported UTI  org.mihailod.chipmachineplus.chiptune
#       Claims every playable extension that is NOT a system type and NOT on the
#       deny list. We export it with the ChipMachine document icon; because
#       nothing else on the system opens these obscure formats, files get our
#       icon and open in ChipMachine. Registered at LSHandlerRank=Alternate so
#       we never plant a hard "Owner" claim.
#
#   * System-type references (mp3/wav/flac/...) -> one CFBundleDocumentTypes
#       entry that points LSItemContentTypes at the EXISTING system UTIs at
#       LSHandlerRank=Alternate. We appear in "Open With" but never redefine the
#       type or its icon, and never become default unless the user says so.
#
#   * Deny list -> dropped entirely (UADE modland prefix tokens; js/md/dat).
#
# Usage:
#   gen_info_plist.sh --version X.Y.Z \
#       [--exts FILE] [--denylist FILE] [--systemtypes FILE]
#
# Defaults resolve to the sibling .txt files next to this script.
set -eu

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

VERSION=""
EXTS_FILE="${SCRIPT_DIR}/extensions.txt"
DENY_FILE="${SCRIPT_DIR}/MacOSHandlerDenyList.txt"
SYS_FILE="${SCRIPT_DIR}/MacOSSystemTypeExtensions.txt"

# Bundle identity. Defaults match the historical build; package_app.sh overrides
# them per variant from variants.conf (--bundle-id / --display-name). UMBRELLA_UTI
# defaults to "<bundle-id>.chiptune" unless given explicitly.
BUNDLE_ID="org.mihailod.chipmachineplus"
DISPLAY_NAME="ChipMachineAS"
UMBRELLA_UTI=""
APP_CATEGORY=""      # LSApplicationCategoryType; emitted only when non-empty (MAS)
# LSMinimumSystemVersion. This MUST match the deployment target actually baked
# into the built binary (LC_BUILD_VERSION / minos), so package_app.sh reads it
# off the binary and passes --min-os. It used to be hardcoded to "11.0" while
# CMakeLists.txt derived the real target from the BUILD HOST's macOS version --
# so the plist advertised 11.0 while the binary required 26.5, letting users on
# older systems install an app that could not launch. Never hardcode this again.
# The fallback below is only for callers that pass no --min-os (dev_update_
# doctypes.sh, which rewrites doc types in an ALREADY-BUILT bundle).
MIN_OS="11.0"
APP_ICON="AppIcon.icns"
DOC_ICON="DocIcon.icns"
DEVELOPMENT_REGION="en"
COPYRIGHT="Copyright © 2026 Mihailo Despotovic. All rights reserved."

while [ $# -gt 0 ]; do
    case "$1" in
        --version)       VERSION="$2"; shift 2 ;;
        --exts)          EXTS_FILE="$2"; shift 2 ;;
        --denylist)      DENY_FILE="$2"; shift 2 ;;
        --systemtypes)   SYS_FILE="$2"; shift 2 ;;
        --bundle-id)     BUNDLE_ID="$2"; shift 2 ;;
        --display-name)  DISPLAY_NAME="$2"; shift 2 ;;
        --umbrella-uti)  UMBRELLA_UTI="$2"; shift 2 ;;
        --app-category)  APP_CATEGORY="$2"; shift 2 ;;
        --min-os)        MIN_OS="$2"; shift 2 ;;
        --copyright)     COPYRIGHT="$2"; shift 2 ;;
        --dev-region)    DEVELOPMENT_REGION="$2"; shift 2 ;;
        *) echo "gen_info_plist.sh: unknown arg '$1'" >&2; exit 2 ;;
    esac
done

# Derive the umbrella UTI from the bundle id if not given explicitly, so a
# variant's exported type never collides with another variant's.
[ -n "$UMBRELLA_UTI" ] || UMBRELLA_UTI="${BUNDLE_ID}.chiptune"

if [ -z "$VERSION" ]; then
    echo "gen_info_plist.sh: --version is required" >&2
    exit 2
fi
if [ ! -f "$EXTS_FILE" ]; then
    echo "gen_info_plist.sh: extensions file not found: $EXTS_FILE" >&2
    exit 1
fi

# --- normalise the input lists ---------------------------------------------
# strip_comments: drop '#' comments and blank lines.
strip_comments() { sed 's/#.*//' "$1" | grep -vE '^[[:space:]]*$' || true; }

# Full playable set: lowercase, strip a leading dot, keep only tokens that are
# valid macOS filename-extension tags ([a-z0-9]+). That alnum filter alone
# already discards every special-char UADE prefix token; the deny list below
# additionally removes the alnum-but-dangerous ones (js/md/dat).
ALL=$(strip_comments "$EXTS_FILE" \
        | awk '{print tolower($1)}' | sed 's/^\.//' \
        | grep -E '^[a-z0-9]+$' | sort -u)

# Deny set (first column only, normalised the same way).
DENY=""
[ -f "$DENY_FILE" ] && DENY=$(strip_comments "$DENY_FILE" \
        | awk '{print tolower($1)}' | sed 's/^\.//' | sort -u)

# System-type pairs "ext uti", restricted to formats we actually play (ext in
# ALL) so we never advertise a system type no plugin can handle.
SYS_PAIRS=""
if [ -f "$SYS_FILE" ]; then
    SYS_PAIRS=$(strip_comments "$SYS_FILE" \
        | awk 'NF>=2 {e=tolower($1); sub(/^\./,"",e); print e" "$2}' \
        | while read -r e u; do
              printf '%s\n' "$ALL" | grep -qxF "$e" && echo "$e $u"
          done)
fi
SYS_EXTS=$(printf '%s\n' "$SYS_PAIRS" | awk 'NF>=1{print $1}' | sort -u)
SYS_UTIS=$(printf '%s\n' "$SYS_PAIRS" | awk 'NF>=2{print $2}' | sort -u | grep -vE '^$' || true)

# Umbrella set = ALL - DENY - SYS_EXTS
EXCLUDE=$(printf '%s\n%s\n' "$DENY" "$SYS_EXTS" | grep -vE '^$' | sort -u)
if [ -n "$EXCLUDE" ]; then
    UMBRELLA=$(comm -23 <(printf '%s\n' "$ALL") <(printf '%s\n' "$EXCLUDE"))
else
    UMBRELLA="$ALL"
fi

# --- emit the plist ---------------------------------------------------------
emit_strings() { while read -r x; do [ -n "$x" ] && printf '                <string>%s</string>\n' "$x"; done; }

# LSApplicationCategoryType is required for the Mac App Store; emit the key only
# when a category was supplied (the plus/Developer ID build passes none, keeping
# its plist unchanged). Built as a standalone block to avoid nested ${} parsing.
CATEGORY_BLOCK=""
if [ -n "$APP_CATEGORY" ]; then
    CATEGORY_BLOCK=$(printf '\n    <key>LSApplicationCategoryType</key>\n    <string>%s</string>' "$APP_CATEGORY")
fi

cat <<PLIST_HEAD
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>${DEVELOPMENT_REGION}</string>
    <key>CFBundleExecutable</key>
    <string>chipmachine</string>
    <key>CFBundleIconFile</key>
    <string>${APP_ICON}</string>
    <key>CFBundleIdentifier</key>
    <string>${BUNDLE_ID}</string>
    <key>CFBundleName</key>
    <string>${DISPLAY_NAME}</string>
    <key>CFBundleDisplayName</key>
    <string>${DISPLAY_NAME}</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>${CATEGORY_BLOCK}
    <key>CFBundleShortVersionString</key>
    <string>${VERSION}</string>
    <key>CFBundleVersion</key>
    <string>${VERSION}</string>
    <key>LSMinimumSystemVersion</key>
    <string>${MIN_OS}</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSHumanReadableCopyright</key>
    <string>${COPYRIGHT}</string>

    <!-- Export compliance. Without this key App Store Connect stops every
         upload on the encryption questionnaire, so it is asserted here rather
         than answered by hand per submission.

         false = "uses no non-exempt encryption". The bundle does ship OpenSSL
         (libssl.3/libcrypto.3 in Contents/Frameworks, pulled in by the libav
         stack -- otool -L on Contents/Frameworks/libavformat.62.dylib shows
         both), but it is used ONLY for standard HTTPS transport when fetching
         tunes from public archives. That falls under the exemption for encryption limited
         to authentication/transport via standard protocols. The app implements
         no encryption of its own, ships no proprietary or non-standard crypto,
         and does not encrypt user content at rest.

         Re-evaluate this if the app ever encrypts stored data, adds its own
         crypto, or ships a non-standard protocol -- it is a legal declaration,
         not a build flag. Emitted for BOTH variants: mas needs it for App Store
         ingestion, and plus carries it harmlessly (Gatekeeper ignores it). -->
    <key>ITSAppUsesNonExemptEncryption</key>
    <false/>

    <key>UTExportedTypeDeclarations</key>
    <array>
        <dict>
            <key>UTTypeIdentifier</key>
            <string>${UMBRELLA_UTI}</string>
            <key>UTTypeDescription</key>
            <string>Chiptune / tracker music</string>
            <key>UTTypeConformsTo</key>
            <array>
                <string>public.audio</string>
            </array>
            <key>UTTypeTagSpecification</key>
            <dict>
                <key>public.filename-extension</key>
                <array>
PLIST_HEAD

printf '%s\n' "$UMBRELLA" | emit_strings

cat <<PLIST_MID
                </array>
            </dict>
        </dict>
    </array>

    <key>CFBundleDocumentTypes</key>
    <array>
        <dict>
            <key>CFBundleTypeName</key>
            <string>Chiptune / tracker music</string>
            <key>CFBundleTypeRole</key>
            <string>Viewer</string>
            <key>LSHandlerRank</key>
            <string>Alternate</string>
            <key>CFBundleTypeIconFile</key>
            <string>${DOC_ICON}</string>
            <key>LSItemContentTypes</key>
            <array>
                <string>${UMBRELLA_UTI}</string>
            </array>
        </dict>
        <dict>
            <key>CFBundleTypeName</key>
            <string>Chiptune / tracker music (by extension)</string>
            <key>CFBundleTypeRole</key>
            <string>Viewer</string>
            <key>LSHandlerRank</key>
            <string>Alternate</string>
            <key>CFBundleTypeExtensions</key>
            <array>
PLIST_MID

# SEPARATE document-type entry, extensions only, NO LSItemContentTypes.
#
# This is what reliably advertises "ChipMachine opens this extension" in Finder's
# "Open With" menu regardless of which foreign UTI a file resolves to (e.g. VLC
# owns org.videolan.mod for .mod). Two hard-won facts from testing on macOS 26:
#   1. If an entry carries LSItemContentTypes, macOS HONORS ONLY the UTI and
#      ignores any CFBundleTypeExtensions in the SAME entry -- so the extension
#      list must live in its own entry (this one), split from the UTI entry
#      above that drives typing/icon on uncontested formats.
#   2. LaunchServices only offers apps from a standard location (/Applications,
#      ~/Applications) in the Open With menu -- an app run from a build/temp path
#      will NOT appear no matter how it is declared. Install to /Applications.
# It is a handler declaration, not a type export, so it never hijacks a file's
# icon or Kind. Documentation-legacy but fully functional (ProTracker, FastTracker,
# VLC all ship it) and, being a plist key, emits no compile-time deprecation.
printf '%s\n' "$UMBRELLA" | emit_strings

cat <<'PLIST_MID2'
            </array>
        </dict>
PLIST_MID2

# System-type reference entry (only if we actually have any matching system UTI).
if [ -n "$SYS_UTIS" ]; then
cat <<'PLIST_SYS_HEAD'
        <dict>
            <key>CFBundleTypeName</key>
            <string>Audio file</string>
            <key>CFBundleTypeRole</key>
            <string>Viewer</string>
            <key>LSHandlerRank</key>
            <string>Alternate</string>
            <key>LSItemContentTypes</key>
            <array>
PLIST_SYS_HEAD
printf '%s\n' "$SYS_UTIS" | emit_strings
cat <<'PLIST_SYS_TAIL'
            </array>
        </dict>
PLIST_SYS_TAIL
fi

cat <<'PLIST_TAIL'
    </array>
</dict>
</plist>
PLIST_TAIL
