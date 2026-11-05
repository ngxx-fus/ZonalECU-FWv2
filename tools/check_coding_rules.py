#!/usr/bin/env python3
"""
==============================================================================================================
ZonalECU-FWv2 Coding Rules Checker (tools/check_coding_rules.py)
==============================================================================================================
Purpose:
    Automated static checker invoked by `pre-commit` (or manually via CLI) to enforce the project's
    C coding rules defined in `CodingRules.md` (Renesas RA FSP + Embedded C conventions).

Usage:
    # Run via pre-commit on all tracked files:
    pre-commit run zecu-coding-rules --all-files

    # Run directly on specific files:
    python tools/check_coding_rules.py components/Common/Macros.h components/Common/Error.h

How to Maintain / Customize Rules:
    1. Toggle individual rules ON/OFF in the `ENABLED_RULES` dictionary below.
    2. Add project-specific technical terms to `.vscode/settings.json` (`"cSpell.words"`) or
       `CUSTOM_ALLOWED_WORDS` below.
    3. Add new common misspellings to `COMMON_TYPOS` under Section 2.
    4. Each rule is implemented as a dedicated, self-contained `check_rXX_<name>()` function with
       detailed comments explaining what it checks and how to adjust its logic.
==============================================================================================================
"""

from __future__ import annotations

import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Sequence, Set


# ============================================================================================================
# 1. CONFIGURATION & RULE TOGGLES (Edit here for quick maintenance)
# ============================================================================================================

# Maximum allowed characters per line (110 matches the 2-line section banners in .vscode/ZECU.code-snippets)
MAX_LINE_LENGTH: int = 110

# Expected column widths of `/* SECTION ***...` (109) and ` ***...*/` (110) separator banner lines
# (Matches .vscode/ZECU.code-snippets and CodingRules.md; 108 is also accepted for 1-line sub-banners)
ALLOWED_BANNER_WIDTHS: Set[int] = {108, 109, 110}

# Required indentation step size in spaces (CodingRules.md Section 4.1: tabsize = 4, including macros)
TAB_SIZE: int = 4

# Master switchboard: Set any rule to `False` to temporarily disable it across the repository
ENABLED_RULES: Dict[str, bool] = {
    "R01_LINE_WIDTH":                True,  # Section 1  : Line length must not exceed MAX_LINE_LENGTH
    "R02_BANNERS":                   True,  # Section 6.1: No `===` banners; `*` banners must be 108-110 chars
    "R03_HEADER_GUARDS_CPP":         True,  # Section 2  : .h files need #ifndef guards & HEADER_CPP/FOOTER_CPP
    "R04_COMPACT_DOXYGEN":           True,  # Section 6.5: Doxygen blocks open with `/** @brief` (no bare `/**`)
    "R05_NO_IF_0":                   True,  # Section 5  : Disallow bare `#if 0` (use descriptive macro names)
    "R06_TYPEDEF_NAMING":            True,  # Section 3.4: Custom typedefs must match `<module>_<name>_t`
    "R07_POINTER_PREFIX":            True,  # Section 3.3: Pointer params/vars must use `p_`, `pp_`, `gp_`, `sp_`
    "R08_FUNC_NAMING":               True,  # Section 3.2: Public funcs `<PREFIX>_<PascalCase>`, private `prv_...`
    "R09_BRANCH_COMMENTS":           True,  # Section 6.2: Mandatory comment before if/else/switch/return/break/...
    "R10_TABSIZE_4":                 True,  # Section 4.1: No `\t` tabs; 4-space indentation (including macros)
    "R11_BLANK_LINE_BEFORE_COMMENT": True,  # Section 6.2: Empty line required before standalone comments
    "R12_SPELL_CHECK":               True,  # Section 1  : Spell check comments, docs, and identifiers
}

# Files (by basename) that do not require HEADER_CPP / FOOTER_CPP
# (e.g., Macros.h defines HEADER_CPP itself; SharedAPIs.h is a pure include aggregator)
HEADER_CPP_EXEMPT_FILES: Set[str] = {
    "Macros.h",
    "SharedAPIs.h",
    "__prv_shared__.h",
}

# Pointer variable names allowed without a `p_` prefix (e.g., standard ESP-IDF logging `TAG` or `argv`)
ALLOWED_POINTER_NAMES: Set[str] = {
    "TAG",
    "argv",
}

# Function names allowed without `<PREFIX>_<PascalCase>` (e.g., ESP-IDF entry point `app_main` or `main`)
ALLOWED_FUNCTION_NAMES: Set[str] = {
    "app_main",
    "main",
}

