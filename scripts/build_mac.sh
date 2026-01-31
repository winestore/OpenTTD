#!/bin/bash
#
# Transport Empire - Mac Build Script
# This script builds OpenTTD with Transport Empire modifications on macOS
#

set -e

echo "======================================"
echo "Transport Empire - Mac Build"
echo "======================================"
echo ""

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo "ERROR: Homebrew is required. Install from https://brew.sh"
    echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    exit 1
fi

# Install dependencies
echo "[1/5] Checking dependencies..."
DEPS="cmake sdl2 libpng lzo xz fluidsynth fontconfig freetype harfbuzz icu4c libogg libvorbis opus opusfile"

for dep in $DEPS; do
    if ! brew list $dep &>/dev/null; then
        echo "  Installing $dep..."
        brew install $dep
    else
        echo "  ✓ $dep"
    fi
done

# Get the script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Create build directory
echo ""
echo "[2/5] Configuring build..."
mkdir -p build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" \
    -DCMAKE_PREFIX_PATH="$(brew --prefix)" \
    -DICU_ROOT="$(brew --prefix icu4c)"

# Build
echo ""
echo "[3/5] Building Transport Empire..."
CORES=$(sysctl -n hw.ncpu)
make -j$CORES

# Check if build succeeded
if [ -f "openttd" ]; then
    echo ""
    echo "[4/5] Build successful!"
else
    echo ""
    echo "ERROR: Build failed!"
    exit 1
fi

# Create app bundle info
echo ""
echo "[5/5] Creating Transport Empire.app..."

APP_DIR="Transport Empire.app"
rm -rf "$APP_DIR"
mkdir -p "$APP_DIR/Contents/MacOS"
mkdir -p "$APP_DIR/Contents/Resources"

# Copy binary
cp openttd "$APP_DIR/Contents/MacOS/Transport Empire"

# Create Info.plist
cat > "$APP_DIR/Contents/Info.plist" << 'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>Transport Empire</string>
    <key>CFBundleIdentifier</key>
    <string>com.transportempire.game</string>
    <key>CFBundleName</key>
    <string>Transport Empire</string>
    <key>CFBundleDisplayName</key>
    <string>Transport Empire</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
</dict>
</plist>
PLIST

echo ""
echo "======================================"
echo "BUILD COMPLETE!"
echo "======================================"
echo ""
echo "To run Transport Empire:"
echo "  1. From terminal: ./build/openttd"
echo "  2. Or double-click: build/Transport Empire.app"
echo ""
echo "To install to Applications:"
echo "  cp -r \"build/Transport Empire.app\" /Applications/"
echo ""
echo "======================================"
echo "TRANSPORT EMPIRE FEATURES:"
echo "======================================"
echo "  ✓ Stock Market System"
echo "  ✓ Hostile Takeovers"
echo "  ✓ AI Personalities (Gordon Gekko, Warren Buffett, etc.)"
echo "  ✓ Dynamic Weather (Rain, Snow, Fog, Storms)"
echo "  ✓ Day/Night Cycle"
echo "  ✓ Company Reputation System"
echo "  ✓ Market Manipulation & Investigations"
echo ""
