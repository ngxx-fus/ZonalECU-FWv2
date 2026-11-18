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
| **008** | Formatting | Header guard & top-level `#include` indented by `BeforeHash` | `clang-format` fails to detect header guards preceded by comment banners, indenting file-level directives. | Keep `.clang-format` `IndentPPDirectives: None`; delegate scope-aware preprocessor indent to `format_rules.py`. |
| **009** | Architecture / 64-bit | Hardcoded 32-bit pointer mask truncating addresses on host test | Casting pointer to `uint32_t` strips upper 32 bits on 64-bit host architecture. | Use `uintptr_t` for pointer-integer conversions across host and target. |
| **010** | Formatting / Static Analysis | Function-like macros skipped & false-positive rule flags | Formatter only parsed object-like macros; rules flagged intentional vendor identifiers. | Update regex patterns and whitelist intentional architectural identifiers. |
| **011** | Formatting | Preprocessor heuristic flagged conditional macro as header guard | Top of source file conditionals misidentified as header include guard. | Ensure header guard checks strictly apply to `.h` files with matching file guard conventions. |
| **012** | Formatting / Architecture | Union member alignment omission & local variable shadowing | Formatter missed union scope; atomic macros declared colliding variable names. | Extend union member parsing and namespace macro-internal temporary variables with `__ze_`. |
| **013** | Formatting | PointerAlignment gluing `*const` in macro definitions | `clang-format` treats `*const` as right-aligned without spaces in macro expansions. | Apply custom post-formatting regex pass to enforce whitespace around `*const`. |
| **014** | Architecture / Build | Flash partitioning & macro namespace collision (`BIT`) | Generic single-word macros collided with ESP-IDF headers. | Guard generic macros with `#ifndef` and use explicit namespaces. |
| **015** | Formatting | Pointer typedef column gluing & array bracket alignment | `align_typedef_block` omitted spaces around `*`; union bracket alignment lacked lookahead. | Maintain 4 distinct columns for pointer typedefs and align array subscript brackets. |
| **016** | Storage / Architecture | Bare-metal flash storage without partition dependencies | High-level `esp_partition` clashed with static section placement requirements. | Place 64KB blocks into dedicated `.rodata.*` linker section and manipulate via ROM SPI flash. |
| **017** | RTOS / Concurrency | Spinlock busy-wait causing Task Watchdog starvation | `esp_rom_delay_us` busy-loop does not yield to IDLE task on priority >= 1 tasks. | Implement hybrid spin-delay mechanism yielding to RTOS ticks on longer waits. |
| **018** | Static Analysis / Macro | Duplicate 'const' specifier with `in` / `ro` macros | Macro `in` / `ro` expands to `const`; writing `in const void ...` causes duplicate `const` compiler warning. | Omit bare `const` when using `in` or `ro` attribute macros; `in` already declares const input parameter. |

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

---

### [ENTRY-008] Clang-Format `BeforeHash` Indenting Top-Level Header Guards & `#include`
- **Date**: 2026-11-13
- **Component**: `.clang-format` & `.github/scripts/format_rules.py`
- **Symptom**:
  - Top-level preprocessor directives in header files (guard `#define`, `#include <stdint.h>`, standalone `#define`) were erroneously indented by 4 spaces (`    #define __GUARD__`, `    #include`).
  - Preprocessor directives inside functions (e.g. `#if PARAM_CHECK` inside `void foo()`) were forced to column 0 by clang-format instead of aligning with the enclosing C code scope.
- **Root Cause**:
  1. `clang-format`'s include guard recognition heuristic only detects header guards when `#ifndef` is at the very beginning of the file. In ZonalECU-FWv2, header files have top Doxygen comment blocks (`/* @file ... */`) and section banners (`/* GUARD/BEGIN ... */`) before `#ifndef`. Consequently, `clang-format` treats `#ifndef __HEADER_H__` as a normal nested block and indents every subsequent preprocessor directive in the file by 4 spaces.
  2. `clang-format` always resets `#` to column 0 inside functions, ignoring enclosing C block indentation.
