# Project Troubleshooting & Engineering Lessons Learned (`NoteWhileFixing.md`)

This document records technical issues diagnosed across the project, root causes identified, fixes applied, and engineering lessons learned to prevent future regressions.

---

## 1. Summary of Logged Entries

| ID | Category | Summary / Symptom | Root Cause | Prevention Rule |
| :--- | :--- | :--- | :--- | :--- |
| **001** | CI / Container | `/bin/sh: sudo: not found` (Exit Code 127) | Container (`espressif/idf`) runs as root and lacks `sudo`. | Detect `sudo` availability before invoking package managers in CI scripts. |
| **002** | Formatting | Enum assignment (`=`) alignment collapsed by `clang-format` | `.clang-format` `AlignConsecutiveAssignments` affects local function variables. | Isolate enum alignment into `format_rules.py`; do not mix with general-purpose C formater. |
| **003** | Documentation | ASCII table inside `@verbatim` merged with tag line | Regex parser treated `@verbatim` as a generic Doxygen header tag. | Isolate verbatim blocks with strict passthrough logic in doc parsing scripts. |
| **004** | CI / Docker DNS | `Could not resolve 'security.ubuntu.com'` in container | Docker inherits unreachable loopback resolver (`127.0.0.53`) from host. | Add `options: --dns 8.8.8.8 --dns 1.1.1.1` and fallback nameservers in container steps. |
| **005** | CI / APT Network | `azure.archive.ubuntu.com` stalled / hung for 3+ minutes | Default APT TCP timeout is 120s with retries on throttled Azure mirror. | Added `Acquire::http::Timeout=5`, `timeout 30s` guard, mirror replacement, and `timeout-minutes`. |
| **006** | Static Analysis / Windows | `OSError: [WinError 10093]` in `clangd_check.py` & invalid URI | `select.select` only works on sockets on Windows; `file://C:\...` is malformed URI. | Use reader thread + `queue.Queue` and `pathlib.Path.as_uri()` for cross-platform LSP. |
| **007** | Static Analysis / Windows | `compiler_check.py` WinError 2 file not found | `shlex.split` removes backslashes on Windows; mangles escaped compiler flags. | Use native command string with regex replacement on Windows; normalize paths with lowercase. |

---

## 2. Detailed Technical Entries

### [ENTRY-001] CI Docker Container Lacks `sudo` & Bashism In `/bin/sh` (Exit Code 127)
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
  1. `Job 1` runs inside an `espressif/idf:release-v6.1` Docker container as root (`uid=0`), which does not bundle the `sudo` binary.
  2. In GitHub Actions, workflow `run:` steps run under `/bin/sh -e` (POSIX `dash` on Ubuntu). The syntax `command -v sudo &> /dev/null` is a **bashism**: in POSIX `dash`, `&` puts the command in the background, and `> /dev/null` redirects a null command, which **always returns exit code 0 (success)**! As a result, `if command -v sudo &> /dev/null; then SUDO="sudo"; fi` was **always evaluated to true**, forcing `$SUDO` to be `"sudo"` even when `sudo` did not exist.
- **Fix Applied**:
  - Replaced non-POSIX `&> /dev/null` with standard POSIX redirection `> /dev/null 2>&1`.
  - Added an explicit non-root user check `[ "$(id -u)" -ne 0 ]` so that container environments running as root never attempt to invoke `sudo`:
    ```bash
    SUDO=""
    if [ "$(id -u)" -ne 0 ] && command -v sudo > /dev/null 2>&1; then
      SUDO="sudo"
    fi
    $SUDO apt-get update -o Acquire::Retries=3 || true
    $SUDO apt-get install -y --no-install-recommends clangd
    ```
- **Key Lesson & Prevention Rule**:
  - Never use bash-specific `&> /dev/null` inside GitHub Actions workflow inline scripts unless `shell: bash` is explicitly specified; always use POSIX `> /dev/null 2>&1`.
  - Check `[ "$(id -u)" -ne 0 ]` in addition to `command -v sudo` to properly detect container root environments.

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

