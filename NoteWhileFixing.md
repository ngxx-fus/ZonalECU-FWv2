# Project Troubleshooting & Engineering Lessons Learned (`NoteWhileFixing.md`)

This document records technical issues diagnosed across the project, root causes identified, fixes applied, and engineering lessons learned to prevent future regressions.

---

## 1. Summary of Logged Entries

| ID | Category | Summary / Symptom | Root Cause | Prevention Rule |
| :--- | :--- | :--- | :--- | :--- |
| **001** | CI / Container | `/bin/sh: sudo: not found` (Exit Code 127) | Container (`espressif/idf`) runs as root and lacks `sudo`. | Detect `sudo` availability before invoking package managers in CI scripts. |
| **002** | Formatting | Enum assignment (`=`) alignment collapsed by `clang-format` | `.clang-format` `AlignConsecutiveAssignments` affects local function variables. | Isolate enum alignment into `format_rules.py`; do not mix with general-purpose C formater. |
| **003** | Documentation | ASCII table inside `@verbatim` merged with tag line | Regex parser treated `@verbatim` as a generic Doxygen header tag. | Isolate verbatim blocks with strict passthrough logic in doc parsing scripts. |

---

## 2. Detailed Technical Entries

### [ENTRY-001] CI Docker Container Lacks `sudo` (Exit Code 127)
- **Date**: 2026-11-11
- **Component**: `.github/workflows/ci.yml` (`Job 1: Clangd Static Analysis`)
- **Symptom**:
  ```text
  Run if ! command -v clangd &> /dev/null; then
  /__w/_temp/...sh: 5: sudo: not found
  clangd not found. Installing...
  /__w/_temp/...sh: 6: sudo: not found
  Error: Process completed with exit code 127.
  ```
- **Root Cause**:
  - `Job 0` runs directly on the GitHub Actions host runner (`runs-on: ubuntu-latest`), where a non-root `runner` user has `sudo` privileges.
  - `Job 1` runs inside a Docker container (`container: image: espressif/idf:release-v6.1`). Docker containers typically run as the `root` user (`uid=0`) and do not have the `sudo` package installed. Calling `sudo apt-get` directly triggers a command-not-found error.
- **Fix Applied**:
  - Added dynamic `sudo` resolution before running package management commands:
    ```bash
    SUDO=""
    if command -v sudo &> /dev/null; then SUDO="sudo"; fi
    $SUDO apt-get update -o Acquire::Retries=3 || true
    $SUDO apt-get install -y --no-install-recommends clangd
    ```
- **Key Lesson & Prevention Rule**:
  - Never assume `sudo` exists in Docker/containerized CI steps. Always detect `command -v sudo` or invoke commands directly when running as root.

---

### [ENTRY-002] Clang-Format Collapsing Enum Assignment Alignment
- **Date**: 2026-11-11
- **Component**: `.clang-format` & `.github/scripts/format_rules.py`
- **Symptom**:
  - In `FW-ESP/comp/App/SOME-IP/Core/SOMEIP_Types.h`, `e_someip_result` had its `=` operators collapsed to 1 space after short enum names (`SOMEIP_RES_OK = 0`), while trailing comments remained aligned.
- **Root Cause**:
  - Setting `AlignConsecutiveAssignments: Consecutive` in `.clang-format` correctly aligns `=` in enums, but also aggressively aligns local variable assignments inside C function bodies (e.g. `p_cur->p_buf = ...`), which violates C code style.
- **Fix Applied**:
  - Kept `.clang-format` with `AlignConsecutiveAssignments: None` to protect C function implementations.
  - Delegated strict enum `=` alignment (`longest_name + 1 space`) and trailing comments (`longest_code + 1 space`) exclusively to `format_rules.py` (`align_all_enums()`).
- **Key Lesson & Prevention Rule**:
  - Maintain strict separation of concerns: general AST formatting is handled by `clang-format`, while project-specific structural alignments (enums, Doxygen column layouts, macro backslashes) are handled by `format_rules.py`.

---

### [ENTRY-003] Doxygen Parser Mangling Preformatted ASCII Tables (`@verbatim`)
- **Date**: 2026-11-11
- **Component**: `.github/scripts/format_rules.py` (`align_doxygen_block`)
- **Symptom**:
  - In `FW-ESP/comp/App/SOME-IP/Core/SOMEIP_Check.h`, the ASCII decision matrix table for `SOMEIP_CheckShouldReplyError()` had its first row merged directly onto the `@verbatim` tag line:
    ```c
    *  @verbatim    received                                       reply ERROR?   spec
    ```
- **Root Cause**:
  - The Doxygen block parsing regex matched any `@tag` and treated `@verbatim` as a standard documentation tag, padding it and joining the subsequent line of text into its description field.
- **Fix Applied**:
  - Added dedicated verbatim tracking state (`is_verbatim`) in `align_doxygen_block()`.
  - Lines between `@verbatim` and `@endverbatim` (inclusive) are passed through 100% untouched without any regex parsing or tag reformatting.
- **Key Lesson & Prevention Rule**:
  - Any automated comment or documentation formatter must treat preformatted blocks (`@verbatim`, `@code`, ASCII diagrams) as atomic raw text blocks and bypass standard indentation rules.