# Project-specific technical words exempt from spell-check warnings (supplemented by `.vscode/settings.json`)
CUSTOM_ALLOWED_WORDS: Set[str] = {
    "zecu",
    "someip",
    "xtensa",
    "freertos",
    "espressif",
    "nvs",
    "autoneg",
    "inout",
    "prv",
    "msecs",
    "usecs",
}

# Curated dictionary of common English & Embedded C misspellings -> suggested correction.
# Easy to maintain: add `"typo_lowercase": "correction"` entries here at any time.
COMMON_TYPOS: Dict[str, str] = {
    "abiguity":       "ambiguity",
    "accomodate":     "accommodate",
    "acheive":        "achieve",
    "addres":         "address",
    "adddress":       "address",
    "aligment":       "alignment",
    "allign":         "align",
    "alligned":       "aligned",
    "allignment":     "alignment",
    "alway":          "always",
    "arround":        "around",
    "asign":          "assign",
    "asignment":      "assignment",
    "auxillary":      "auxiliary",
    "availabe":       "available",
    "availble":       "available",
    "begining":       "beginning",
    "bufer":          "buffer",
    "bufffer":        "buffer",
    "calback":        "callback",
    "calculat":       "calculate",
    "cant":           "cannot / can't",
    "charachter":     "character",
    "channnel":       "channel",
    "clearified":     "clarified",
    "collison":       "collision",
    "commad":         "command",
    "comunication":   "communication",
    "compability":    "compatibility",
    "comparision":    "comparison",
    "confitnue":      "continue",
    "configuartion":  "configuration",
    "configuraton":   "configuration",
    "conection":      "connection",
    "consistant":     "consistent",
    "contiguos":      "contiguous",
    "continous":      "continuous",
    "controler":      "controller",
    "convertor":      "converter",
    "correspondant":  "correspondent",
    "currenly":       "currently",
    "defualt":        "default",
    "decribe":        "describe",
    "defintion":      "definition",
    "dependancy":     "dependency",
    "desciptor":      "descriptor",
    "destinaton":     "destination",
    "detatch":        "detach",
    "disconect":      "disconnect",
    "documention":    "documentation",
    "dont":           "do not / don't",
    "enviroment":     "environment",
    "errror":         "error",
    "excecute":       "execute",
    "exisitng":       "existing",
    "explicitely":    "explicitly",
    "faliure":        "failure",
    "feild":          "field",
    "foward":         "forward",
    "freind":         "friend",
    "fuction":        "function",
    "funtion":        "function",
    "garantee":       "guarantee",
    "guarante":       "guarantee",
    "hander":         "handler",
    "hardwre":        "hardware",
    "headder":        "header",
    "hight":          "height / high",
    "identifer":      "identifier",
    "immediatly":     "immediately",
    "implemetation":  "implementation",
    "incomming":      "incoming",
    "incompatibel":   "incompatible",
    "independant":    "independent",
    "infomation":     "information",
    "initalize":      "initialize",
    "initalized":     "initialized",
    "initilize":      "initialize",
    "inteface":       "interface",
    "interupt":       "interrupt",
    "interrrupt":     "interrupt",
    "invokation":     "invocation",
    "lenght":         "length",
    "lenth":          "length",
    "maitain":        "maintain",
    "maintainance":   "maintenance",
    "managment":      "management",
    "maxium":         "maximum",
    "mesage":         "message",
    "messge":         "message",
    "milisecond":     "millisecond",
    "minium":         "minimum",
    "mismach":        "mismatch",
    "modle":          "model / module",
    "mutiple":        "multiple",
    "neccessary":     "necessary",
    "necesary":       "necessary",
    "negotation":     "negotiation",
    "notfication":    "notification",
    "occured":        "occurred",
    "occurence":      "occurrence",
    "oficial":        "official",
    "overide":        "override",
    "packge":         "package",
    "paramter":       "parameter",
    "paramters":      "parameters",
    "paralllel":      "parallel",
    "periperal":      "peripheral",
    "peripherial":    "peripheral",
    "permision":      "permission",
    "pionter":        "pointer",
    "posible":        "possible",
    "prefered":       "preferred",
    "preceed":        "precede",
    "preceeding":     "preceding",
    "priviledge":     "privilege",
    "proccess":       "process",
    "procesor":       "processor",
    "protcol":        "protocol",
    "readabillity":   "readability",
    "recieve":        "receive",
    "recieved":       "received",
    "reciever":       "receiver",
    "recognise":      "recognize",
    "recomend":       "recommend",
    "refernce":       "reference",
    "regiser":        "register",
    "relevent":       "relevant",
    "repositry":      "repository",
    "requried":       "required",
    "resett":         "reset",
    "responce":       "response",
    "retreive":       "retrieve",
    "retrun":         "return",
    "seperate":       "separate",
    "seperated":      "separated",
    "seperator":      "separator",
    "sequnce":        "sequence",
    "signficant":     "significant",
    "similiar":       "similar",
    "specifcation":   "specification",
    "standart":       "standard",
    "statment":       "statement",
    "stucture":       "structure",
    "subsytem":       "subsystem",
    "succesful":      "successful",
    "succesfully":    "successfully",
    "sucess":         "success",
    "syncronize":     "synchronize",
    "threshhold":     "threshold",
    "timout":         "timeout",
    "transciever":    "transceiver",
    "transfered":     "transferred",
    "transmision":    "transmission",
    "tigger":         "trigger",
    "unecessary":     "unnecessary",
    "unintialized":   "uninitialized",
    "unkown":         "unknown",
    "usefull":        "useful",
    "valdiate":       "validate",
    "varible":        "variable",
    "verfication":    "verification",
    "wether":         "whether",
    "writen":         "written",
}


