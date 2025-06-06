#!/bin/bash

# ChopsBrowser Build Script

echo "=== ChopsBrowser Build Script ==="
echo

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build
echo
echo "Building..."
make -j8

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo
echo "Build completed successfully!"
echo

# List the built products
echo "Built products:"
find . -name "*.vst3" -o -name "*.component" -o -name "*.app" | sort

echo
echo "Done!"

# Optional: Show the paths for easy copying
echo
echo "=== Installation Paths ==="
echo "VST3: ~/Library/Audio/Plug-Ins/VST3/"
echo "AU: ~/Library/Audio/Plug-Ins/Components/"
echo
echo "To install the plugin, run:"
echo "cp -R ${SCRIPT_DIR}/build/ChopsBrowser_artefacts/VST3/ChopsBrowser.vst3 ~/Library/Audio/Plug-Ins/VST3/"
echo "cp -R ${SCRIPT_DIR}/build/ChopsBrowser_artefacts/AU/ChopsBrowser.component ~/Library/Audio/Plug-Ins/Components/"