#!/usr/bin/env sh
# ============================================================================================================
# ZonalECU-FWv2 Native Git Pre-Commit Hook (tools/pre-commit-hook.sh -> .git/hooks/pre-commit)
# ============================================================================================================
# Purpose:
#   Automatically runs `tools/check_coding_rules.py` on all staged `.c` and `.h` files before `git commit`.
#   Works out-of-the-box in Git Bash, VS Code Git UI, GitHub Desktop, and ESP-IDF terminals on Windows/Linux
#   by automatically locating Python (system PATH or ESP-IDF's bundled Python virtual environment).
#
# Installation (to re-install after a fresh clone):
#   cp tools/pre-commit-hook.sh .git/hooks/pre-commit
#   chmod +x .git/hooks/pre-commit
#
# Bypass (for emergency/WIP commits only):
#   git commit -m "WIP: message" --no-verify
# ============================================================================================================

set -e

# ------------------------------------------------------------------------------------------------------------
# 1. Collect Staged C/Header Files (Added, Copied, Modified, Renamed — excluding Deleted files & .template/)
# ------------------------------------------------------------------------------------------------------------
STAGED_C_FILES=$(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(c|h)$' | grep -v '^components/\.template/' || true)

# Exit immediately if no .c or .h files are staged in this commit
if [ -z "$STAGED_C_FILES" ]; then
    exit 0
fi

# ------------------------------------------------------------------------------------------------------------
# 2. Locate a Working Python 3 Interpreter
#    (Checks PATH first, then ESP-IDF Python environments on Windows/Linux so VS Code GUI commits work)
# ------------------------------------------------------------------------------------------------------------
PYTHON_CMD=""

for candidate in \
    "python3" \
    "python" \
    "/c/ngxx.fus.workspace/apps/Espressif/python_env/idf5.5_py3.11_env/Scripts/python.exe" \
    "C:/ngxx.fus.workspace/apps/Espressif/python_env/idf5.5_py3.11_env/Scripts/python.exe" \
    "$HOME/.espressif/python_env"/*/Scripts/python.exe \
    "$HOME/.espressif/python_env"/*/bin/python3
do
    if command -v "$candidate" >/dev/null 2>&1; then
        # Verify it is a real Python interpreter (not the Windows Store stub alias)
        if "$candidate" -c "import sys" >/dev/null 2>&1; then
            PYTHON_CMD="$candidate"
            break
        fi
    elif [ -x "$candidate" ]; then
        if "$candidate" -c "import sys" >/dev/null 2>&1; then
            PYTHON_CMD="$candidate"
            break
        fi
    fi
done

if [ -z "$PYTHON_CMD" ]; then
    echo "[ERROR] ZonalECU pre-commit hook could not find a Python 3 interpreter."
    echo "        Ensure Python 3 or ESP-IDF Python environment is installed."
    exit 1
fi

# ------------------------------------------------------------------------------------------------------------
# 3. Execute ZonalECU-FWv2 Coding Rules Checker on Staged Files
# ------------------------------------------------------------------------------------------------------------
echo "[pre-commit] Running ZonalECU-FWv2 C Coding Standard checks (tools/check_coding_rules.py)..."
# shellcheck disable=SC2086
"$PYTHON_CMD" tools/check_coding_rules.py $STAGED_C_FILES

