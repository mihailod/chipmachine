#!/bin/zsh
set -e

# Establish paths relative to this script's physical location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHIPMACHINE_DIR="${SCRIPT_DIR}"
WORKSPACE_ROOT="$(cd "${CHIPMACHINE_DIR}/.." && pwd)"
BUILD_DIR="${WORKSPACE_ROOT}/build"
APP_NAME="ChipMachineAS.app"
TARGET_DIR="${WORKSPACE_ROOT}/${APP_NAME}"
ICON_PATH="${CHIPMACHINE_DIR}/icon.png"

echo "=== Starting Apple Silicon App Bundle Packaging ==="
echo "Workspace Root: ${WORKSPACE_ROOT}"
echo "Target App Bundle: ${TARGET_DIR}"

# 1. Clean previous packaging attempts and set up pristine directories
rm -rf "${TARGET_DIR}"
mkdir -p "${TARGET_DIR}/Contents/MacOS"
mkdir -p "${TARGET_DIR}/Contents/Resources"

# 2. Copy compiled binary as the primary bundle entry point
if [ ! -f "${BUILD_DIR}/chipmachine" ]; then
    echo "CRITICAL ERROR: Compiled binary not found at ${BUILD_DIR}/chipmachine!"
    exit 1
fi
echo "-> Copying executable binary..."
cp "${BUILD_DIR}/chipmachine" "${TARGET_DIR}/Contents/MacOS/chipmachine"
chmod +x "${TARGET_DIR}/Contents/MacOS/chipmachine"

# 3. Create Info.plist (Enforces the display name 'ChipMachineAS')
echo "-> Creating Info.plist..."
cat <<EOF > "${TARGET_DIR}/Contents/Info.plist"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>chipmachine</string>
    <key>CFBundleIconFile</key>
    <string>AppIcon.icns</string>
    <key>CFBundleIdentifier</key>
    <string>org.mihailod.chipmachineas</string>
    <key>CFBundleName</key>
    <string>ChipMachineAS</string>
    <key>CFBundleDisplayName</key>
    <string>ChipMachineAS</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

# 4. Copy the asset and Lua payloads from the chipmachine source tree
echo "-> Packaging runtime assets into bundle..."
if [ -d "${CHIPMACHINE_DIR}/data" ]; then
    cp -R "${CHIPMACHINE_DIR}/data" "${TARGET_DIR}/Contents/Resources/"
else
    echo "ERROR: Data folder not found at ${CHIPMACHINE_DIR}/data"
    exit 1
fi

if [ -d "${CHIPMACHINE_DIR}/lua" ]; then
    echo "-> Packaging Lua subsystem files into bundle..."
    cp -R "${CHIPMACHINE_DIR}/lua" "${TARGET_DIR}/Contents/Resources/"
else
    echo "WARNING: Lua folder not found at ${CHIPMACHINE_DIR}/lua. Scripting features may fail."
fi

# 5. Fix Native ARM64 Dynamic Library Linkages Deeply
echo "-> Resolving recursive dynamic library paths..."
cd "${TARGET_DIR}/Contents/MacOS"

# Use an associative array to track processed libraries natively in Zsh
typeset -A PROCESSED_LIBS

discover_and_patch() {
    local TARGET_FILE="$1"
    
    # Read otool output line by line safely using a while loop
    otool -L "$TARGET_FILE" | grep -E '/opt/homebrew/|/usr/local/' | awk '{print $1}' | while read -r LIB; do
        [ -z "$LIB" ] && continue
        local LIB_BASE=$(basename "$LIB")
        
        if [ -z "${PROCESSED_LIBS[$LIB_BASE]}" ]; then
            echo "   Isolating dependency: $LIB_BASE (Required by $(basename "$TARGET_FILE"))"
            
            if [ ! -f "$LIB_BASE" ]; then
                cp "$LIB" "."
                chmod +w "$LIB_BASE"
            fi
            
            PROCESSED_LIBS[$LIB_BASE]=1
            
            # Recurse down to capture nested dependencies inside the copied library
            discover_and_patch "$LIB_BASE"
        fi
        
        # Redirect the reference to point inside the bundle relatively
        install_name_tool -change "$LIB" "@executable_path/$LIB_BASE" "$TARGET_FILE"
    done
    
    if [[ "$TARGET_FILE" == *.dylib ]]; then
        install_name_tool -id "@executable_path/$TARGET_FILE" "$TARGET_FILE"
    fi
}

# Run deep dependency sweep starting from your main binary entry point
discover_and_patch "chipmachine"

# 6. Build the Icons
cd "${CHIPMACHINE_DIR}"
if [ -f "${ICON_PATH}" ]; then
    echo "-> Compiling application icon from local icon.png..."
    mkdir -p temp.iconset
    sips -z 16 16     "${ICON_PATH}" --out temp.iconset/icon_16x16.png
    sips -z 32 32     "${ICON_PATH}" --out temp.iconset/icon_16x16@2x.png
    sips -z 32 32     "${ICON_PATH}" --out temp.iconset/icon_32x32.png
    sips -z 64 64     "${ICON_PATH}" --out temp.iconset/icon_32x32@2x.png
    sips -z 128 128   "${ICON_PATH}" --out temp.iconset/icon_128x128.png
    sips -z 256 256   "${ICON_PATH}" --out temp.iconset/icon_128x128@2x.png
    sips -z 256 256   "${ICON_PATH}" --out temp.iconset/icon_256x256.png
    sips -z 512 512   "${ICON_PATH}" --out temp.iconset/icon_256x256@2x.png
    sips -z 512 512   "${ICON_PATH}" --out temp.iconset/icon_512x512.png
    sips -z 1024 1024 "${ICON_PATH}" --out temp.iconset/icon_512x512@2x.png
    iconutil -c icns temp.iconset -o "${TARGET_DIR}/Contents/Resources/AppIcon.icns"
    rm -rf temp.iconset
fi

# 7. Apply ad-hoc code signatures
echo "-> Applying ad-hoc code signatures..."
if command -v codesign &> /dev/null; then
    find "${TARGET_DIR}/Contents/MacOS" -name "*.dylib" -exec codesign -f -s - {} \;
    codesign -f -s - "${TARGET_DIR}/Contents/MacOS/chipmachine"
    codesign -f -s - "${TARGET_DIR}"
    echo "-> Code signing complete."
fi

echo "=== Success: ${APP_NAME} generated cleanly in workspace root! ==="
