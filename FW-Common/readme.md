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

### Windows (PowerShell)
```powershell
pwsh -File .\FW-Common\scripts\working_tree_update.ps1
```

### Linux / WSL / macOS (Bash)
```bash
./FW-Common/scripts/working_tree_update.sh
```

### Script Capabilities
- **Job 0.1 (Git Conventions)**: Validates branch naming (`ZE-<Feature/Improve/Update/Fix>-<Desc>`) and displays formatted commit templates.
- **Job 0.2 (Code Formatting)**: Automatically formats C/H files via `format_rules.py` and `clang-format`.
- **Job 1.1 (Clangd Static Analysis)**: Runs Clangd LSP JSON-RPC engine to verify AST and Clang-Tidy checks.
- **Job 1.2 (Compiler Strict Check)**: Runs the target cross-compiler with `-Werror=unused-function` and `-Werror=unused-variable`.