---

### [ENTRY-004] Docker Container DNS Failure Causing "Could not resolve 'archive.ubuntu.com'"
- **Date**: 2026-11-11
- **Component**: `.github/workflows/ci.yml` (`Job 1: Clangd Static Analysis` & `Job 3: FW-ESP Build`)
- **Symptom**:
  ```text
  Ign:1 http://security.ubuntu.com/ubuntu noble-security InRelease
  Err:1 http://security.ubuntu.com/ubuntu noble-security InRelease
    Could not resolve 'security.ubuntu.com'
  Err:2 http://archive.ubuntu.com/ubuntu noble InRelease
    Could not resolve 'archive.ubuntu.com'
  Reading package lists...
  E: Unable to locate package clangd
  Error: Process completed with exit code 100.
  ```
- **Root Cause**:
  - On Ubuntu GitHub Actions host runners, system DNS resolution is handled by `systemd-resolved` listening on loopback `127.0.0.53`.
  - When a container job starts (`container: image: espressif/idf:release-v6.1`), Docker inherits `/etc/resolv.conf` from the host runner. Inside the container's isolated network namespace, `127.0.0.53` points to the container's own loopback interface where no DNS resolver is running. Consequently, all domain name lookups fail with `Could not resolve ...`.
  - Because DNS lookups fail, `apt-get update` cannot fetch repository indices, leaving package lists empty and causing `apt-get install clangd` to fail with `E: Unable to locate package clangd` (Exit Code 100).
- **Fix Applied**:
  1. Configured explicit public DNS servers directly in GitHub Actions container jobs using `options: --dns 8.8.8.8 --dns 1.1.1.1`:
     ```yaml
     container:
       image: espressif/idf:release-v6.1
       options: --dns 8.8.8.8 --dns 1.1.1.1
     ```
  2. Added runtime fallback inside the installation script to populate `/etc/resolv.conf` if writeable:
     ```bash
     if [ -w /etc/resolv.conf ]; then
       echo "nameserver 8.8.8.8" > /etc/resolv.conf
       echo "nameserver 1.1.1.1" >> /etc/resolv.conf
     fi
     ```
- **Key Lesson & Prevention Rule**:
  - Whenever running CI jobs inside Docker containers that require outbound internet connectivity (package managers, git submodules), always declare `options: --dns 8.8.8.8 --dns 1.1.1.1` to prevent host `systemd-resolved` loopback leakage.

---

### [ENTRY-005] APT Mirror Stalling on Azure Ubuntu Mirrors Causing CI Hang
- **Date**: 2026-11-11
- **Component**: `.github/workflows/ci.yml` (Package installation across all jobs)
- **Symptom**:
  ```text
  Ign:2 http://azure.archive.ubuntu.com/ubuntu resolute InRelease
  Ign:3 http://azure.archive.ubuntu.com/ubuntu resolute-updates InRelease
  Ign:4 http://azure.archive.ubuntu.com/ubuntu resolute-backports InRelease
  Ign:5 http://azure.archive.ubuntu.com/ubuntu resolute-security InRelease
  ... (retrying repeatedly, hanging for 3+ minutes)
  ```
- **Root Cause**:
  - GitHub Actions Ubuntu runners use Microsoft Azure network mirrors (`azure.archive.ubuntu.com`). During peak loads or regional network throttling, these mirrors drop TCP connections or become completely unresponsive.
  - By default, `apt-get` has an aggressive TCP timeout of 120–300 seconds per mirror connection. Combined with retries, a single stalled mirror blocks CI execution for several minutes.
