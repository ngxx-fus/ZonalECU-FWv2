# FW-Common Subsystem

Shared cross-target resources and developer tools supporting both `FW-ESP` and `FW-STM`.

---

## 1. Directory Structure

```text
FW-Common/
├── conf/                # Shared cross-target configuration headers (FW-Config.h)
├── doc/                 # Master Doxygen documentation configuration & assets
├── drawio/              # System architecture & hardware block diagrams (SystemView.drawio)
├── scripts/             # Local developer automation & pre-commit tooling
│   ├── working_tree_update.ps1   # PowerShell local pre-commit check (Job 0 & 1)
│   └── working_tree_update.sh    # Bash local pre-commit check (Job 0 & 1)
└── readme.md            # Subsystem overview
```

---

## 2. Developer Scripts (`FW-Common/scripts/`)

Before committing changes, developers should run `working_tree_update` to auto-format code according to `CodingRules.md`, verify Git branch/commit conventions, and execute static analysis locally:

### 2.1 Windows Coworker Setup & Usage Guide

#### A. Prerequisites
1. **Python 3.10+**: Ensure Python is installed and added to `PATH` (can be installed via Microsoft Store or `winget install Python.Python.3.12`).
2. **clang-format**:
   - Option 1 (Recommended): Run `winget install LLVM.LLVM` in PowerShell, or
   - Option 2: `format_rules.py` and `working_tree_update.ps1` will automatically search ESP-IDF's bundled clang-format in `C:\Espressif\tools\esp-clang`.

#### B. Cách 1: Format nhanh bằng VS Code (Khuyên dùng - 1 Click / Phím tắt)
Dự án đã cấu hình sẵn `.vscode/tasks.json`:
- **Format file đang mở**: Mở file `.c` hoặc `.h` trong VS Code $\rightarrow$ Nhấn **`Ctrl + Shift + B`** (hoặc mở Command Palette `Ctrl+Shift+P` $\rightarrow$ chọn `Tasks: Run Task` $\rightarrow$ `ZECU: Format Active File`).
- **Format toàn bộ dự án**: Mở Command Palette $\rightarrow$ chọn `Tasks: Run Task` $\rightarrow$ `ZECU: Format Entire Project`.
- **Chạy kiểm tra tổng thể (Job 0 & Job 1)**: Chọn `Tasks: Run Task` $\rightarrow$ `ZECU: Run Working Tree Pre-Commit (Job 0 & 1)`.

#### C. Cách 2: Dùng PowerShell / Terminal
Mở PowerShell tại thư mục gốc repository:
- **Format 1 file cụ thể**:
  ```powershell
  python .github\scripts\format_rules.py FW-ESP\comp\Comm\ESP-Logger\SysLog.c
  ```
- **Chạy toàn bộ tiền kiểm tra Pre-Commit (Tự động format + Check AST Clangd + Git conventions)**:
  ```powershell
  .\FW-Common\scripts\working_tree_update.ps1
  ```
  *(Nếu PowerShell báo lỗi Execution Policy, chạy với cờ bypass: `powershell -ExecutionPolicy Bypass -File .\FW-Common\scripts\working_tree_update.ps1`)*

---

### 2.2 Linux / WSL / macOS Developer Guide

#### A. Prerequisites
1. **Python 3**:
   - Ubuntu / Debian: `sudo apt update && sudo apt install -y python3`
   - Arch Linux: `sudo pacman -S python`
2. **clang-format**:
   - Ubuntu / Debian: `sudo apt install -y clang-format`
   - Arch Linux: `sudo pacman -S clang`

#### B. Cách 1: Format nhanh bằng VS Code (Khuyên dùng - 1 Click / Phím tắt)
Dự án đã cấu hình sẵn `.vscode/tasks.json` tương thích hoàn toàn với Linux:
- **Format file đang mở**: Mở file `.c` hoặc `.h` trong VS Code $\rightarrow$ Nhấn **`Ctrl + Shift + B`** (hoặc `Ctrl+Shift+P` $\rightarrow$ chọn `Tasks: Run Task` $\rightarrow$ `ZECU: Format Active File`).
- **Format toàn bộ dự án**: Mở Command Palette `Ctrl+Shift+P` $\rightarrow$ chọn `Tasks: Run Task` $\rightarrow$ `ZECU: Format Entire Project`.
- **Chạy kiểm tra tổng thể (Job 0 & Job 1)**: Chọn `Tasks: Run Task` $\rightarrow$ `ZECU: Run Working Tree Pre-Commit (Job 0 & 1)`.

#### C. Cách 2: Dùng Terminal (Bash)
Tại thư mục gốc repository:
- **Format 1 file cụ thể**:
  ```bash
  python3 .github/scripts/format_rules.py FW-ESP/comp/Comm/ESP-Logger/SysLog.c
  ```
- **Chạy toàn bộ tiền kiểm tra Pre-Commit (Tự động format + Check AST Clangd + Git conventions)**:
  ```bash
  ./FW-Common/scripts/working_tree_update.sh
  ```
- **Chế độ kiểm tra nghiêm ngặt (không tự sửa file)**:
  ```bash
  ./FW-Common/scripts/working_tree_update.sh --check-only
  ```

### 2.3 Script Capabilities
- **Job 0.1 (Git Conventions)**: Validates branch naming (`ZE-<Feature/Improve/Update/Fix>-<Desc>`) and displays formatted commit templates.
- **Job 0.2 (Code Formatting)**: Automatically formats C/H files via `format_rules.py` and `clang-format`.
- **Job 1.1 (Clangd Static Analysis)**: Runs Clangd LSP JSON-RPC engine to verify AST and Clang-Tidy checks.
- **Job 1.2 (Compiler Strict Check)**: Runs the target cross-compiler with `-Werror=unused-function` and `-Werror=unused-variable`.