# ============================================================================================================
# 2. DATA STRUCTURES & REGEX PATTERNS
# ============================================================================================================

@dataclass
class Violation:
    """Represents a single coding rule violation reported to the developer."""
    filepath: str
    line_num: int
    rule_id: str
    message: str

    def format(self) -> str:
        return f"{self.filepath}:{self.line_num}: [{self.rule_id}] {self.message}"


# Regex for prohibited `===` or `---` style box banners (CodingRules.md Section 6.1)
RE_PROHIBITED_BANNER = re.compile(r"^\s*/\*+\s*[=-]{4,}")

# Regex for major/sub-section asterisk banners:
RE_ASTERISK_BANNER_TOP = re.compile(r"^/\*\s+[A-Z0-9_/\s-]+\s+\*{10,}\r?$")
RE_ASTERISK_BANNER_BOT = re.compile(r"^\s*\*+\*/\r?$")
RE_ASTERISK_SUBBANNER  = re.compile(r"^/\*\s+[A-Z0-9_/\s-]+\s+\*{10,}/\r?$")

# Regex for bare `#if 0` preprocessor blocks (CodingRules.md Section 5)
RE_BARE_IF_0 = re.compile(r"^\s*#\s*if\s+0\b")

# Regex for single-line typedefs: `typedef <existing_type> <new_type_name>;`
RE_SINGLE_LINE_TYPEDEF = re.compile(
    r"^\s*typedef\s+(?!struct\b|enum\b|union\b)([^;()]+?)\s+([A-Za-z_][A-Za-z0-9_]*)\s*;"
)

# Regex for closing line of multi-line `typedef struct/enum/union { ... } <type_name>;`
RE_CLOSING_TYPEDEF = re.compile(r"^\s*\}\s*([A-Za-z_][A-Za-z0-9_]*)\s*;")

# Valid `<module>_<name>_t` pattern: all lowercase alphanumeric with at least one underscore before `_t`
RE_VALID_TYPE_NAME = re.compile(r"^[a-z][a-z0-9]*(?:_[a-z0-9]+)+_t$")

# Regex for function definitions/declarations at column 0:
RE_FUNC_SIGNATURE = re.compile(
    r"^(?!typedef\b|return\b|else\b|if\b|while\b|for\b|switch\b)"
    r"([A-Za-z_][A-Za-z0-9_\s\*]*?)\b([A-Za-z_][A-Za-z0-9_]*)\s*\(([^;{}]*)\)\s*(?:;|\{|$)"
)

# Valid function naming patterns (CodingRules.md Section 3.2):
RE_PUBLIC_FUNC_NAME  = re.compile(r"^(?:[A-Z][A-Z0-9]*|[A-Z][a-z0-9]+(?:[A-Z][a-z0-9]+)*)_[A-Z][A-Za-z0-9]*$")
RE_PRIVATE_FUNC_NAME = re.compile(r"^(?:prv_|__)[A-Za-z0-9_]+$")

# Regex for branching statements in `.c` files (CodingRules.md Section 6.2):
RE_BRANCH_STATEMENT = re.compile(
    r"^\s*(?:}\s*)?(if\s*\(|else\s+if\s*\(|else\b|switch\s*\(|return\b|break\s*;|continue\s*;|goto\s+[A-Za-z_])"
)

# Regex for preprocessor directives (captures leading indentation + directive keyword + rest of line)
RE_PREPROC_DIRECTIVE = re.compile(r"^(\s*)#(\s*)(if|ifdef|ifndef|elif|else|endif|define|undef|include|pragma|error|warning)\b(.*)$")


# ============================================================================================================
# 3. INDIVIDUAL RULE CHECK FUNCTIONS
# ============================================================================================================

