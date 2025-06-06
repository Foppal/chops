#!/bin/bash

# ChopsBrowser Clean Build Script with Auto-Installation

echo "=== ChopsBrowser Clean Build Script ==="
echo

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Remove build directory
if [ -d "build" ]; then
    echo "Removing existing build directory..."
    rm -rf build
fi

# Create fresh build directory
echo "Creating fresh build directory..."
mkdir build
cd build

# Check for SQLite3
echo "Checking for SQLite3..."
if ! pkg-config --exists sqlite3; then
    echo "Warning: SQLite3 not found via pkg-config"
    echo "On macOS, SQLite3 should be available system-wide"
    echo "On Linux, you may need to install: libsqlite3-dev"
fi

# Configure with CMake
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
    -DCMAKE_CXX_STANDARD=17

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    echo
    echo "=== Troubleshooting ==="
    echo "1. Make sure JUCE is properly cloned in the JUCE subdirectory"
    echo "2. Check that all source files are in the correct locations"
    echo "3. Ensure you have Xcode command line tools installed"
    echo "4. Try running 'xcode-select --install' if you get compiler errors"
    echo "5. For SQLite issues on Linux: sudo apt-get install libsqlite3-dev"
    echo "6. For SQLite issues on macOS: brew install sqlite3"
    echo "7. Make sure the Plugin directory exists with CMakeLists.txt"
    exit 1
fi

# Build
echo
echo "Building..."
make -j8

if [ $? -ne 0 ]; then
    echo "Build failed!"
    echo
    echo "=== Troubleshooting ==="
    echo "1. Check compiler errors above for missing headers or libraries"
    echo "2. Ensure SQLite3 is properly installed and linked"
    echo "3. Try building a single target: make ChopsBrowserPlugin"
    echo "4. Check CMake configuration warnings"
    echo "5. Make sure all source files (PluginProcessor.cpp, PluginEditor.cpp, UIBridge.cpp) exist in Plugin/"
    exit 1
fi

echo
echo "Build completed successfully!"
echo

# List the built products
echo "Built products:"
find . -name "*.vst3" -o -name "*.component" -o -name "*.app" | sort

# Determine the operating system for installation paths
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
    AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    VST3_DIR="$HOME/.vst3"
    AU_DIR="" # AU not typically used on Linux
else
    # Windows (if running in Git Bash or similar)
    VST3_DIR="$USERPROFILE/Documents/VST3"
    AU_DIR=""
fi

# Create plugin directories if they don't exist
echo
echo "=== Installing Plugins ==="

if [ ! -z "$VST3_DIR" ]; then
    mkdir -p "$VST3_DIR"
fi

if [ ! -z "$AU_DIR" ]; then
    mkdir -p "$AU_DIR"
fi

# Install VST3
VST3_BUILT=$(find . -name "ChopsBrowserPlugin.vst3" -type d | head -1)
if [ ! -z "$VST3_BUILT" ] && [ ! -z "$VST3_DIR" ]; then
    echo "Installing VST3 plugin..."
    echo "From: ${SCRIPT_DIR}/build/${VST3_BUILT}"
    echo "To: ${VST3_DIR}/"
    
    # Remove existing installation
    if [ -d "${VST3_DIR}/ChopsBrowserPlugin.vst3" ]; then
        echo "Removing existing VST3 installation..."
        rm -rf "${VST3_DIR}/ChopsBrowserPlugin.vst3"
    fi
    
    # Copy new version
    cp -R "${SCRIPT_DIR}/build/${VST3_BUILT}" "${VST3_DIR}/"
    
    if [ $? -eq 0 ]; then
        echo "✅ VST3 installed successfully!"
        echo "   Location: ${VST3_DIR}/ChopsBrowserPlugin.vst3"
    else
        echo "❌ VST3 installation failed!"
    fi
else
    echo "❌ VST3 plugin not found in build output"
    echo "   This usually means the Plugin/CMakeLists.txt is missing or the build failed"
fi

# Install AU (macOS only)
if [[ "$OSTYPE" == "darwin"* ]]; then
    AU_BUILT=$(find . -name "ChopsBrowserPlugin.component" -type d | head -1)
    if [ ! -z "$AU_BUILT" ] && [ ! -z "$AU_DIR" ]; then
        echo "Installing AU plugin..."
        echo "From: ${SCRIPT_DIR}/build/${AU_BUILT}"
        echo "To: ${AU_DIR}/"
        
        # Remove existing installation
        if [ -d "${AU_DIR}/ChopsBrowserPlugin.component" ]; then
            echo "Removing existing AU installation..."
            rm -rf "${AU_DIR}/ChopsBrowserPlugin.component"
        fi
        
        # Copy new version
        cp -R "${SCRIPT_DIR}/build/${AU_BUILT}" "${AU_DIR}/"
        
        if [ $? -eq 0 ]; then
            echo "✅ AU installed successfully!"
            echo "   Location: ${AU_DIR}/ChopsBrowserPlugin.component"
        else
            echo "❌ AU installation failed!"
        fi
    else
        echo "⚠️  AU plugin not found in build output"
    fi
fi

# Check if we built the standalone app
if [ -d "StandaloneApp/ChopsLibraryManager.app" ] || [ -f "StandaloneApp/ChopsLibraryManager" ]; then
    echo
    echo "=== Standalone App Built Successfully ==="
    if [ -d "StandaloneApp/ChopsLibraryManager.app" ]; then
        echo "App Bundle: ${SCRIPT_DIR}/build/StandaloneApp/ChopsLibraryManager.app"
        echo "To run: open \"${SCRIPT_DIR}/build/StandaloneApp/ChopsLibraryManager.app\""
    else
        echo "Executable: ${SCRIPT_DIR}/build/StandaloneApp/ChopsLibraryManager"
        echo "To run: \"${SCRIPT_DIR}/build/StandaloneApp/ChopsLibraryManager\""
    fi
fi

echo
echo "=== Build Summary ==="
echo "Build location: ${SCRIPT_DIR}/build/"
echo "Plugin formats built: VST3, AU (macOS), Standalone"
echo
echo "=== Next Steps ==="
echo "1. Open your DAW (Ableton Live, Logic Pro, etc.)"
echo "2. Rescan plugins or restart the DAW"
echo "3. Look for 'Chops Browser' in your plugin list"
echo "4. The plugin should now appear under Effects or Tools"
echo
echo "=== DAW-Specific Instructions ==="
echo "Ableton Live:"
echo "  - Go to Preferences > File/Folder > VST3 Plugin Custom Folder"
echo "  - Make sure ${VST3_DIR} is scanned"
echo "  - Restart Ableton and look under Audio Effects > VST3"
echo
echo "Logic Pro (macOS):"
echo "  - The AU plugin should be automatically detected"
echo "  - Look under Audio > Audio FX > Camp Rock > Chops Browser"
echo
echo "Done! 🎵"