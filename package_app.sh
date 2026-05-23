#!/bin/zsh
set -e

# Establish precise absolute paths independent of execution context
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHIPMACHINE_DIR="${SCRIPT_DIR}"
WORKSPACE_ROOT="$(cd "${CHIPMACHINE_DIR}/.." && pwd)"
BUILD_DIR="${WORKSPACE_ROOT}/build"
APP_NAME="ChipMachineAS.app"
TARGET_DIR="${WORKSPACE_ROOT}/${APP_NAME}"
ICON_PATH="${CHIPMACHINE_DIR}/icon.png"

# Target payload directories
MAC_OS_DIR="${TARGET_DIR}/Contents/MacOS"
RESOURCES_DIR="${TARGET_DIR}/Contents/Resources"

echo "=== Starting Apple Silicon App Bundle Packaging ==="
echo "Workspace Root: ${WORKSPACE_ROOT}"
echo "Target App Bundle: ${TARGET_DIR}"

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

# 3. Create Info.plist (Enforces the display name 'ChipMachineAS')
echo "-> Creating Info.plist..."
printf '<?xml version="1.0" encoding="UTF-8"?>\n<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n<plist version="1.0">\n<dict>\n    <key>CFBundleExecutable</key>\n    <string>chipmachine</string>\n    <key>CFBundleIconFile</key>\n    <string>AppIcon.icns</string>\n    <key>CFBundleIdentifier</key>\n    <string>org.mihailod.chipmachineas</string>\n    <key>CFBundleName</key>\n    <string>ChipMachineAS</string>\n    <key>CFBundleDisplayName</key>\n    <string>ChipMachineAS</string>\n    <key>CFBundlePackageType</key>\n    <string>APPL</string>\n    <key>CFBundleShortVersionString</key>\n    <string>1.0.0</string>\n    <key>LSMinimumSystemVersion</key>\n    <string>11.0</string>\n    <key>NSHighResolutionCapable</key>\n    <true/>\n</dict>\n</plist>\n' > "${TARGET_DIR}/Contents/Info.plist"

# 4. Copy the asset and Lua payloads from the chipmachine source tree
echo "-> Packaging runtime assets into bundle..."
if [ -d "${CHIPMACHINE_DIR}/data" ]; then
    cp -R "${CHIPMACHINE_DIR}/data" "${RESOURCES_DIR}/"
else
    echo "ERROR: Data folder not found at ${CHIPMACHINE_DIR}/data"
    exit 1
fi

if [ -d "${CHIPMACHINE_DIR}/lua" ]; then
    echo "-> Packaging Lua subsystem files into bundle..."
    cp -R "${CHIPMACHINE_DIR}/lua" "${RESOURCES_DIR}/"
else
    echo "WARNING: Lua folder not found at ${CHIPMACHINE_DIR}/lua. Scripting features may fail."
fi

if [ -d "${CHIPMACHINE_DIR}/bin" ]; then
    echo "-> Packaging helper binaries into bundle..."
    # 1. Put binary ffmpeg in MacOS so it shares the library pathing
    cp -L "${CHIPMACHINE_DIR}/bin/ffmpeg" "${MAC_OS_DIR}/"
    
    # 2. Put scripts/links in Resources/bin to avoid codesign issues in MacOS/
    mkdir -p "${RESOURCES_DIR}/bin"
    cp -L "${CHIPMACHINE_DIR}/bin/yt-dlp" "${RESOURCES_DIR}/bin/"
    cp -L "${CHIPMACHINE_DIR}/bin/youtube-dl" "${RESOURCES_DIR}/bin/"
    
    chmod +x "${MAC_OS_DIR}/ffmpeg"
    chmod +x "${RESOURCES_DIR}/bin/"*
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
            echo "   Isolating dependency: $LIB_BASE (Required by $(basename "$TARGET_FILE_PATH"))"
            
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
        echo "   [Patching Executable Linkage] inside $(basename "$TARGET_FILE_PATH"): changing $LIB -> @executable_path/$LIB_BASE"
        install_name_tool -change "$LIB" "@executable_path/$LIB_BASE" "$TARGET_FILE_PATH"
    done
    
    if [[ "$TARGET_FILE_PATH" == *.dylib ]]; then
        install_name_tool -id "@executable_path/$(basename "$TARGET_FILE_PATH")" "$TARGET_FILE_PATH"
    fi
}

# Run dependency injection pass for Mach-O binaries in the MacOS folder
discover_and_patch "${MAC_OS_DIR}/chipmachine"
discover_and_patch "${MAC_OS_DIR}/ffmpeg"

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
    # Sign all dylibs and binaries in the MacOS folder
    for f in "${MAC_OS_DIR}/"*; do
        if [ -f "$f" ]; then
            codesign -f -s - "$f"
        fi
    done
    codesign -f -s - "${TARGET_DIR}"
    echo "-> Code signing complete."
fi

echo "=== Success: ${APP_NAME} generated cleanly in workspace root! ==="
