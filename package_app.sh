#!/bin/zsh
set -e

# Establish precise absolute paths independent of execution context
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
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

# Extract the version inside the quotes from #define VERSION_STR "X.Y.Z"
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

# Inside Section 4 of package_app.sh:
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

if [ -d "${CHIPMACHINE_DIR}/bin" ]; then
    echo "-> Packaging helper binaries into bundle..."
    # Put all binaries in MacOS so they share the library pathing and codesigning context
    cp -L "${CHIPMACHINE_DIR}/bin/ffmpeg" "${MAC_OS_DIR}/"
    cp -L "${CHIPMACHINE_DIR}/bin/yt-dlp" "${MAC_OS_DIR}/"
    cp -P "${CHIPMACHINE_DIR}/bin/youtube-dl" "${MAC_OS_DIR}/"
    
    chmod +x "${MAC_OS_DIR}/ffmpeg"
    chmod +x "${MAC_OS_DIR}/yt-dlp"
else
    echo "WARNING: bin folder not found at ${CHIPMACHINE_DIR}/bin. YouTube playback will fail."
fi

# 5. Fix Native ARM64 Dynamic Library Linkages Deeply
echo "-> Resolving recursive dynamic library paths..."

# Global associative array for dependency state tracking
typeset -A PROCESSED_LIBS

discover_and_patch() {
    local TARGET_FILE_PATH="$1"
    
    # Check if the file is a Mach-O binary before attempting to patch it
    if ! file "$TARGET_FILE_PATH" | grep -q "Mach-O"; then
        return 0
    fi

    # Process line-by-line while cleanly splitting out whitespaces and the trailing metadata parentheses
    otool -L "$TARGET_FILE_PATH" | grep -E '/opt/homebrew/|/usr/local/' | awk '{print $1}' | while read -r RAW_LIB; do
        # Robustly strip any hidden tabs, trailing carriage returns, or spaces
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
            
            # Register globally
            PROCESSED_LIBS[$LIB_BASE]=1
            
            # Safe depth-first recursion
            discover_and_patch "$DEST_LIB_PATH"
        fi
        
        # Rewrite absolute dependency paths to relative bundle references explicitly
        echo "    [Patching Executable Linkage] inside $(basename "$TARGET_FILE_PATH"): changing $LIB -> @executable_path/$LIB_BASE"
        install_name_tool -change "$LIB" "@executable_path/$LIB_BASE" "$TARGET_FILE_PATH"
    done
    
    if [[ "$TARGET_FILE_PATH" == *.dylib ]]; then
        install_name_tool -id "@executable_path/$(basename "$TARGET_FILE_PATH")" "$TARGET_FILE_PATH"
    fi
}

# Run dependency injection pass for primary Mach-O binaries in the MacOS folder
for EXE in "${MAC_OS_DIR}/"*; do
    if [ -x "$EXE" ] && [ ! -L "$EXE" ]; then
        discover_and_patch "$EXE"
    fi
done

# Run deep dependency pass explicitly on any Python binary modules/extension libraries inside data directory
echo "-> Patching compiled Python native extensions inside bundle..."
find "${RESOURCES_DIR}/data/python_runtime" -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.bundle" \) | while read -r PYTHON_EXT; do
    discover_and_patch "$PYTHON_EXT"
done

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
echo "-> Applying ad-hoc code signatures..."
if command -v codesign &> /dev/null; then
    # Target and sign Python binary extensions first (deepest level)
    find "${RESOURCES_DIR}/data/python_runtime" -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.bundle" \) | while read -r py_ext; do
        codesign -f -s - "$py_ext"
    done

    # Sign all dylibs and binaries in the MacOS folder next
    for f in "${MAC_OS_DIR}/"*; do
        if [ -f "$f" ] && [ ! -L "$f" ]; then
            codesign -f -s - "$f"
        fi
    done
    
    # Sign main bundle last
    codesign -f -s - "${TARGET_DIR}"
    echo "-> Code signing complete."
fi

echo "=== Success: ${APP_NAME} generated cleanly in workspace root! ==="
echo "=== Making the final distribution package...==="

# Change context into the workspace directory so zip handles local filenames exclusively
cd "${WORKSPACE_ROOT}"
zip -r -y ./ChipMachineAS.zip ./${APP_NAME}
cd "${CHIPMACHINE_DIR}"

echo "=== Done!==="
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
    # Check if gh CLI is missing before attempting interaction
    if ! command -v gh &> /dev/null; then
        echo "ERROR: 'gh' command line tool not found in PATH. Skipping automated execution."
        exit 1
    fi

    # Prompt user on standard error channel to preserve any output streaming architectures
    printf "Provide release notes and confirm the official release upload to GitHub per command above [Y/N] ? " >&2
    read -r RESPONSE

    if [[ "$RESPONSE" == "y" || "$RESPONSE" == "Y" ]]; then
        # Capture the specific note payload requested
        printf "Release short note (CTRL+C to abort): " >&2
        read -r SHORT_NOTE
        
        # Build pristine string appending note with precise spacing constraints
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