def check_r01_line_width(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R01_LINE_WIDTH] Enforces maximum line width (CodingRules.md Section 1).
    """
    violations: List[Violation] = []
    for idx, raw_line in enumerate(lines, start=1):
        line = raw_line.rstrip("\r\n")
        if len(line) > MAX_LINE_LENGTH:
            violations.append(
                Violation(
                    filepath=filepath,
                    line_num=idx,
                    rule_id="R01_LINE_WIDTH",
                    message=f"Line length ({len(line)}) exceeds maximum {MAX_LINE_LENGTH} characters.",
                )
            )
    return violations


def check_r02_banners(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R02_BANNERS] Enforces section separator banner rules (CodingRules.md Section 6.1):
      - Prohibits `/* ===...` or `/* ---...` decorative borders.
      - Ensures 2-line and 1-line asterisk section banners match standard banner width.
    """
    violations: List[Violation] = []
    for idx, raw_line in enumerate(lines, start=1):
        line = raw_line.rstrip("\r\n")

        if RE_PROHIBITED_BANNER.match(line):
            violations.append(
                Violation(
                    filepath=filepath,
                    line_num=idx,
                    rule_id="R02_BANNERS",
                    message="Prohibited '===' or '---' banner format. Use standard asterisk section banners.",
                )
            )
            continue

        if (
            RE_ASTERISK_BANNER_TOP.match(line)
            or RE_ASTERISK_BANNER_BOT.match(line)
            or RE_ASTERISK_SUBBANNER.match(line)
        ):
            if len(line) not in ALLOWED_BANNER_WIDTHS:
                violations.append(
                    Violation(
                        filepath=filepath,
                        line_num=idx,
                        rule_id="R02_BANNERS",
                        message=(
                            f"Section banner width is {len(line)} chars "
                            f"(expected 109/110 matching ZECU.code-snippets)."
                        ),
                    )
                )
    return violations


def check_r03_header_guards_and_cpp(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R03_HEADER_GUARDS_CPP] Enforces header file structure (CodingRules.md Section 2):
      - `.h` files must have `#ifndef __..._H__` and `#define __..._H__` include guards.
      - `.h` files must use `HEADER_CPP` and `FOOTER_CPP` in pairs (unless in HEADER_CPP_EXEMPT_FILES).
    """
    violations: List[Violation] = []
    path_obj = Path(filepath)
    if path_obj.suffix.lower() != ".h":
        return violations

    content = "\n".join(lines)

    if not re.search(r"#\s*ifndef\s+__[A-Z0-9_]+_H__", content):
        violations.append(
            Violation(
                filepath=filepath,
                line_num=1,
                rule_id="R03_HEADER_GUARDS_CPP",
                message="Header file is missing standard include guard `#ifndef __<MODULE>_<NAME>_H__`.",
            )
        )

    if path_obj.name in HEADER_CPP_EXEMPT_FILES:
        return violations

    header_cpp_count = len(re.findall(r"^\s*HEADER_CPP\s*$", content, flags=re.MULTILINE))
    footer_cpp_count = len(re.findall(r"^\s*FOOTER_CPP\s*$", content, flags=re.MULTILINE))

    if header_cpp_count == 0 or footer_cpp_count == 0 or header_cpp_count != footer_cpp_count:
        violations.append(
            Violation(
                filepath=filepath,
                line_num=1,
                rule_id="R03_HEADER_GUARDS_CPP",
                message=(
                    f"Header file must contain matching HEADER_CPP ({header_cpp_count}) "
                    f"and FOOTER_CPP ({footer_cpp_count}) macros."
                ),
            )
        )
    return violations


def check_r04_compact_doxygen(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R04_COMPACT_DOXYGEN] Enforces compact Doxygen comment blocks (CodingRules.md Section 6.5):
      - Disallows standalone `/**` opening lines (must start directly with `/** @brief ...`).
      - Disallows standalone `*/` closing lines on multi-line Doxygen blocks (close on last text line).
    """
    violations: List[Violation] = []
    in_doxygen_block = False

    for idx, raw_line in enumerate(lines, start=1):
        stripped = raw_line.strip()

        if stripped.startswith("/**") and not stripped.startswith("/***"):
            if stripped == "/**":
                violations.append(
                    Violation(
                        filepath=filepath,
                        line_num=idx,
                        rule_id="R04_COMPACT_DOXYGEN",
                        message="Avoid standalone '/**' line; open directly with '/** @brief ...'.",
                    )
                )
            in_doxygen_block = not stripped.endswith("*/")
            continue

        if in_doxygen_block:
            if stripped == "*/":
                violations.append(
                    Violation(
                        filepath=filepath,
                        line_num=idx,
                        rule_id="R04_COMPACT_DOXYGEN",
                        message="Avoid standalone '*/' closing line in Doxygen block; close on the last text line.",
                    )
                )
                in_doxygen_block = False
            elif stripped.endswith("*/"):
                in_doxygen_block = False

    return violations


