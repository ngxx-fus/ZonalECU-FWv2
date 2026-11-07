#!/usr/bin/env bash
# ==============================================================================
# Script: static_analysis.sh
# Purpose: Run Clangd LSP static analysis & semantic checks on all project files
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

cd "${PROJECT_ROOT}"

echo "=================================================="
echo " Starting Clangd Static Analysis"
echo " Project root: ${PROJECT_ROOT}"
echo "=================================================="

# Check for required executables
if ! command -v clangd &> /dev/null; then
    echo "ERROR: 'clangd' executable not found. Please install clangd (e.g. sudo apt install clangd)."
    exit 1
fi

if ! command -v python3 &> /dev/null; then
    echo "ERROR: 'python3' executable not found. Please install python3."
    exit 1
fi

# Ensure compile_commands.json exists in build/
if [ ! -f "build/compile_commands.json" ]; then
    echo "Notice: build/compile_commands.json not found."
    if command -v idf.py &> /dev/null; then
        echo "Running 'idf.py reconfigure' to generate compilation database..."
        idf.py reconfigure
    elif [ -f "/home/fus/.fus/esp/export.sh" ]; then
        # shellcheck disable=SC1091
        . "/home/fus/.fus/esp/export.sh"
        idf.py reconfigure
    elif [ -n "${IDF_PATH:-}" ] && [ -f "${IDF_PATH}/export.sh" ]; then
        # shellcheck disable=SC1090
        . "${IDF_PATH}/export.sh"
        idf.py reconfigure
    else
        echo "ERROR: Cannot find build/compile_commands.json and idf.py is not available."
        exit 1
    fi
fi

# Discover all project C source files (main/ and comp/, excluding .template)
# shellcheck disable=SC2207
FILES=($(find main comp -name "*.c" ! -path "*/.template/*" | sort))
TOTAL_FILES=${#FILES[@]}

echo "Scanning files for static analysis (${TOTAL_FILES} files):"
for file in "${FILES[@]}"; do
    echo "  - ${file}"
done
echo "--------------------------------------------------"

# Run Python Clangd LSP Checker
python3 "${SCRIPT_DIR}/clangd_check.py" "${PROJECT_ROOT}" "${FILES[@]}"
