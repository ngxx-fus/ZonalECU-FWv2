<#
.SYNOPSIS
    Local Working Tree Update & Static Check Runner (Job 0 & Job 1) for ZonalECU-FWv2.

.DESCRIPTION
    Port of GitHub Actions Job 0 (Conventions & Formatting) and Job 1 (Clangd Static Analysis &
    Strict Compiler Checks) to run locally in PowerShell before committing code.

.PARAMETER CheckOnly
    Run in strict verification mode. If code is reformatted, reports failure instead of leaving
    formatted changes for staging.

.PARAMETER SkipFormat
    Skip code formatting checks.

.PARAMETER SkipGitConventions
    Skip Git branch and commit message checks.

.PARAMETER SkipStaticAnalysis
    Skip Job 1 Clangd LSP and target compiler static analysis.

.PARAMETER Files
    Optional list of specific files to run static analysis on. Defaults to all project C files.

.EXAMPLE
    .\FW-Common\scripts\working_tree_update.ps1
    .\FW-Common\scripts\working_tree_update.ps1 -CheckOnly
    .\FW-Common\scripts\working_tree_update.ps1 -SkipGitConventions
#>

[CmdletBinding()]
param(
    [switch]$CheckOnly,
    [switch]$SkipFormat,
    [switch]$SkipGitConventions,
    [switch]$SkipStaticAnalysis,
    [string[]]$Files
)

$ErrorActionPreference = "Stop"

# ------------------------------------------------------------------------------
# 0. Locate Project Root & Setup Environment
# ------------------------------------------------------------------------------
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = (Resolve-Path (Join-Path $ScriptDir "..\..")).Path
Set-Location $ProjectRoot

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host " ZonalECU-FWv2 Local Pre-Commit Check (Job 0 & 1)" -ForegroundColor Cyan
Write-Host " Project Root : $ProjectRoot" -ForegroundColor DarkGray
Write-Host " Execution Time: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor Cyan

# ------------------------------------------------------------------------------
# Tool Discovery & PATH Auto-Configuration
# ------------------------------------------------------------------------------
function Resolve-ToolPath {
    param([string]$CommandName, [string[]]$SearchRoots)
    $cmd = Get-Command $CommandName -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }
    foreach ($root in $SearchRoots) {
        if (Test-Path $root) {
            $found = Get-ChildItem -Path $root -Filter "$CommandName.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($found) {
                $binDir = Split-Path -Parent $found.FullName
                if ($env:PATH -notlike "*$binDir*") {
                    $env:PATH = "$binDir;$env:PATH"
                }
                return $found.FullName
            }
        }
    }
    return $null
}

# 1. Python
$PythonExe = (Get-Command python -ErrorAction SilentlyContinue).Source
if (-not $PythonExe) {
    $PythonExe = Resolve-ToolPath "python" @("C:\Espressif\tools\python", "$env:LOCALAPPDATA\Programs\Python")
}
if (-not $PythonExe) {
    Write-Host "❌ ERROR: Python executable not found in PATH or standard locations." -ForegroundColor Red
    exit 1
}

# 2. Clang-format
$ClangFormatExe = Resolve-ToolPath "clang-format" @(
    "C:\Espressif\tools\esp-clang",
    "C:\Program Files\LLVM"
)

# 3. Clangd
$ClangdExe = Resolve-ToolPath "clangd" @(
    "C:\Espressif\tools\esp-clangd",
    "C:\Program Files\LLVM"
)

# 4. Xtensa Cross-Compiler Toolchain
$XtensaGcc = Resolve-ToolPath "xtensa-esp32-elf-gcc" @(
    "C:\Espressif\tools\xtensa-esp-elf"
)

Write-Host "`n[Toolchain Environment]" -ForegroundColor Yellow
Write-Host "  Python       : $PythonExe"
Write-Host "  clang-format : $(if ($ClangFormatExe) { $ClangFormatExe } else { 'Not Found (Formatting will skip AST format)' })"
Write-Host "  clangd       : $(if ($ClangdExe) { $ClangdExe } else { 'Not Found' })"
Write-Host "  xtensa-gcc   : $(if ($XtensaGcc) { $XtensaGcc } else { 'Not Found' })"

$OverallSuccess = $true
$Job0Status = "PASSED"
$Job1Status = "PASSED"

