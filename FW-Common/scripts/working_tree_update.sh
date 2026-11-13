#!/usr/bin/env bash
# ==============================================================================
# Script: working_tree_update.sh
# Purpose: Local Working Tree Update & Static Check Runner (Job 0 & Job 1)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

cd "${PROJECT_ROOT}"

echo "=================================================="
echo " ZonalECU-FWv2 Local Pre-Commit Check (Job 0 & 1)"
echo " Project Root : ${PROJECT_ROOT}"
echo " Execution Time: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=================================================="

# Check for Python
PYTHON_CMD=""
if command -v python3 > /dev/null 2>&1; then
    PYTHON_CMD="python3"
elif command -v python > /dev/null 2>&1; then
    PYTHON_CMD="python"
else
    echo "❌ ERROR: Python executable not found."
    exit 1
fi

CHECK_ONLY=0
SKIP_FORMAT=0
SKIP_GIT_CONVENTIONS=0
SKIP_STATIC_ANALYSIS=0

for arg in "$@"; do
    case "$arg" in
        --check-only) CHECK_ONLY=1 ;;
        --skip-format) SKIP_FORMAT=1 ;;
        --skip-git-conventions) SKIP_GIT_CONVENTIONS=1 ;;
        --skip-static-analysis) SKIP_STATIC_ANALYSIS=1 ;;
    esac
done

OVERALL_SUCCESS=1
JOB0_STATUS="PASSED"
JOB1_STATUS="PASSED"

# ------------------------------------------------------------------------------
# JOB 0: Conventions & Formatting
# ------------------------------------------------------------------------------
echo ""
echo "=================================================="
echo " Job 0: Conventions & Formatting"
echo "=================================================="

if [ "$SKIP_GIT_CONVENTIONS" -eq 0 ]; then
    echo ">>> [Job 0.1] Checking Git Branch & Commit Conventions..."
    if [ -x ".github/scripts/check_git_conventions.sh" ]; then
        ./.github/scripts/check_git_conventions.sh || true
    fi

    COMMIT_DATE_STR="$(date '+%m/%d/%y %H:%M:%S')"
    echo ""
    echo "  [Commit Message Template for next commit]:"
    echo "  git commit -m \"ZonalECU-FWv2 | ${COMMIT_DATE_STR} | <Your message here>\""
    echo ""
fi

if [ "$SKIP_FORMAT" -eq 0 ]; then
    echo ">>> [Job 0.2] Running Code Formatting Rules (format_rules.py)..."
    if [ -f ".github/scripts/format_rules.py" ]; then
        "$PYTHON_CMD" .github/scripts/format_rules.py FW-Common FW-ESP FW-STM

        if ! git diff --exit-code -- FW-Common FW-ESP FW-STM > /dev/null 2>&1; then
            echo ""
            echo "⚠️ Code formatting modifications detected in working tree."
            if [ "$CHECK_ONLY" -eq 1 ]; then
                echo "❌ FAILED: Working tree contains unformatted code (--check-only mode)."
                JOB0_STATUS="FAILED"
                OVERALL_SUCCESS=0
            else
                echo "✅ Code reformatted according to CodingRules.md. Stage files before committing."
            fi
        else
            echo "✅ All source files comply with formatting rules!"
        fi
    fi
fi

# ------------------------------------------------------------------------------
# JOB 1: Clangd Static Analysis & Strict Compiler Checks
# ------------------------------------------------------------------------------
if [ "$SKIP_STATIC_ANALYSIS" -eq 0 ]; then
    echo ""
    echo "=================================================="
    echo " Job 1: Clangd Static Analysis & Compiler Checks"
    echo "=================================================="

    if [ ! -f "FW-ESP/build/compile_commands.json" ]; then
        if command -v idf.py > /dev/null 2>&1; then
            echo "Running 'idf.py -C FW-ESP reconfigure' to generate compilation database..."
            idf.py -C FW-ESP reconfigure
        else
            echo "❌ ERROR: Cannot find FW-ESP/build/compile_commands.json and idf.py is not available."
            JOB1_STATUS="FAILED"
            OVERALL_SUCCESS=0
        fi
    fi

    if [ -f "FW-ESP/build/compile_commands.json" ]; then
        if [ -x ".github/scripts/static_analysis.sh" ]; then
            ./.github/scripts/static_analysis.sh || {
                JOB1_STATUS="FAILED"
                OVERALL_SUCCESS=0
            }
        fi
    fi
fi

echo ""
echo "=================================================="
echo " Execution Summary Report"
echo "=================================================="
echo "  Job 0 (Conventions & Formatting) : ${JOB0_STATUS}"
echo "  Job 1 (Clangd & Compiler Check)  : ${JOB1_STATUS}"
echo "=================================================="

if [ "$OVERALL_SUCCESS" -eq 1 ]; then
    echo "✅ All local pre-commit checks PASSED successfully! Ready to commit."
    exit 0
else
    echo "❌ Some checks FAILED. Please review output before committing."
    exit 1
fi