def check_r05_no_if_0(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R05_NO_IF_0] Disallows bare `#if 0` blocks (CodingRules.md Section 5).
    """
    violations: List[Violation] = []
    for idx, raw_line in enumerate(lines, start=1):
        if RE_BARE_IF_0.match(raw_line):
            violations.append(
                Violation(
                    filepath=filepath,
                    line_num=idx,
                    rule_id="R05_NO_IF_0",
                    message="Do not use bare `#if 0`. Use a descriptive macro (e.g., `#if ZE_TEMP_DISABLED`).",
                )
            )
    return violations


def check_r06_typedef_naming(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R06_TYPEDEF_NAMING] Enforces `<module>_<name>_t` naming on all `typedef` declarations
    (CodingRules.md Section 3.4).
    """
    violations: List[Violation] = []
    in_multiline_typedef = False
    brace_depth = 0

    for idx, raw_line in enumerate(lines, start=1):
        code_line = re.sub(r"/\*.*?\*/", "", raw_line)
        code_line = re.sub(r"//.*$", "", code_line).strip()
        if not code_line:
            continue

        single_match = RE_SINGLE_LINE_TYPEDEF.match(code_line)
        if single_match and "{" not in code_line:
            type_name = single_match.group(2)
            if not RE_VALID_TYPE_NAME.match(type_name):
                violations.append(
                    Violation(
                        filepath=filepath,
                        line_num=idx,
                        rule_id="R06_TYPEDEF_NAMING",
                        message=(
                            f"Typedef `{type_name}` must use lowercase `<module>_<name>_t` format "
                            f"(e.g., `zecu_{type_name.lower()}_t`)."
                        ),
                    )
                )
            continue

        if re.match(r"^typedef\s+(?:struct|enum|union)\b", code_line):
            in_multiline_typedef = True
            brace_depth = code_line.count("{") - code_line.count("}")
            continue

        if in_multiline_typedef:
            brace_depth += code_line.count("{") - code_line.count("}")
            if brace_depth <= 0 and ";" in code_line:
                closing_match = RE_CLOSING_TYPEDEF.match(code_line)
                if closing_match:
                    type_name = closing_match.group(1)
                    if not RE_VALID_TYPE_NAME.match(type_name):
                        violations.append(
                            Violation(
                                filepath=filepath,
                                line_num=idx,
                                rule_id="R06_TYPEDEF_NAMING",
                                message=(
                                    f"Typedef `{type_name}` must use lowercase `<module>_<name>_t` format."
                                ),
                            )
                        )
                in_multiline_typedef = False
                brace_depth = 0

    return violations


def check_r07_pointer_prefix(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R07_POINTER_PREFIX] Enforces `p_` / `pp_` / `gp_` / `sp_` prefixes on pointer parameters
    (CodingRules.md Section 3.3, Renesas RA FSP standard).
    """
    violations: List[Violation] = []

    for idx, raw_line in enumerate(lines, start=1):
        code_line = re.sub(r"/\*.*?\*/", "", raw_line)
        code_line = re.sub(r"//.*$", "", code_line).rstrip()
        if not code_line or code_line.lstrip().startswith("#"):
            continue

        func_match = RE_FUNC_SIGNATURE.match(code_line)
        if func_match:
            params_str = func_match.group(3).strip()
            if params_str and params_str != "void":
                for param in params_str.split(","):
                    param_clean = param.strip()
                    if "*" in param_clean:
                        ident_match = re.search(
                            r"\*+\s*(?:const\s+|RESTRICT\s+)*([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[.*\])?$",
                            param_clean,
                        )
                        if ident_match:
                            param_name = ident_match.group(1)
                            if (
                                param_name not in ALLOWED_POINTER_NAMES
                                and not param_name.startswith(("p_", "pp_"))
                            ):
                                violations.append(
                                    Violation(
                                        filepath=filepath,
                                        line_num=idx,
                                        rule_id="R07_POINTER_PREFIX",
                                        message=(
                                            f"Pointer parameter `{param_name}` must be prefixed with `p_` or `pp_` "
                                            f"(e.g., `p_{param_name}`)."
                                        ),
                                    )
                                )

    return violations


def check_r08_func_naming(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R08_FUNC_NAMING] Enforces function naming conventions (CodingRules.md Section 3.2):
      - Public functions: `<PREFIX>_<PascalCase>()` (e.g., `ETH_Open`, `ZECU_ErrToStr`, `Comm_Init`).
      - Private (`static`) functions: `prv_<name>()` or `__<name>()`.
    """
    violations: List[Violation] = []

    for idx, raw_line in enumerate(lines, start=1):
        code_line = re.sub(r"/\*.*?\*/", "", raw_line)
        code_line = re.sub(r"//.*$", "", code_line).rstrip()
        if not code_line or code_line.lstrip().startswith("#"):
            continue

        func_match = RE_FUNC_SIGNATURE.match(code_line)
        if not func_match:
            continue

        qualifiers = func_match.group(1).strip()
        func_name  = func_match.group(2).strip()

        if func_name in ALLOWED_FUNCTION_NAMES:
            continue

        is_static = bool(re.search(r"\bstatic\b", qualifiers))

        if is_static:
            if not RE_PRIVATE_FUNC_NAME.match(func_name):
                violations.append(
                    Violation(
                        filepath=filepath,
                        line_num=idx,
                        rule_id="R08_FUNC_NAMING",
                        message=(
                            f"Private static function `{func_name}()` must start with `prv_` or `__` "
                            f"(e.g., `prv_{func_name}`)."
                        ),
                    )
                )
        else:
            if not RE_PUBLIC_FUNC_NAME.match(func_name):
                violations.append(
                    Violation(
                        filepath=filepath,
                        line_num=idx,
                        rule_id="R08_FUNC_NAMING",
                        message=(
                            f"Public function `{func_name}()` must follow `<PREFIX>_<PascalCase>()` "
                            f"(e.g., `MOD_ActionName`)."
                        ),
                    )
                )

    return violations