- **Fix Applied**:
  1. Set `IndentPPDirectives: None` in `.clang-format`.
  2. Implemented `format_preprocessor_indentation()` in `format_rules.py`:
     - Accurately detects header guards regardless of preceding comments, keeping `#ifndef __..._H__`, `#define __..._H__`, `#include`, and top-level `#define` at column 0.
     - Indents conditional blocks (`#ifndef ZE_ASSERT_EXPR`) so inner macros receive 4 spaces per nesting level.
     - Detects C scope brace depth and aligns `#if`/`#endif` with the surrounding function statements.
- **Key Lesson & Prevention Rule**:
  - Do not use `IndentPPDirectives: BeforeHash` when headers contain pre-guard comment banners. Keep clang-format preprocessor indentation disabled (`None`) and let the dedicated AST/regex engine in `format_rules.py` apply scope-aware preprocessor indentation.

---

### [2026-11-14] Struct Column Alignment Collapse & Pointer Qualifier Gluing (`*RO`)
- **Component**: `.github/scripts/format_rules.py` & `.clang-format`
- **Symptom**:
  - Struct member declarations with custom qualifiers (`RW`, `RO`, `ro_ptr`, etc.) had their column alignments collapsed by formatting.
  - Pointer declarations with post-asterisk qualifiers (`RW void * RO p_ext;`) were glued into `RW void *RO p_ext;` because `clang-format` treated `RO` as an identifier attached to `*`.
- **Root Cause**:
  - `clang-format` has `AlignConsecutiveDeclarations: None` and does not recognize custom permission macros (`RO`, `WO`, `RW`, `ro_ptr`, etc.) as type qualifiers, attaching `*` to the qualifier token and flattening inter-column whitespace.
  - `format_rules.py` lacked a post-processing pass for struct member definitions.
- **Fix Applied**:
  - Implemented `align_struct_block()` and `align_all_structs()` in `format_rules.py`:
    1. Un-glues any `*<QUALIFIER>` pattern (e.g. `*RO` -> `* RO`).
    2. Parses struct fields into `[prefix_qual] [type] [ptr_qual] [name;] [comment]` columns.
    3. Aligns types, qualifiers, and member names to uniform columns across the struct block.
    4. Integrated `align_all_structs()` into the post-processing pipeline in `format_file()`.
- **Key Lesson & Prevention Rule**:
  - C code formatters cannot natively parse domain-specific attribute/permission macros without custom AST or post-processing passes. Keep `align_all_structs()` in `format_rules.py` to guarantee consistent struct layout.

---

### [2026-11-14] Pointer Auto-Renaming Missed Macro-Based Pointer Declarations (`ptr_*`)
- **Component**: `.github/scripts/format_rules.py`, `FW-ESP/readme.md`, `FW-ESP/CodingRules.md`
- **Symptom**:
  - Variables, parameters, and struct members declared using macro-based pointer annotations (such as `ze_flash_cfg_t ro_ptr cfg;` or `in void ro_ptr data`) were not recognized as pointers and failed to receive the mandatory `p_` prefix.
- **Root Cause**:
  - `PTR_DECL_PATTERN` and `decl_pattern` in `format_rules.py` strictly searched for literal asterisks (`*`), ignoring domain-specific pointer permission macros defined in `Macros.h`.
- **Fix Applied**:
  - Defined `PTR_QUALIFIERS_REGEX` covering all 12 pointer macro variants: `ro_ptr`, `rw_ptr`, `wo_ptr`, `RO_PTR`, `RW_PTR`, `WO_PTR`, `__ro_ptr`, `__rw_ptr`, `__wo_ptr`, `__RO_PTR`, `__RW_PTR`, `__WO_PTR`.
  - Expanded `PTR_DECL_PATTERN` and `decl_pattern` to detect identifiers preceded by either `*` or any `ptr_*` macro.
  - Added negative lookbehind `(?<!\()` to ensure function pointer declarations (e.g. `(*FlashOpen)`) are not misidentified as data pointer variables.
  - Documented explicit pointer naming enforcement for `ptr_*` macros in `FW-ESP/readme.md` and `FW-ESP/CodingRules.md`.
- **Key Lesson & Prevention Rule**:
  - When abstracting native C syntax (such as pointer dereference/declaration `*`) into custom architectural macros, all linters, formatting tools, and naming scanners across the repository must be updated synchronously to treat those macros as direct semantic equivalents of raw pointers.