- **Fix Applied**:
  1. Replaced flaky Azure mirrors with official canonical Ubuntu mirrors:
     ```bash
     $SUDO sed -i 's/azure\.archive\.ubuntu\.com/archive\.ubuntu\.com/g' /etc/apt/sources.list /etc/apt/sources.list.d/* 2>/dev/null || true
     ```
  2. Enforced aggressive 5-second HTTP/HTTPS connection timeouts and 1 retry on `apt-get`:
     `-o Acquire::http::Timeout=5 -o Acquire::https::Timeout=5 -o Acquire::Retries=1`
  3. Wrapped `apt-get update` with the Linux coreutils `timeout 30s` command to guarantee execution never stalls past 30 seconds:
     ```bash
     timeout 30s $SUDO apt-get update -o Acquire::http::Timeout=5 -o Acquire::https::Timeout=5 -o Acquire::Retries=1 || true
     ```
  4. Added explicit job-level `timeout-minutes: 5` (and 10/15 where appropriate) across all jobs in `.github/workflows/ci.yml`.
- **Key Lesson & Prevention Rule**:
  - Never allow unbounded network operations in CI scripts. Always configure short connect timeouts, command-level `timeout` wrappers, and job-level `timeout-minutes`.

---

### [ENTRY-006] Windows `select.select()` Pipe Incompatibility & Malformed File URIs in `clangd_check.py`
- **Date**: 2026-11-13
- **Component**: `.github/scripts/clangd_check.py`
- **Symptom**:
  ```text
  OSError: [WinError 10093] Either the application has not called WSAStartup, or WSAStartup failed
  ```
  Additionally, Clangd failed to resolve project headers with `'stdbool.h' file not found`.
- **Root Cause**:
  1. In Python on Windows, `select.select()` is implemented on Winsock and only accepts socket file descriptors; invoking it on pipe streams (`proc.stdout`) raises `OSError [WinError 10093]`.
  2. URI interpolation `f"file://{full_path}"` on Windows yields `file://C:\...`, which is an invalid URI with `C:` parsed as the network authority instead of a root local drive path, causing Clangd to fail compilation database lookups.
- **Fix Applied**:
  1. Replaced `select.select` with a daemon background thread (`stdout_reader`) pushing decoded JSON-RPC responses to a thread-safe `queue.Queue()`.
  2. Used RFC-compliant `pathlib.Path(p).as_uri()` for `rootUri` and document URIs (`file:///C:/...`).
- **Key Lesson & Prevention Rule**:
  - Never use `select.select()` on subprocess stdout/stderr pipes in scripts intended to run cross-platform (Windows & Linux). Use threading and `queue.Queue` with timeout polling.

---

### [ENTRY-007] `shlex.split` Stripping Path Backslashes on Windows in `compiler_check.py`
- **Date**: 2026-11-13
- **Component**: `.github/scripts/compiler_check.py`
- **Symptom**:
  ```text
  FileNotFoundError: [WinError 2] The system cannot find the file specified
  ```
- **Root Cause**:
  - In `compile_commands.json` generated on Windows, compilation commands contain backslash-delimited compiler paths (`C:\Espressif\tools\...\gcc.exe`).
  - By default, Python's `shlex.split(cmd_str)` operates in POSIX mode, where backslashes are treated as escape characters and stripped from Windows file paths (e.g. `C:\Espressif...` becomes `C:Espressif...`).
- **Fix Applied**:
  - On Windows (`os.name == 'nt'`), preserve the raw command string and replace `-o <file>` with `-o NUL -Werror=unused-function -Werror=unused-variable` via regex directly, delegating command-line parsing to Windows `CreateProcess` via `subprocess.run(new_cmd)`.
  - Normalized all file keys with `os.path.normpath(os.path.abspath(f)).lower()` to handle case-insensitivity and slash discrepancies between CMake entries and CLI paths.
- **Key Lesson & Prevention Rule**:
  - Never parse Windows command strings with POSIX `shlex.split()`. On Windows, invoke subprocesses with the modified command string directly or use `posix=(os.name != 'nt')`.
