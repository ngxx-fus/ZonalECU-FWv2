#!/usr/bin/env bash
# ==============================================================================
# Script: check_git_conventions.sh
# Purpose: Check branch naming and commit message conventions (Recommended)
# ==============================================================================
set -euo pipefail

echo "=================================================="
echo " Checking Git Branch & Commit Conventions"
echo "=================================================="

FAILED=0

# ------------------------------------------------------------------------------
# 1. Check Branch Name
# Pattern: ZE-(Feature|Improve|Update|Fix)-<ShortDescription>
# ------------------------------------------------------------------------------
BRANCH="${GITHUB_HEAD_REF:-${GITHUB_REF_NAME:-$(git rev-parse --abbrev-ref HEAD)}}"
echo "Current branch: ${BRANCH}"

BRANCH_REGEX="^ZE-(Feature|Improve|Update|Fix)-[A-Za-z0-9._-]+$"
MAIN_BRANCH_REGEX="^(FWv2-Master|main|master)$"

if [[ "${BRANCH}" =~ $MAIN_BRANCH_REGEX ]]; then
    echo "✅ Branch is a protected/mainline branch (${BRANCH}), skipping branch name check."
elif [[ "${BRANCH}" =~ $BRANCH_REGEX ]]; then
    echo "✅ PASSED: Branch name '${BRANCH}' complies with convention."
else
    echo "⚠️ WARNING: Branch name '${BRANCH}' does not follow convention:"
    echo "   Expected: ZE-<Feature/Improve/Update/Fix>-<ShortDescription>"
    echo "   Example:  ZE-Feature-UpdateGitHubAction"
    FAILED=1
fi

echo "--------------------------------------------------"

# ------------------------------------------------------------------------------
# 2. Check Commit Messages
# Pattern: <ZE/ZonalECU/ZonalECU-FWv2> | MM/DD/YY HH:MM:SS | <message>
# ------------------------------------------------------------------------------
COMMIT_REGEX="^(ZE|ZonalECU|ZonalECU-FWv2) \| [0-9]{2}/[0-9]{2}/[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2} \| .+$"
MERGE_REGEX="^Merge (branch|pull request|remote-tracking) .+$"

# Determine commit range
if [ -n "${GITHUB_BASE_REF:-}" ]; then
    echo "Checking commits in PR (against origin/${GITHUB_BASE_REF})..."
    git fetch origin "${GITHUB_BASE_REF}" --depth=50 2>/dev/null || true
    COMMITS=$(git log "origin/${GITHUB_BASE_REF}..HEAD" --format="%H" || git log -1 --format="%H")
else
    echo "Checking latest commit(s)..."
    COMMITS=$(git log -1 --format="%H")
fi

COMMIT_COUNT=0
for HASH in $COMMITS; do
    COMMIT_COUNT=$((COMMIT_COUNT + 1))
    MSG=$(git log -1 --format="%s" "${HASH}")
    
    if [[ "${MSG}" =~ $COMMIT_REGEX ]] || [[ "${MSG}" =~ $MERGE_REGEX ]]; then
        echo "✅ PASSED: [${HASH:0:7}] ${MSG}"
    else
        echo "⚠️ WARNING: [${HASH:0:7}] does not follow commit format:"
        echo "   Commit message : \"${MSG}\""
        echo "   Expected format: <ZE/ZonalECU/ZonalECU-FWv2> | MM/DD/YY HH:MM:SS | <message>"
        echo "   Example        : ZonalECU-FWv2 | 11/06/26 20:04:03 | Updated static-analysis job check"
        FAILED=1
    fi
done

echo "=================================================="
if [ ${FAILED} -ne 0 ]; then
    echo "⚠️ Git conventions check finished with warning(s)."
    echo "Notice: This check is HIGHLY RECOMMENDED (non-blocking)."
    exit 1
else
    echo "✅ All Git branch and commit conventions passed!"
    exit 0
fi
