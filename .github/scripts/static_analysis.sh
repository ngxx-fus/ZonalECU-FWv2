#!/usr/bin/env bash
# ==============================================================================
# Script: static_analysis.sh
# Purpose: Run Clangd static analysis & semantic checks on all project files
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

cd "${PROJECT_ROOT}"

echo "=================================================="
echo " Starting Clangd Static Analysis"
echo " Project root: ${PROJECT_ROOT}"
echo "=================================================="

# Check for clangd executable
if ! command -v clangd &> /dev/null; then
    echo "ERROR: 'clangd' executable not found. Please install clangd (e.g. sudo apt install clangd)."
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

# Discover all project C source files (main/ and components/, excluding .template)
FILES=$(find main components -name "*.c" ! -path "*/.template/*" | sort)

TOTAL_FILES=0
FAILED_FILES=0

echo "Scanning files for static analysis:"
for file in $FILES; do
    echo "  - ${file}"
    TOTAL_FILES=$((TOTAL_FILES + 1))
done
echo "Total files to analyze: ${TOTAL_FILES}"
echo "--------------------------------------------------"

for file in $FILES; do
    echo "Checking: ${file} ..."
    OUTPUT=$(clangd --query-driver="**/xtensa-esp32-elf-*" --check="${file}" 2>&1 || true)
    
    # Check if clangd output reports errors
    if echo "${OUTPUT}" | grep -qE "All checks completed, [1-9][0-9]* errors"; then
        echo "❌ FAILED: ${file}"
        echo "${OUTPUT}" | grep -E "error:|All checks completed"
        FAILED_FILES=$((FAILED_FILES + 1))
    else
        echo "✅ PASSED: ${file} (0 errors)"
    fi
done

echo "=================================================="
if [ ${FAILED_FILES} -gt 0 ]; then
    echo "❌ Clangd Static Analysis FAILED: ${FAILED_FILES}/${TOTAL_FILES} files have errors."
    exit 1
else
    echo "✅ Clangd Static Analysis PASSED: ${TOTAL_FILES}/${TOTAL_FILES} files clean (0 errors)."
fi
echo "=================================================="