---

### [2026-11-14] Function Prototype Alignment & Multiline Parameter Backslash Layout
- **Component**: `.github/scripts/format_rules.py`, `FW-ESP/CodingRules.md`, `FW-ESP/readme.md`
- **Symptom**:
  - Function prototypes in header files (`/* SHARED FUNCTIONS */`), callback pointers in API structs, and function definitions in `.c` files lacked vertical alignment for opening parentheses and suffered from inconsistent multiline parameter layout.
- **Root Cause**:
  - `clang-format` flattens or arbitrarily indents function parameter lists and does not recognize project-specific 4-column parameter conventions (`[dir_qual] [type] [ptr_qual] [param_name]`) or continuation backslash (`\`) margins.
- **Fix Applied**:
  - Implemented `align_all_function_declarations()` in `format_rules.py`:
    1. Detects contiguous blocks of function declarations and aligns return types and names so opening parentheses `(` line up vertically.
    2. Enforces the single-line rule for declarations under 130 columns ($< 130$).
    3. For declarations reaching or exceeding 130 columns ($\ge 130$), automatically splits parameters onto new lines, formats them into 4 aligned columns, and appends right-aligned continuation backslashes (`\`) at column 130 (`BANNER_WIDTH`).
    4. Formally documented this convention as Section 3.4 in `FW-ESP/CodingRules.md` and added summary in `FW-ESP/readme.md`.
- **Key Lesson & Prevention Rule**:
  - High-density embedded API tables and prototype declarations benefit significantly from deterministic column alignment. Coupling clang-format AST formatting with a custom Python post-processing pass ensures strict adherence to project layout conventions without sacrificing standard formatting benefits.

---

### [2026-11-14] Struct Designated Initializer Alignment & Single-Line Unpacking
- **Component**: `.github/scripts/format_rules.py`, `FW-ESP/comp/Comm/ESP-Flash/Flash.c`
- **Symptom**:
  - Struct instances initialized using designated initializers (e.g. `ze_flash_instance_t g_ze_flash_ctrl = { ... };` or `prv_ze_flash_info`) were either collapsed by `clang-format` into arbitrary single-line/multi-line blocks without alignment of `=` signs, or left with irregular member indentation and missing trailing commas.
- **Root Cause**:
  - Standard `clang-format` configuration (`ColumnLimit: 130`) automatically packs small designated initializer structs onto single lines (`{.field = val, ...}`). Furthermore, clang-format has no native rule to vertically align `=` across designated initializer fields (`.field = value`).
- **Fix Applied**:
  - Implemented `align_designated_initializer_block()` and `align_all_designated_initializers()` in `format_rules.py`:
    1. Detects both collapsed single-line initializers and multiline struct initializer blocks.
    2. Extracts all fields enclosed within `{ ... }` honoring nested braces, brackets, and string literals.
    3. Normalizes structure: `<indent><decl> = {`, 4-space indented fields (`<indent>    .<field> = <value>,`), and closing `<indent>};`.
    4. Calculates maximum field name length across the block and vertically pads field identifiers so all `=` signs align at the exact same column.
    5. Guarantees trailing commas on all member initializers for clean git diffs and aligns inline trailing comments.
    6. Verified on `FW-ESP/comp/Comm/ESP-Flash/Flash.c` and validated with clean ESP-IDF v6.1 builds.
- **Key Lesson & Prevention Rule**:
  - Embedded systems rely heavily on clear static configuration structs and instance definitions. Enforcing designated initializer alignment in `format_rules.py` guarantees uniform readability and clean version control diffs regardless of how clang-format attempts to pack lines.

---

### [ENTRY-009] Hardcoded 32-bit Pointer Mask Truncating Addresses on 64-bit Host Test Runner
- **Date**: 2026-11-17
- **Component**: `FW-ESP/comp/Comm/ESP-Mem/Mem.c` (`WORD_BASE` macro)
- **Symptom**:
  - Executing Unity unit tests on x64 host (`make` in `ESP-Mem/.test`) caused immediate `SIGSEGV (Segmentation fault)` inside `prv_copy_to_aligned_dst` when accessing `p_src_word` or `p_dst_word`.
- **Root Cause**:
  - `WORD_BASE(addr)` was defined as `AND(UINTPTR(addr), 0xFFFFFFFC)`.
  - On a 64-bit host (x86_64), `0xFFFFFFFC` is a 32-bit `unsigned int` constant. When bitwise-ANDed with a 64-bit `uintptr_t` address (`0x55555555e990`), the upper 32 bits were truncated to zero, generating invalid addresses (e.g. `0x00000000...`) that faulted upon dereference.
- **Fix Applied**:
  - Redefined `WORD_BASE(addr)` in `Mem.c` to use bitwise inversion of a typed pointer-width integer:
    ```c
    #define WORD_BASE(addr) AND(UINTPTR(addr), ~((uintptr_t)3U))
    ```
  - This dynamically expands to `0xFFFFFFFC` on 32-bit MCUs (ESP32 / STM32) and `0xFFFFFFFFFFFFFFFC` on 64-bit test hosts, fully preserving full pointer width.
- **Key Lesson & Prevention Rule**:
  - Never hardcode 32-bit hex literals (e.g. `0xFFFFFFFC`) for pointer alignment masking. Always use bitwise inversion on `uintptr_t` (e.g. `~((uintptr_t)offset_mask)`) to guarantee seamless host-test portability.

---

### [ENTRY-010] Function-Like Macros Skipped by Formatter & False-Positive Rule Flags on Intentional Identifiers
- **Date**: 2026-11-17
- **Component**: `.github/scripts/format_rules.py`, `.github/scripts/clangd_check.py`
- **Symptom**:
  - Function-like / parameterized macros with arguments (e.g. `#define ZE_ATOMIC_INCR(p_atomic) ...` or `#define ze_atomic(type) ...`) were not aligned into neat vertical columns by `format_rules.py` when separated or formatted, leaving replacement values jagged.
  - Intentional custom identifier conventions (e.g. lowercase macro `#define ze_atomic(type) _Atomic type` serving as a type keyword) had no mechanism to bypass naming convention enforcement or static analysis diagnostics.
- **Root Cause**:
  - In `align_macro_definitions_in_section`, `single_define_pattern` matched only plain macro identifiers `([A-Za-z0-9_]+)`, completely ignoring function-like macros with argument lists `(...)`.
  - Tooling lacked an exception annotation convention to suppress automated renaming or static analysis failure on intentional non-standard constructs.
- **Fix Applied**:
  - Updated `single_define_pattern` regex in `format_rules.py` to match both object-like and function-like macros:
    ```python
    r"^(\s*#\s*define\s+)([A-Za-z0-9_]+(?:\([^)]*\))?)(\s+)(.+?)(?:\s*(/\*.*?\*/|//.*))?$"
    ```
  - Implemented the `/*ZE_EXCEPTION(...)*/` mechanism:
    - `preindex_declarations`: Excludes declarations on lines annotated with `/*ZE_EXCEPTION(...)*/`.
    - `apply_naming_transformations`: Skips detection and renaming on lines with `/*ZE_EXCEPTION(...)*/`.
    - `clangd_check.py`: Filters out diagnostics raised on lines marked with `/*ZE_EXCEPTION(...)*/`.
    - Added `"unitytest"` to `EXCLUDE_DIRS` to isolate external unit testing libraries from formatting checks.
  - Implemented `align_typedef_block` and `align_all_typedefs` in `format_rules.py` to automatically align contiguous single-line `typedef` blocks into 4 straight columns (`typedef` + modifier/macro + type/args + alias name) or 3 columns (simple types and pointer typedefs) so clang-format does not collapse vertical spacing.
- **Key Lesson & Prevention Rule**:
  - Preprocessor formatters must accommodate C function-like macros and variadic macros. Codebase quality tools should always provide a standardized inline exception comment (e.g. `/*ZE_EXCEPTION(...)*/`) to handle domain-specific architectural exceptions gracefully without failing automated CI gates.
  - Clang-format collapses spaces in typedef declarations (`AlignConsecutiveDeclarations` does not cover macros/modifiers). Dedicated AST/regex post-processing in `format_rules.py` guarantees consistent 4-column alignment for typedefs.

---

### [ENTRY-011] Preprocessor Heuristic Falsely Flagged Conditional Macro as Header Guard in Source File
- **Date**: 2026-11-17
- **Component**: `.github/scripts/format_rules.py` (`format_preprocessor_indentation`)
- **Symptom**:
  - In `FW-ESP/comp/Comm/ESP-Logger/SysLog.c`, `#define CPU_YIELD() /* Place holder */` nested inside `#ifndef CPU_YIELD` was incorrectly held at column 0 instead of being indented by 4 spaces (`level 1`), while subsequent blocks like `#ifndef CPU_RESET_WDT` had their inner macros correctly indented.
- **Root Cause**:
  - `format_preprocessor_indentation` scanned the first 35 lines for an initial `#ifndef MACRO` followed by `#define MACRO` to detect file-level header guards.
  - Because `SysLog.c` defined `#ifndef CPU_YIELD` followed by `#define CPU_YIELD()` on lines 17-18, the script misidentified `CPU_YIELD` as the file header guard. Header guard guards are deliberately kept at column 0.
- **Fix Applied**:
  - Restricted header guard detection exclusively to header files (`file_name.endswith((".h", ".hpp", ".H"))`).
  - Required that header guard `#define` directives must be strictly parameterless and empty (only whitespace and optional comments: `^#\s*define\s+[A-Za-z0-9_]+\s*(?:/\*.*?\*/|//.*)?$`).
  - Passed `file_name` to `format_preprocessor_indentation` in `format_file`.
- **Key Lesson & Prevention Rule**:
  - Header guards only exist in header files (`.h`), never in implementation source files (`.c`). Preprocessor formatting heuristics must enforce strict file-extension boundaries and syntax checks (no macro arguments or values) to prevent conditional definitions from being mistaken for file guards.

---

### [ENTRY-012] Union Member Alignment Omission in Formatter & Local Variable Shadowing in Atomic Operations
- **Date**: 2026-11-17
- **Component**: `.github/scripts/format_rules.py`, `FW-ESP/comp/Comm/ESP-Logger/SysLog.c`, `FW-ESP/comp/Common/Atomic/Atomic.h`
- **Symptom**:
  - In `FW-ESP/comp/Common/Base/Types.h`, member fields of `ze_dword_un`, `ze_word_un`, and `ze_hword_un` were collapsed to a single space by clang-format without column alignment.
  - In `FW-ESP/comp/Comm/ESP-Logger/SysLog.c:91`, static analysis failed with `[clang/atomic_op_needs_atomic]: Address argument to atomic operation must be a pointer to _Atomic type ('uint32_t *' (aka 'unsigned int *') invalid)`.
- **Root Cause**:
  1. `align_all_structs` in `format_rules.py` only scanned for `struct` definitions, completely omitting `union` definitions (`typedef union ... {`).
  2. In `ZE_SysLog_Init()`, `uint32_t prv_flag = ZE_ATOMIC_GET(&prv_flag);` declared a local variable named `prv_flag`. Under C scope rules (ISO C99/C11 §6.2.1), the local identifier's scope begins immediately after its declarator, thereby shadowing the file-scope `static ze_atomic(uint32_t) prv_flag;`. Consequently, `&prv_flag` in the initializer took the address of the uninitialized local `uint32_t` (`uint32_t *`), which is non-atomic and violated `atomic_load` requirements.
  3. `ZE_ATOMIC_GET(p_atomic)` in `Atomic.h` lacked surrounding parentheses `((p_atomic))` unlike the other atomic helper macros.
- **Fix Applied**:
  1. Extended `align_all_structs` regex to match both `struct` and `union` (`(?:struct|union)`), restoring column alignment across all union definitions in `Types.h`.
  2. Renamed the local variable in `ZE_SysLog_Init()` from `prv_flag` to `cur_flag` to avoid identifier shadowing.
  3. Added parameter enclosing parentheses `atomic_load((p_atomic))` in `Atomic.h`.
- **Key Lesson & Prevention Rule**:
  - Struct alignment utilities must treat `struct` and `union` uniformly since both follow identical field declaration syntax.
  - Never use the same identifier for a local variable and a file-scope/static variable in an initialization expression (`T x = FUNC(&x)`). The local declaration shadows the outer scope in its own initializer, resulting in unintentional self-referencing.

---

### [ENTRY-013] Clang-Format PointerAlignment Gluing `*const` in Preprocessor Macro Definitions
- **Date**: 2026-11-17
- **Component**: `.github/scripts/format_rules.py`, `FW-ESP/comp/Common/Base/Macros.h`
- **Symptom**:
  - In `FW-ESP/comp/Common/Base/Macros.h`, pointer permission macros (`RO_PTR`, `ro_ptr`, `__RO_PTR`, `__ro_ptr`) had their replacement tokens glued together as `*const` instead of `* const`. Any manual addition of whitespace was automatically reverted to `*const` on running `format_rules.py`.
- **Root Cause**:
  - `.clang-format` configures `PointerAlignment: Right`. During clang-format invocation, pointer asterisks bind to the identifier/keyword immediately to their right when no left-hand type is present in the macro value, producing `*const`.
  - While `align_struct_block()` had previously implemented a regex un-gluing pass for struct member declarations, `align_macro_definitions_in_section()` did not perform qualifier detachment on macro replacement values.
- **Fix Applied**:
  - Added regex un-gluing in `align_macro_definitions_in_section()` in `.github/scripts/format_rules.py`:
    ```python
    val = re.sub(r"\*(const|RO|WO|RW|ro|wo|rw)\b", r"* \1", val)
    ```
  - Re-formatted `Macros.h`: all 4 macro variants (`RO_PTR`, `ro_ptr`, `__RO_PTR`, `__ro_ptr`) now preserve `* const`, and all inline trailing comments across permission macro blocks are vertically aligned at the same column.
- **Key Lesson & Prevention Rule**:
  - `clang-format`'s `PointerAlignment: Right` aggressively glues asterisks to right-hand qualifiers in preprocessor definitions. Post-processing macro alignment routines must actively decouple `*` from qualifiers (`const`, `RO`, `rw`, etc.) to enforce project readability guidelines.

---

### [ENTRY-014] Dedicated 64KB Flash Partitioning, Top-Level Preprocessor Indentation & Macro Collisions
- **Date**: 2026-11-17
- **Component**: `.github/scripts/format_rules.py`, `FW-ESP/partitions.csv`, `FW-ESP/comp/Comm/ESP-Flash/`, `FW-ESP/comp/Common/Base/Macros.h`
- **Symptom**:
  1. Top-level variable/function declarations within `#if ... #else ... #endif` blocks had their indentation stripped to column 0 by clang-format.
  2. Multi-line struct initializers with inline `/* ... */` comments were mangled by `align_designated_initializer_block()` because it only parsed `//` comments.
  3. Building firmware failed due to `#define BIT(x) (x)` redefinition error colliding with ESP-IDF's `<esp_bit_defs.h>`.
  4. ESP-Flash had no dedicated hardware partition and had syntax errors due to simultaneous initialization of union members in `ze_fblock_t`.
- **Root Cause**:
  1. In `format_preprocessor_indentation()`, `effective_pp_indent` was only applied if `brace_depth > 0`, ignoring top-level code.
  2. `align_designated_initializer_block()` lacked regex detection for C-style `/* ... */` comments.
  3. `Macros.h` defined generic unit macros (`BIT`, `BYTE`, etc.) unconditionally without `#ifndef`.
  4. ESP32 SPI flash cannot be written directly as RAM; writable variables must be buffered in RAM/PSRAM, and flash operations must be addressed via a dedicated partition.
- **Fix Applied**:
  1. Updated `format_preprocessor_indentation()` to indent top-level code by `effective_pp_indent` when inside conditional directives.
  2. Enhanced `align_designated_initializer_block()` to extract both `//` and `/* ... */` comments cleanly.
  3. Protected `BIT`, `BYTE`, `KILO`, `MEGA`, etc. with `#ifndef` guards in `Macros.h`.
  4. Created custom partition table `FW-ESP/partitions.csv` with a dedicated 64KB raw data partition `storage` at offset `0x110000`, enabled in `sdkconfig`.
  5. Implemented complete Flash driver in `Flash.h` and `Flash.c` using `esp_partition_find_first`, `esp_partition_read`, `esp_partition_erase_range`, and `esp_partition_write`, with PSRAM/DRAM cache buffering. Successfully compiled and linked firmware binary.
- **Key Lesson & Prevention Rule**:
  - Always guard generic single-word macros (e.g. `BIT`) with `#ifndef` to prevent clashes with vendor SDK headers.
  - Dedicated storage on ESP32 must always be backed by an explicit entry in `partitions.csv` and manipulated through the partition abstraction API.

---

### [ENTRY-015] Formatter Pointer Typedef Column Gluing & Struct/Union Array Bracket Alignment
- **Date**: 2026-11-17
- **Component**: `.github/scripts/format_rules.py`, `FW-ESP/comp/Common/Base/Types.h`
- **Symptom**:
  1. In `Types.h`, pointer typedefs (`typedef uint64_t *uint64ptr_t;`, etc.) had asterisks `*` glued directly to alias names instead of a dedicated space-padded column (`* uint64ptr_t;`). Blank lines between typedef sub-groups caused column widths to fragment.
  2. In union member definitions (`ze_hword_un`, `ze_word_un`, `ze_dword_un`), array subscript brackets (`[WORD2HWORD]`, `[DWORD2WORD]`) were unaligned across fields, and any spaces preceding `[` caused `STRUCT_MEMBER_PATTERN` match failures.
- **Root Cause**:
  1. `align_typedef_block()` constructed code lines using `*{p[4]}` rather than `* {p[4]}`. `align_all_typedefs()` lacked lookahead to span single blank lines between related typedef groups.
  2. `STRUCT_MEMBER_PATTERN` used `(?:\[[^\]]*\])?` with no whitespace tolerance before `[`, causing declarations like `half_st  [WORD2HWORD]` to be treated as raw unformatted lines.
- **Fix Applied**:
  1. Updated `align_typedef_block()` to maintain 4 distinct columns: `typedef [type] * [name;]` with spaces flanking both sides of `*`.
  2. Added lookahead logic in `align_all_typedefs()` to preserve unified column alignment across single blank lines between typedef groups.
  3. Separated identifier and array dimension in `STRUCT_MEMBER_PATTERN` (`([a-zA-Z_][a-zA-Z0-9_]*)(?:\s*(\[[^\]]*\]))?`).
  4. Extended `align_struct_block()` to vertically align `[` brackets across all array members within structs/unions.
- **Key Lesson & Prevention Rule**:
  - Embedded C type definitions and data structure tables require deterministic column alignment for both pointer asterisks and array brackets. Formatting tools must explicitly decouple token parsing from whitespace layout to guarantee clean tabular structure across blocks.

---

### [ENTRY-016] Low-Level Bare-Metal Flash Storage Architecture & 64KB Linker Section Placement
- **Date**: 2026-11-18
- **Component**: `FW-ESP/comp/Comm/ESP-Flash` (`Flash.h`, `Flash.c`, `CMakeLists.txt`)
- **Symptom**:
  - Higher-level partition table abstraction (`esp_partition`) was initially used, requiring partition CSV declarations and RAM buffer allocation, conflicting with bare-metal flash requirements where dedicated 64KB flash space is directly reserved in flash memory without partition dependencies.
- **Root Cause**:
  - Architectural mismatch between high-level vendor partition frameworks (`esp_partition_*`) and bare-metal embedded storage design where flash blocks are statically placed in DROM linker sections (`.rodata.ze_flash`) and manipulated via low-level ROM SPI Flash APIs.
- **Fix Applied**:
  1. Replaced `esp_partition` with low-level `spi_flash` and `esp_rom` components (`esp_rom_spiflash.h`, `spi_flash_mmap.h`).
  2. Implemented dedicated 64KB block placeholder `g_ze_flash_block` in section `.rodata.ze_flash` with `used` attribute to guarantee the linker retains the 64KB allocation at compile time without being stripped.
  3. Defined `ZE_PLACE_IN_FLASH` as `__attribute__((section(".rodata.ze_flash"), used, aligned(4)))` allowing user variables to be statically placed contiguously in the 64KB flash block.
  4. Implemented strict 64KB boundary checks (`prv_is_valid_range`) rejecting any out-of-bounds access with `ZE_ERR_INVALID_SIZE`.
  5. Built 128-byte line cache `s_cache` (`ze_flash_cache_t`) to accelerate reads and stage unaligned accesses.
  6. Implemented low-level write mechanism with sector erase (`esp_rom_spiflash_erase_sector`) and word-level bitwise mask merging for unaligned head/tail bytes, matching `Mem.c`.
- **Key Lesson & Prevention Rule**:
  - When designing bare-metal flash storage subsystems without partition table dependencies, place reserved flash blocks directly into dedicated `.rodata.*` linker sections and resolve physical addresses via `spi_flash_cache2phys()`. SPI Flash hardware requires 4-byte aligned writes and 4KB sector erasures; unaligned accesses must be staged through a word-merging algorithm.

---

### [ENTRY-017] Task Watchdog Starvation in Spinlocks & ESP32 Hybrid Delay Architecture in ESP-Mutex
- **Date**: 2026-11-18
- **Component**: `FW-ESP/comp/Comm/ESP-Mutex`, `FW-ESP/comp/Common/Mutex`
- **Symptom**:
  - Standard atomic spinlocks relying solely on `esp_rom_delay_us` and `taskYIELD()` cause Task Watchdog Timer (TWDT) timeout resets (panics) when a high-priority task spins on a lock for extended periods, because `taskYIELD()` does not yield to lower-priority tasks, starving the FreeRTOS `IDLE` task on the core.
- **Root Cause**:
  - `esp_rom_delay_us` is a CPU busy-wait loop that burns cycles. In FreeRTOS SMP on ESP32, IDLE tasks feed the Task Watchdog; starving IDLE tasks causes TWDT to trigger after the timeout period (5 seconds).
- **Fix Applied**:
  1. Implemented a dual-layer architecture: multi-target atomic spinlock in `comp/Common/Mutex/` with configurable placeholder hooks (`ZE_DISABLE_IRQ`, `ZE_ENABLE_IRQ`, `ZE_YIELD`, `ZE_WATCH_DOG_RESET`, `CPU_DELAY_US`).
  2. Created hardware-adapted wrapper `comp/Comm/ESP-Mutex` implementing a hybrid delay algorithm:
     - For short durations ($< 1\text{ ms}$): uses microsecond delay (`esp_rom_delay_us(100)`) and `taskYIELD()` for low latency.
     - For longer waits ($\ge 1\text{ ms}$): yields CPU via `vTaskDelay(pdMS_TO_TICKS(1))` and explicitly invokes `esp_task_wdt_reset()`, ensuring IDLE tasks get CPU time to feed the TWDT.
  3. Formatted and verified with `format_rules.py` and `clangd_check.py` (0 errors), compiling into `libCommon.a` and `libESP-Mutex.a`.
- **Key Lesson & Prevention Rule**:
  - In RTOS environments on multicore microcontrollers, never use infinite busy-wait spinlocks without yielding to lower-priority tasks. Implement hybrid spin-delay mechanisms where long waits drop back to RTOS tick delays to feed watchdog timers.

---

### [ENTRY-018] Duplicate 'const' Declaration Specifier When Combining `in` / `ro` with `const`
- **Date**: 2026-11-18
- **Component**: `FW-ESP/comp/Common/Base/Macros.h`, `FW-ESP/comp/Comm/ESP-Flash/Flash.h`
- **Symptom**:
  - Clangd emitted `-Wduplicate-decl-specifier` (`Duplicate 'const' declaration specifier`) on function prototypes:
    `ze_err_t ZE_Flash_Read(out void rw_ptr p_dest, in const void ro_ptr p_flash_src, in uint32_t size);`
- **Root Cause**:
  - In `comp/Common/Base/Macros.h`, attribute macros `IN` and `in` are defined as `const`. Writing `in const void ro_ptr` caused the preprocessor to expand to `const const void *const`, which triggers C11 duplicate type specifier warnings.
- **Fix Applied**:
  - Simplified parameter declarations to `in void ro_ptr p_src`, removing the redundant explicit `const`.
- **Key Lesson & Prevention Rule**:
  - When using Renesas RA / ZECU parameter direction macros (`in`, `ro`, `IN`, `RO`), never prepend or append raw `const` to the type; the direction macro itself already provides the `const` qualifier.







