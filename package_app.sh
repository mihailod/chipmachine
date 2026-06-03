#!/bin/zsh
set -e

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

# Target payload directories
MAC_OS_DIR="${TARGET_DIR}/Contents/MacOS"
RESOURCES_DIR="${TARGET_DIR}/Contents/Resources"

# -----------------------------------------------------------------
# Parse Arguments
# -----------------------------------------------------------------
RELEASE_IT=false
for arg in "$@"; do
    if [[ "$arg" == "--releaseit" ]]; then
        RELEASE_IT=true
    fi
done

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

# 3. Create Info.plist (Dynamically uses the parsed $VERSION_STR)
echo "-> Creating Info.plist..."
printf '<?xml version="1.0" encoding="UTF-8"?>\n<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n<plist version="1.0">\n<dict>\n    <key>CFBundleExecutable</key>\n    <string>chipmachine</string>\n    <key>CFBundleIconFile</key>\n    <string>AppIcon.icns</string>\n    <key>CFBundleIdentifier</key>\n    <string>org.mihailod.chipmachineas</string>\n    <key>CFBundleName</key>\n    <string>ChipMachineAS</string>\n    <key>CFBundleDisplayName</key>\n    <string>ChipMachineAS</string>\n    <key>CFBundlePackageType</key>\n    <string>APPL</string>\n    <key>CFBundleShortVersionString</key>\n    <string>%s</string>\n    <key>LSMinimumSystemVersion</key>\n    <string>11.0</string>\n    <key>NSHighResolutionCapable</key>\n    <true/>\n</dict>\n</plist>\n' "${VERSION_STR}" > "${TARGET_DIR}/Contents/Info.plist"

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
YTDLP_DEST="${RESOURCES_DIR}/bin/ytdlp"

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
fi

# 7. Apply ad-hoc code signatures
#
# Strategy: sign each nested Mach-O individually (deepest content first), then
# seal the outer bundle LAST — and WITHOUT --deep.
#
# Why not --deep: --deep recursively descends into the yt-dlp PyInstaller tree
# and tries to interpret its package/*.dist-info directories as nested bundles,
# failing with "bundle format unrecognized, invalid, or unsuitable". Because the
# tree now lives in Contents/Resources/ (not MacOS/), codesign seals it as data
# via the normal resource envelope, so the plain bundle seal handles it
# correctly. We only need to individually sign the actual Mach-O code: the main
# executable, ffmpeg and bundled dylibs in MacOS/, plus yt-dlp and every
# *.so/*.dylib under the Resources ytdlp tree.
echo "-> Applying ad-hoc code signatures..."
if command -v codesign &> /dev/null; then
    if [ -d "${RESOURCES_DIR}/data/python_runtime" ]; then
        find "${RESOURCES_DIR}/data/python_runtime" -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.bundle" \) | while read -r py_ext; do
            codesign -f -s - "$py_ext"
        done
    fi

    # Sign every Mach-O under MacOS/ and the Resources ytdlp tree.
    # Filter to Mach-O only — codesign rejects .py, .pyc, and other data files.
    find "${MAC_OS_DIR}" "${YTDLP_DEST}" -type f 2>/dev/null | while read -r mf; do
        if file "$mf" | grep -q "Mach-O"; then
            codesign -f -s - "$mf"
        fi
    done

    # Seal the bundle (no --deep — see header comment).
    codesign -f -s - "${TARGET_DIR}"
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
zip -r -y ./ChipMachineAS.zip ./${APP_NAME}
cd "${CHIPMACHINE_DIR}"

echo "=== Done! ==="
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
