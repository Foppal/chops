#!/bin/bash

echo "Listing project structure for ChopsBrowser..."
echo "Output will be saved to project_structure.txt"

# Get the directory where the script is located (should be project root)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Use a simpler find command
# We will exclude common build artifacts and the potentially large JUCE directory
# to keep the output manageable, but you can adjust this.
find "$SCRIPT_DIR" \
    -not \( -path "$SCRIPT_DIR/build/*" -prune \) \
    -not \( -path "$SCRIPT_DIR/JUCE/*" -prune \) \
    -not \( -path "$SCRIPT_DIR/.git/*" -prune \) \
    -not \( -path "$SCRIPT_DIR/UI/node_modules/*" -prune \) \
    -not \( -name ".DS_Store" \) \
    -print | sort > project_structure.txt

echo "Done. Please share the content of project_structure.txt"
echo "Also, please share the content of:"
echo "1. CHOPSBROWSER/CMakeLists.txt (your root CMakeLists.txt)"
echo "2. CHOPSBROWSER/StandaloneApp/CMakeLists.txt"
echo "3. CHOPSBROWSER/Plugin/CMakeLists.txt (if it exists and is relevant now)"