# ------------------------------------------------------------------------------
# JOB 0: Conventions & Formatting
# ------------------------------------------------------------------------------
Write-Host "`n==================================================" -ForegroundColor Cyan
Write-Host " Job 0: Conventions & Formatting" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# --- Step 0.1: Git Branch Convention Check ---
if (-not $SkipGitConventions) {
    Write-Host "`n>>> [Job 0.1] Checking Git Branch & Commit Conventions..." -ForegroundColor Yellow
    $CurrentBranch = git rev-parse --abbrev-ref HEAD 2>$null
    Write-Host "Current Branch: $CurrentBranch"

    $BranchRegex = "^ZE-(Feature|Improve|Update|Fix)-[A-Za-z0-9._-]+$"
    $MainlineRegex = "^(ZE-Master|FWv2-Master|main|master)$"

    if ($CurrentBranch -match $MainlineRegex) {
        Write-Host "  [OK] Protected/mainline branch ($CurrentBranch), skipping branch name check." -ForegroundColor Green
    } elseif ($CurrentBranch -match $BranchRegex) {
        Write-Host "  [OK] Branch name complies with convention: $CurrentBranch" -ForegroundColor Green
    } else {
        Write-Host "  [WARN] Branch name '$CurrentBranch' does not follow convention:" -ForegroundColor Yellow
        Write-Host "         Expected : ZE-<Feature/Improve/Update/Fix>-<ShortDescription>" -ForegroundColor Yellow
        Write-Host "         Example  : ZE-Feature-UpdateWorkingTree" -ForegroundColor Yellow
    }

    # --- Step 0.2: Check Latest Commit Format ---
    $LatestMsg = git log -1 --format="%s" 2>$null
    $CommitRegex = "^(ZE|ZonalECU|ZonalECU-FWv2) \| [0-9]{2}[/-][0-9]{2}[/-][0-9]{2,4} [0-9]{2}:[0-9]{2}:[0-9]{2} \| .+$"
    $MergeRegex = "^Merge .+$"

    if ($LatestMsg) {
        if ($LatestMsg -match $CommitRegex -or $LatestMsg -match $MergeRegex) {
            Write-Host "  [OK] Latest commit format complies: $LatestMsg" -ForegroundColor Green
        } else {
            Write-Host "  [WARN] Latest commit did not match convention format:" -ForegroundColor Yellow
            Write-Host "         Commit msg: `"$LatestMsg`"" -ForegroundColor Yellow
        }
    }

    # Display ready-to-use template for next commit
    $CommitDateStr = Get-Date -Format "MM/dd/yy HH:mm:ss"
    Write-Host "`n  [Commit Message Template for next commit]:" -ForegroundColor Magenta
    Write-Host "  git commit -m `"ZonalECU-FWv2 | $CommitDateStr | <Your message here>`"`n" -ForegroundColor Magenta
}

# --- Step 0.3: Code Formatting Check (format_rules.py) ---
if (-not $SkipFormat) {
    Write-Host ">>> [Job 0.2] Running Code Formatting Rules (format_rules.py)..." -ForegroundColor Yellow
    $FormatScript = Join-Path $ProjectRoot ".github\scripts\format_rules.py"

    if (Test-Path $FormatScript) {
        $statusBefore = @(git status --porcelain -- FW-Common FW-ESP FW-STM 2>$null)
        & $PythonExe $FormatScript FW-Common FW-ESP FW-STM
        $statusAfter = @(git status --porcelain -- FW-Common FW-ESP FW-STM 2>$null)

        # Detect files specifically modified during this formatting run
        $newDiff = Compare-Object -ReferenceObject $statusBefore -DifferenceObject $statusAfter |
            Where-Object { $_.SideIndicator -eq "=>" } |
            ForEach-Object { $_.InputObject.Substring(3).Trim() }

        if ($newDiff) {
            Write-Host "`n⚠️ Code formatting modifications detected in:" -ForegroundColor Yellow
            $newDiff | ForEach-Object { Write-Host "   - $_" -ForegroundColor Yellow }

            if ($CheckOnly) {
                Write-Host "❌ FAILED: Working tree contains unformatted code (CheckOnly mode)." -ForegroundColor Red
                $Job0Status = "FAILED"
                $OverallSuccess = $false
            } else {
                Write-Host "✅ Code has been reformatted according to CodingRules.md." -ForegroundColor Green
                Write-Host "   Please review and stage the modified files before committing." -ForegroundColor Cyan
            }
        } else {
            Write-Host "✅ All source files comply with formatting rules!" -ForegroundColor Green
        }
    } else {
        Write-Host "⚠️ Warning: format_rules.py not found at $FormatScript" -ForegroundColor Yellow
    }
}

# ------------------------------------------------------------------------------
# JOB 1: Clangd Static Analysis & Compiler Strict Semantic Checks
# ------------------------------------------------------------------------------
if (-not $SkipStaticAnalysis) {
    Write-Host "`n==================================================" -ForegroundColor Cyan
    Write-Host " Job 1: Clangd Static Analysis & Compiler Checks" -ForegroundColor Cyan
    Write-Host "==================================================" -ForegroundColor Cyan

    $CompileCommandsPath = Join-Path $ProjectRoot "FW-ESP\build\compile_commands.json"
    if (-not (Test-Path $CompileCommandsPath)) {
        Write-Host "Notice: FW-ESP/build/compile_commands.json not found." -ForegroundColor Yellow
        $idfCmd = Get-Command idf.py -ErrorAction SilentlyContinue
        if ($idfCmd) {
            Write-Host "Running 'idf.py -C FW-ESP reconfigure' to generate compilation database..." -ForegroundColor Yellow
            idf.py -C FW-ESP reconfigure
        } else {
            Write-Host "❌ ERROR: Cannot find FW-ESP/build/compile_commands.json and idf.py is not available." -ForegroundColor Red
            $Job1Status = "FAILED"
            $OverallSuccess = $false
        }
    }

    if (Test-Path $CompileCommandsPath) {
        # Discover target C files
        if (-not $Files -or $Files.Count -eq 0) {
            $TargetFiles = Get-ChildItem -Path FW-Common, FW-ESP, FW-STM -Filter "*.c" -Recurse |
                Where-Object { $_.FullName -notmatch '\\build\\' -and $_.FullName -notmatch '\\\.template\\' } |
                ForEach-Object {
                    # Convert to relative path from project root
                    $full = $_.FullName
                    if ($full.StartsWith($ProjectRoot)) {
                        $full.Substring($ProjectRoot.Length).TrimStart('\', '/')
                    } else {
                        $full
                    }
                }
        } else {
            $TargetFiles = $Files
        }

        Write-Host "`nTarget Source Files ($($TargetFiles.Count) files):" -ForegroundColor DarkCyan
        $TargetFiles | ForEach-Object { Write-Host "  - $_" }
        Write-Host "--------------------------------------------------" -ForegroundColor DarkGray

        # Phase 1: Clangd Static Analysis (AST & Semantic)
        if ($ClangdExe) {
            Write-Host "`n>>> [Phase 1/2] Running Clangd Semantic & AST Checks..." -ForegroundColor Yellow
            $ClangdScript = Join-Path $ProjectRoot ".github\scripts\clangd_check.py"
            & $PythonExe $ClangdScript $ProjectRoot @TargetFiles
            if ($LASTEXITCODE -ne 0) {
                Write-Host "❌ Clangd Static Analysis reported issues." -ForegroundColor Red
                $Job1Status = "FAILED"
                $OverallSuccess = $false
            }
        } else {
            Write-Host "⚠️ Warning: 'clangd' executable not found. Skipping Phase 1 (Clangd AST check)." -ForegroundColor Yellow
        }

        # Phase 2: Target Compiler Strict Semantic & Dead-Code Checks
        Write-Host "`n>>> [Phase 2/2] Running Target Compiler Strict Semantic & Warning Check..." -ForegroundColor Yellow
        $CompilerScript = Join-Path $ProjectRoot ".github\scripts\compiler_check.py"
        & $PythonExe $CompilerScript $CompileCommandsPath @TargetFiles
        if ($LASTEXITCODE -ne 0) {
            Write-Host "❌ Target compiler strict check reported issues." -ForegroundColor Red
            $Job1Status = "FAILED"
            $OverallSuccess = $false
        }
    }
}

# ------------------------------------------------------------------------------
# Final Summary Report
# ------------------------------------------------------------------------------
Write-Host "`n==================================================" -ForegroundColor Cyan
Write-Host " Execution Summary Report" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "  Job 0 (Conventions & Formatting) : $Job0Status" -ForegroundColor $(if ($Job0Status -eq "PASSED") { "Green" } else { "Red" })
Write-Host "  Job 1 (Clangd & Compiler Check)  : $Job1Status" -ForegroundColor $(if ($Job1Status -eq "PASSED") { "Green" } else { "Red" })
Write-Host "==================================================" -ForegroundColor Cyan

if ($OverallSuccess) {
    Write-Host "✅ All local pre-commit checks PASSED successfully! Ready to commit." -ForegroundColor Green
    exit 0
} else {
    Write-Host "❌ Some checks FAILED. Please review the output above before committing." -ForegroundColor Red
    exit 1
}
