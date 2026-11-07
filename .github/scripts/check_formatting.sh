#!/usr/bin/env bash
# ==============================================================================
# Script: check_formatting.sh
# Purpose: Check that all C/H files comply with project formatting rules.
#          Fails (exit 1) if any file needs reformatting.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

cd "${PROJECT_ROOT}"

echo "=================================================="
echo " Checking Code Formatting Rules"
echo "=================================================="

# Check for required executables
if ! command -v clang-format &> /dev/null; then
    echo "ERROR: 'clang-format' executable not found. Please install clang-format."
    exit 1
fi

if ! command -v python3 &> /dev/null; then
    echo "ERROR: 'python3' executable not found. Please install python3."
    exit 1
fi

# Run format_rules.py across target directories
python3 "${SCRIPT_DIR}/format_rules.py" FW-Common FW-ESP FW-STM

# Verify working tree is clean using git diff on target source trees
if ! git diff --exit-code -- FW-Common FW-ESP FW-STM; then
    echo "=================================================="
    echo "❌ FAILED: Unformatted code detected!"
    echo "The changes shown above do not follow project CodingRules.md."
    echo "Please run locally:"
    echo "  python3 .github/scripts/format_rules.py FW-Common FW-ESP FW-STM"
    echo "and commit the formatted code."
    echo "=================================================="
    exit 1
fi

echo "=================================================="
echo "✅ All source files comply with formatting rules!"
echo "=================================================="
