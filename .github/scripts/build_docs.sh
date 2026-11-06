#!/usr/bin/env bash
# ==============================================================================
# Script: build_docs.sh
# Purpose: Generate Doxygen HTML documentation for ZonalECU-FWv2
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

echo "=================================================="
echo " Generating Doxygen Documentation"
echo " Docs directory: ${PROJECT_ROOT}/docs"
echo "=================================================="

if ! command -v doxygen &> /dev/null; then
    echo "ERROR: 'doxygen' command not found. Please install doxygen and graphviz."
    exit 1
fi

cd "${PROJECT_ROOT}/docs"
doxygen Doxyfile
touch "${PROJECT_ROOT}/docs/doxygen_html/.nojekyll"

echo "=================================================="
echo " Documentation Generated Successfully!"
echo " Output directory: ${PROJECT_ROOT}/docs/doxygen_html"
echo "=================================================="