def check_r09_branch_comments(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R09_BRANCH_COMMENTS] Enforces mandatory comments before branching/control-flow statements
    (`if`, `else if`, `else`, `switch`, `return`, `break`, `continue`, `goto`) in `.c` files
    (CodingRules.md Section 6.2).
    """
    violations: List[Violation] = []
    if Path(filepath).suffix.lower() != ".c":
        return violations

    prev_non_empty_line = ""
    for idx, raw_line in enumerate(lines, start=1):
        stripped = raw_line.strip()
        if not stripped:
            continue

        if stripped.startswith(("#", "/*", "*", "//")):
            prev_non_empty_line = stripped
            continue

        branch_match = RE_BRANCH_STATEMENT.match(raw_line)
        if branch_match:
            keyword = branch_match.group(1).split()[0]
            has_inline_comment = ("/*" in stripped) or ("//" in stripped)
            prev_was_comment   = prev_non_empty_line.endswith("*/") or prev_non_empty_line.startswith("//")

            if not (prev_was_comment or has_inline_comment):
                violations.append(
                    Violation(
                        filepath=filepath,
                        line_num=idx,
                        rule_id="R09_BRANCH_COMMENTS",
                        message=(
                            f"Branching statement `{keyword}` must have an explanatory comment "
                            f"on the preceding line (CodingRules.md Section 6.2)."
                        ),
                    )
                )

        prev_non_empty_line = stripped

    return violations


def check_r10_tabsize_4(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R10_TABSIZE_4] Enforces `tabsize = 4` (4 spaces per indentation level, including preprocessor macros)
    and prohibits raw `\\t` tab characters (CodingRules.md Section 4.1).

    Details:
      1. Rejects any line containing a raw `\\t` tab character.
      2. Tracks nested `#if` / `#ifdef` / `#ifndef` / `#else` / `#elif` / `#endif` / `#define` directives
         (excluding the outer `#ifndef __<MODULE>_<NAME>_H__` include guard) and checks that nested
         preprocessor directives are indented by `4 * preproc_depth` spaces.
      3. Verifies that normal code lines have leading space counts divisible by 4 (ignoring ` *` continuation
         lines inside multi-line `/* ... */` or `/** ... */` comments).
    """
    violations: List[Violation] = []
    preproc_depth = 0
    has_outer_header_guard = False

    for idx, raw_line in enumerate(lines, start=1):
        line = raw_line.rstrip("\r\n")

        # 1. Disallow raw tab characters `\t`
        if "\t" in line:
            violations.append(
                Violation(
                    filepath=filepath,
                    line_num=idx,
                    rule_id="R10_TABSIZE_4",
                    message="Tab character '\\t' is prohibited. Use 4 spaces for indentation (tabsize = 4).",
                )
            )
            continue

        stripped = line.lstrip(" ")
        if not stripped:
            continue

        leading_spaces = len(line) - len(stripped)

        # Skip asterisk continuation lines of multi-line comments (e.g. ` ***...*/` or ` *  @param ...`)
        if stripped.startswith("*"):
            continue

        # 2. Check preprocessor directive indentation (`#ifdef`, `#ifndef`, `#if`, `#else`, `#endif`, `#define`)
        preproc_match = RE_PREPROC_DIRECTIVE.match(line)
        if preproc_match:
            directive = preproc_match.group(3)
            rest      = preproc_match.group(4).strip()

            # Detect top-level header guard `#ifndef __<MODULE>_<NAME>_H__` (stays at indent 0)
            if (
                directive == "ifndef"
                and preproc_depth == 0
                and not has_outer_header_guard
                and re.match(r"^__[A-Z0-9_]+_H__\b", rest)
            ):
                has_outer_header_guard = True
                continue

            # Detect matching `#define __<MODULE>_<NAME>_H__` right after outer guard
            if (
                directive == "define"
                and preproc_depth == 0
                and has_outer_header_guard
                and re.match(r"^__[A-Z0-9_]+_H__\b", rest)
            ):
                continue

            # Detect closing `#endif` for outer header guard at end of file
            if directive == "endif" and preproc_depth == 0 and has_outer_header_guard:
                has_outer_header_guard = False
                continue

            # Adjust expected depth for `#else`, `#elif`, `#endif` before checking indentation
            if directive in ("else", "elif", "endif"):
                expected_depth = max(0, preproc_depth - 1)
            else:
                expected_depth = preproc_depth

            expected_spaces = expected_depth * TAB_SIZE
            if leading_spaces != expected_spaces:
                violations.append(
                    Violation(
                        filepath=filepath,
                        line_num=idx,
                        rule_id="R10_TABSIZE_4",
                        message=(
                            f"Preprocessor directive `#{directive}` has {leading_spaces} leading spaces "
                            f"(expected {expected_spaces} spaces for nesting depth {expected_depth}, tabsize = 4)."
                        ),
                    )
                )

            # Update nesting depth after processing `#if` / `#ifdef` / `#ifndef` / `#endif`
            if directive in ("if", "ifdef", "ifndef"):
                preproc_depth += 1
            elif directive == "endif":
                preproc_depth = max(0, preproc_depth - 1)
            continue

        # 3. Normal code/comment lines: leading spaces must be a multiple of TAB_SIZE (4)
        if leading_spaces % TAB_SIZE != 0:
            violations.append(
                Violation(
                    filepath=filepath,
                    line_num=idx,
                    rule_id="R10_TABSIZE_4",
                    message=(
                        f"Indentation is {leading_spaces} spaces (must be a multiple of {TAB_SIZE} spaces)."
                    ),
                )
            )

    return violations


