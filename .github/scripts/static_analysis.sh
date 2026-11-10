#!/usr/bin/env bash
# ==============================================================================
# Script: static_analysis.sh
# Purpose: Run Clangd LSP static analysis & semantic checks on FW-Common, FW-ESP, and FW-STM
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

# Ensure compile_commands.json exists in FW-ESP/build/
if [ ! -f "FW-ESP/build/compile_commands.json" ]; then
    echo "Notice: FW-ESP/build/compile_commands.json not found."
    if command -v idf.py &> /dev/null; then
        echo "Running 'idf.py -C FW-ESP reconfigure' to generate compilation database..."
        idf.py -C FW-ESP reconfigure
    elif [ -f "/home/fus/.fus/esp/export.sh" ]; then
        # shellcheck disable=SC1091
        . "/home/fus/.fus/esp/export.sh"
        idf.py -C FW-ESP reconfigure
    elif [ -n "${IDF_PATH:-}" ] && [ -f "${IDF_PATH}/export.sh" ]; then
        # shellcheck disable=SC1090
        . "${IDF_PATH}/export.sh"
        idf.py -C FW-ESP reconfigure
    else
        echo "ERROR: Cannot find FW-ESP/build/compile_commands.json and idf.py is not available."
        exit 1
    fi
fi

# Discover all project C source files across FW-Common, FW-ESP, and FW-STM (excluding .template and build)
# shellcheck disable=SC2207
FILES=($(find FW-Common FW-ESP FW-STM -name "*.c" ! -path "*/.template/*" ! -path "*/build/*" | sort))
TOTAL_FILES=${#FILES[@]}

echo "Scanning files for static analysis (${TOTAL_FILES} files):"
for file in "${FILES[@]}"; do
    echo "  - ${file}"
done
echo "--------------------------------------------------"

if [ "${TOTAL_FILES}" -eq 0 ]; then
    echo "No C files found to analyze."
    exit 0
fi

# Phase 1: Run Python Clangd LSP Checker from PROJECT_ROOT (AST & Semantic)
echo ">>> [Phase 1/2] Running Clangd Semantic & AST Checks..."
python3 "${SCRIPT_DIR}/clangd_check.py" "${PROJECT_ROOT}" "${FILES[@]}"

# Phase 2: Run Target Compiler Strict Semantic & Dead-Code Checks (-Werror=unused-*)
echo ""
echo ">>> [Phase 2/2] Running Target Compiler Strict Semantic & Warning Check..."
python3 "${SCRIPT_DIR}/compiler_check.py" "FW-ESP/build/compile_commands.json" "${FILES[@]}"
