#!/usr/bin/env bash
# ==============================================================================
# Script: build_firmware.sh
# Purpose: Build ZonalECU-FWv2 ESP32 firmware using ESP-IDF
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
FW_ESP_DIR="${PROJECT_ROOT}/FW-ESP"

cd "${FW_ESP_DIR}"

echo "=================================================="
echo " Starting Firmware Build (ESP-IDF)"
echo " Firmware root: ${FW_ESP_DIR}"
echo "=================================================="

# Auto-detect and source export.sh if idf.py is not in PATH
if ! command -v idf.py &> /dev/null; then
    if [ -n "${IDF_PATH:-}" ] && [ -f "${IDF_PATH}/export.sh" ]; then
        echo "Sourcing IDF_PATH: ${IDF_PATH}/export.sh"
        # shellcheck disable=SC1090
        . "${IDF_PATH}/export.sh"
    elif [ -f "/home/fus/.fus/esp/export.sh" ]; then
        echo "Sourcing local developer IDF: /home/fus/.fus/esp/export.sh"
        # shellcheck disable=SC1091
        . "/home/fus/.fus/esp/export.sh"
    elif [ -f "/opt/esp/idf/export.sh" ]; then
        echo "Sourcing container IDF: /opt/esp/idf/export.sh"
        # shellcheck disable=SC1091
        . "/opt/esp/idf/export.sh"
    else
        echo "ERROR: idf.py not found in PATH and no export.sh could be located."
        exit 1
    fi
fi

echo "ESP-IDF Version: $(idf.py --version)"

# Execute build
idf.py build

echo "=================================================="
echo " Build Completed Successfully!"
echo " Binaries located in: ${FW_ESP_DIR}/build"
echo "=================================================="