def check_r11_blank_line_before_comment(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R11_BLANK_LINE_BEFORE_COMMENT] Enforces an empty line before standalone comments
    (CodingRules.md Section 6.2).

    Exceptions where a preceding blank line is NOT required:
      1. Line 1 of the file.
      2. Immediately after an opening brace `{` (e.g., start of function, `if (...) {`, `else {`, `struct {`).
      3. Immediately after a `case ...:` or `default:` label in a `switch` statement.
      4. Consecutive comment lines (or continuation lines `* ...` inside a block comment).
    """
    violations: List[Violation] = []
    in_block_comment = False

    for idx, raw_line in enumerate(lines, start=1):
        stripped = raw_line.strip()

        # Track multi-line block comment state
        if in_block_comment:
            if "*/" in stripped:
                in_block_comment = False
            continue

        if not stripped:
            continue

        is_standalone_comment = stripped.startswith(("/*", "//"))
        if is_standalone_comment:
            if stripped.startswith("/*") and "*/" not in stripped:
                in_block_comment = True

            # Exception 1: First line of the file
            if idx == 1:
                continue

            prev_line = lines[idx - 2].strip()

            # Allowed if previous line is empty
            if prev_line == "":
                continue

            # Exception 2: Previous line ends with an opening brace `{` or `:` (for `case ...:` / `default:`)
            if prev_line.endswith(("{", ":")):
                continue

            # Exception 3: Previous line is itself a standalone comment or end of a block comment
            if prev_line.startswith(("/*", "*", "//")) or prev_line.endswith("*/"):
                continue

            violations.append(
                Violation(
                    filepath=filepath,
                    line_num=idx,
                    rule_id="R11_BLANK_LINE_BEFORE_COMMENT",
                    message="Add an empty line before standalone comment (CodingRules.md Section 6.2).",
                )
            )

    return violations


def _load_cspell_words() -> Set[str]:
    """Loads custom project vocabulary from `.vscode/settings.json` (`cSpell.words`) if present."""
    words: Set[str] = set(CUSTOM_ALLOWED_WORDS)
    settings_path = Path(".vscode/settings.json")
    if settings_path.is_file():
        try:
            text = settings_path.read_text(encoding="utf-8", errors="ignore")
            # Strip `//` and `/* */` comments if any exist in JSONC
            clean_json = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
            clean_json = re.sub(r"^\s*//.*$", "", clean_json, flags=re.MULTILINE)
            data = json.loads(clean_json)
            for word in data.get("cSpell.words", []):
                if isinstance(word, str):
                    words.add(word.lower())
        except Exception:
            pass
    return words


def _split_identifier_words(token: str) -> List[str]:
    """Splits `camelCase`, `PascalCase`, and `snake_case` tokens into individual lowercase words."""
    # Split on underscores first, then split camelCase transitions (e.g. `ErrToStr` -> `Err`, `To`, `Str`)
    subtokens = token.split("_")
    words: List[str] = []
    for sub in subtokens:
        if not sub:
            continue
        parts = re.findall(r"[A-Z]?[a-z]+|[A-Z]+(?=[A-Z][a-z]|\d|\W|$)", sub)
        for p in parts:
            words.append(p.lower())
    return words


def check_r12_spell_check(filepath: str, lines: Sequence[str]) -> List[Violation]:
    """
    [R12_SPELL_CHECK] Checks comments, documentation, and identifiers for common English & embedded C
    spelling mistakes (CodingRules.md Section 1).
    Respects custom words configured in `.vscode/settings.json` (`cSpell.words`) and `CUSTOM_ALLOWED_WORDS`.
    """
    violations: List[Violation] = []
    allowed_words = _load_cspell_words()

    for idx, raw_line in enumerate(lines, start=1):
        # Extract all alphabetic tokens from the line
        tokens = re.findall(r"[A-Za-z]{3,}", raw_line)
        for token in tokens:
            for word in _split_identifier_words(token):
                if word in allowed_words:
                    continue
                if word in COMMON_TYPOS:
                    suggestion = COMMON_TYPOS[word]
                    violations.append(
                        Violation(
                            filepath=filepath,
                            line_num=idx,
                            rule_id="R12_SPELL_CHECK",
                            message=f"Spelling error `{word}` (in `{token}`) -> did you mean `{suggestion}`?",
                        )
                    )

    return violations


# ============================================================================================================
# 4. MAIN RUNNER & CLI ENTRY POINT
# ============================================================================================================

def check_file(filepath: str) -> List[Violation]:
    """Runs all enabled coding rule checks on a single `.c` or `.h` file."""
    path_obj = Path(filepath)
    if not path_obj.is_file():
        return []

    try:
        raw_text = path_obj.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        return [Violation(filepath, 0, "IO_ERROR", f"Could not read file: {exc}")]

    lines = raw_text.splitlines()
    violations: List[Violation] = []

    if ENABLED_RULES.get("R01_LINE_WIDTH", False):
        violations.extend(check_r01_line_width(filepath, lines))
    if ENABLED_RULES.get("R02_BANNERS", False):
        violations.extend(check_r02_banners(filepath, lines))
    if ENABLED_RULES.get("R03_HEADER_GUARDS_CPP", False):
        violations.extend(check_r03_header_guards_and_cpp(filepath, lines))
    if ENABLED_RULES.get("R04_COMPACT_DOXYGEN", False):
        violations.extend(check_r04_compact_doxygen(filepath, lines))
    if ENABLED_RULES.get("R05_NO_IF_0", False):
        violations.extend(check_r05_no_if_0(filepath, lines))
    if ENABLED_RULES.get("R06_TYPEDEF_NAMING", False):
        violations.extend(check_r06_typedef_naming(filepath, lines))
    if ENABLED_RULES.get("R07_POINTER_PREFIX", False):
        violations.extend(check_r07_pointer_prefix(filepath, lines))
    if ENABLED_RULES.get("R08_FUNC_NAMING", False):
        violations.extend(check_r08_func_naming(filepath, lines))
    if ENABLED_RULES.get("R09_BRANCH_COMMENTS", False):
        violations.extend(check_r09_branch_comments(filepath, lines))
    if ENABLED_RULES.get("R10_TABSIZE_4", False):
        violations.extend(check_r10_tabsize_4(filepath, lines))
    if ENABLED_RULES.get("R11_BLANK_LINE_BEFORE_COMMENT", False):
        violations.extend(check_r11_blank_line_before_comment(filepath, lines))
    if ENABLED_RULES.get("R12_SPELL_CHECK", False):
        violations.extend(check_r12_spell_check(filepath, lines))

    return violations


def main(argv: Sequence[str]) -> int:
    """CLI entry point invoked by pre-commit with staged filenames."""
    if len(argv) < 2:
        print("Usage: python tools/check_coding_rules.py <file1.c> [file2.h ...]")
        return 0

    all_violations: List[Violation] = []
    for file_arg in argv[1:]:
        all_violations.extend(check_file(file_arg))

    if all_violations:
        print("=" * 108)
        print(f"ZonalECU-FWv2 Coding Rule Violations Found ({len(all_violations)}):")
        print("Refer to `CodingRules.md` for detailed rules and examples.")
        print("=" * 108)
        for v in all_violations:
            print(f"  {v.format()}")
        print("=" * 108)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
