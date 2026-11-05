#!/usr/bin/env sh
# ============================================================================================================
# ZonalECU-FWv2 Portable Git Pre-Commit Hook (tools/pre-commit-hook.sh -> .git/hooks/pre-commit)
# ============================================================================================================
# Purpose:
#   Automatically runs `tools/check_coding_rules.py` on all staged `.c` and `.h` files before `git commit`.
#   100% portable across Windows, Linux, and macOS laptops (Git Bash, VS Code Git UI, GitHub Desktop,
#   and ESP-IDF terminals) without any hardcoded user/machine paths.
#
# Installation (for a coworker after cloning):
#   Option A (Using pre-commit framework):
#     pip install pre-commit && pre-commit install
#   Option B (Zero-dependency native Git hook):
#     cp tools/pre-commit-hook.sh .git/hooks/pre-commit && chmod +x .git/hooks/pre-commit
#   Option C (Automatic via CMake):
#     Running `idf.py reconfigure` or `idf.py build` automatically installs this hook if missing.
#
# Bypass (for emergency/WIP commits only):
#   git commit -m "WIP: message" --no-verify
# ============================================================================================================

set -e

REPO_ROOT=$(git rev-parse --show-toplevel)
cd "$REPO_ROOT"

# ------------------------------------------------------------------------------------------------------------
# 1. Collect Staged C/Header Files (Added, Copied, Modified, Renamed — excluding Deleted files & .template/)
# ------------------------------------------------------------------------------------------------------------
STAGED_C_FILES=$(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(c|h)$' | grep -v '^components/\.template/' || true)

# Exit immediately if no .c or .h files are staged in this commit
if [ -z "$STAGED_C_FILES" ]; then
    exit 0
fi

# ------------------------------------------------------------------------------------------------------------
# 2. Portable Python 3 Discovery (Works across any developer's laptop)
#    Search order:
#      a) Active virtual environment / ESP-IDF environment variables ($IDF_PYTHON_ENV_PATH, $IDF_TOOLS_PATH, $VIRTUAL_ENV)
#      b) System PATH (`python3`, `python`, `py`) — skipping the Windows Store stub alias
#      c) Standard ESP-IDF & Python installation directories on Windows, Linux, and macOS
#      d) Workspace-relative `apps/Espressif` directory (`$REPO_ROOT/../../apps/Espressif/...`)
# ------------------------------------------------------------------------------------------------------------
PYTHON_CMD=""

is_valid_python() {
    [ -n "$1" ] && "$1" -c "import sys; sys.exit(0 if sys.version_info[0] >= 3 else 1)" >/dev/null 2>&1
}

# (a) Check environment variables first
for env_py in \
    "${IDF_PYTHON_ENV_PATH:+$IDF_PYTHON_ENV_PATH/Scripts/python.exe}" \
    "${IDF_PYTHON_ENV_PATH:+$IDF_PYTHON_ENV_PATH/bin/python3}" \
    "${VIRTUAL_ENV:+$VIRTUAL_ENV/Scripts/python.exe}" \
    "${VIRTUAL_ENV:+$VIRTUAL_ENV/bin/python3}"
do
    if is_valid_python "$env_py"; then
        PYTHON_CMD="$env_py"
        break
    fi
done

# (b) Check system PATH (`python3`, `python`, `py`)
if [ -z "$PYTHON_CMD" ]; then
    for path_cmd in python3 python py; do
        if command -v "$path_cmd" >/dev/null 2>&1 && is_valid_python "$path_cmd"; then
            PYTHON_CMD="$path_cmd"
            break
        fi
    done
fi

# (c) Check standard ESP-IDF & system Python locations + workspace-relative paths
if [ -z "$PYTHON_CMD" ]; then
    for candidate in \
        "${IDF_TOOLS_PATH:-$HOME/.espressif}"/python_env/*/Scripts/python.exe \
        "${IDF_TOOLS_PATH:-$HOME/.espressif}"/python_env/*/bin/python3 \
        "$HOME"/.espressif/python_env/*/Scripts/python.exe \
        "$HOME"/.espressif/python_env/*/bin/python3 \
        /c/Espressif/python_env/*/Scripts/python.exe \
        /d/Espressif/python_env/*/Scripts/python.exe \
        "$REPO_ROOT"/../../apps/Espressif/python_env/*/Scripts/python.exe \
        "$REPO_ROOT"/../apps/Espressif/python_env/*/Scripts/python.exe \
        "${LOCALAPPDATA:-$HOME/AppData/Local}"/Programs/Python/Python3*/python.exe \
        /c/Python3*/python.exe \
        "/c/Program Files"/Python3*/python.exe
    do
        if [ -x "$candidate" ] && is_valid_python "$candidate"; then
            PYTHON_CMD="$candidate"
            break
        fi
    done
fi

if [ -z "$PYTHON_CMD" ]; then
    echo "[ERROR] ZonalECU pre-commit hook could not find a Python 3 interpreter."
    echo "        Please ensure Python 3 or ESP-IDF is installed, or set IDF_PYTHON_ENV_PATH."
    exit 1
fi

# ------------------------------------------------------------------------------------------------------------
# 3. Execute ZonalECU-FWv2 Coding Rules Checker on Staged Files
# ------------------------------------------------------------------------------------------------------------
echo "[pre-commit] Running ZonalECU-FWv2 C Coding Standard checks (tools/check_coding_rules.py)..."
# shellcheck disable=SC2086
"$PYTHON_CMD" tools/check_coding_rules.py $STAGED_C_FILES
