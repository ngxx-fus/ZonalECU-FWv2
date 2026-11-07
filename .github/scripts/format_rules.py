#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ==============================================================================
# Script: format_rules.py
# Purpose: Automated source code reformatter complying with ZonalECU-FWv2 rules:
#          - Aligns 110-character section banners
#          - Replaces outdated types with canonical snake_case_t types
#          - Enforces Renesas RA FSP pointer parameter naming (p_*)
#          - Formats indentation and wrapping using clang-format
#          - Reports exact diff: File, Original lines, Formatted lines
# ==============================================================================

import os
import re
import sys
import difflib
import subprocess

BANNER_WIDTH = 110

# Load keyword registry from keywords.yml (supports both PyYAML and fallback parser)
def load_keywords_registry(yml_path: str = None) -> dict:
    """
    Load keywords registry from keywords.yml.
    Returns a dictionary:
      'prefixes': list of strings with type 0 (e.g. ['ZE', 'ZECU', 'SOMEIP', ...])
      'reserved': list of strings with type 1
    """
    if yml_path is None:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        yml_path = os.path.join(script_dir, "keywords.yml")

    prefixes = []
    reserved = []

    if not os.path.isfile(yml_path):
        return {"prefixes": prefixes, "reserved": reserved}

    try:
        import yaml
        with open(yml_path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
            if data and "keywords" in data:
                for entry in data["keywords"]:
                    kw = entry.get("keyword")
                    t = entry.get("type")
                    if kw and t == 0:
                        prefixes.append(kw)
                    elif kw and t == 1:
                        reserved.append(kw)
    except Exception:
        # Robust fallback parser without external PyYAML dependency
        with open(yml_path, "r", encoding="utf-8") as f:
            current_kw = None
            current_type = None
            for line in f:
                line = line.strip()
                kw_match = re.search(r'keyword:\s*["\']?([^"\']+)["\']?', line)
                type_match = re.search(r'type:\s*(\d+)', line)
                if kw_match:
                    current_kw = kw_match.group(1).strip()
                if type_match:
                    current_type = int(type_match.group(1))
                    if current_kw is not None:
                        if current_type == 0:
                            prefixes.append(current_kw)
                        elif current_type == 1:
                            reserved.append(current_kw)
                        current_kw = None
                        current_type = None

    return {"prefixes": prefixes, "reserved": reserved}


REGISTRY = load_keywords_registry()

# Canonical type mappings from CodingRules.md and SomeIp_Types.h
TYPE_REPLACEMENTS = {
    r"\bSomeIp_Result_t\b": "someip_result_t",
    r"\bSomeIp_ByteOrder_t\b": "someip_byte_order_t",
    r"\bSomeIp_LengthFieldSize_t\b": "someip_length_field_size_t",
    r"\bSomeIp_StringEncoding_t\b": "someip_string_encoding_t",
    r"\bSomeIp_SerCursor_t\b": "someip_ser_cursor_t",
    r"\bSomeIp_DeserCursor_t\b": "someip_deser_cursor_t",
    # Endian helper function mappings from SomeIp_Endian.h
    r"\bSomeIp_WriteU16\b": "SOMEIP_WriteU16",
    r"\bSomeIp_WriteU32\b": "SOMEIP_WriteU32",
    r"\bSomeIp_WriteU64\b": "SOMEIP_WriteU64",
    r"\bSomeIp_ReadU16\b": "SOMEIP_ReadU16",
    r"\bSomeIp_ReadU32\b": "SOMEIP_ReadU32",
    r"\bSomeIp_ReadU64\b": "SOMEIP_ReadU64",
}

# Dynamically add function prefix uppercase normalizations for prefixes with type 0
for prefix in REGISTRY["prefixes"]:
    # E.g. If prefix is "SOMEIP", map "SomeIp_" -> "SOMEIP_" and "Someip_" -> "SOMEIP_"
    # E.g. If prefix is "ZECU", map "Zecu_" -> "ZECU_"
    pascal_prefix = "".join(word.capitalize() for word in prefix.split("_"))
    if pascal_prefix.upper() == prefix and pascal_prefix != prefix:
        TYPE_REPLACEMENTS[rf"\b{pascal_prefix}_"] = f"{prefix}_"

# Pointer parameter renaming (Renesas FSP convention: prefix with p_)
POINTER_PARAM_REPLACEMENTS = [
    (r"\bSomeIp_SerCursor_t\s*\*\s*cur\b", "someip_ser_cursor_t * p_cur"),
    (r"\bSomeIp_DeserCursor_t\s*\*\s*cur\b", "someip_deser_cursor_t * p_cur"),
    (r"\bsomeip_ser_cursor_t\s*\*\s*cur\b", "someip_ser_cursor_t * p_cur"),
    (r"\bsomeip_deser_cursor_t\s*\*\s*cur\b", "someip_deser_cursor_t * p_cur"),
    (r"\bconst\s+someip_ser_cursor_t\s*\*\s*cur\b", "const someip_ser_cursor_t * p_cur"),
    (r"\bconst\s+someip_deser_cursor_t\s*\*\s*cur\b", "const someip_deser_cursor_t * p_cur"),
    (r"\buint8_t\s*\*\s*buf\b", "uint8_t * p_buf"),
    (r"\bconst\s+uint8_t\s*\*\s*buf\b", "const uint8_t * p_buf"),
    (r"\bconst\s+uint8_t\s*\*\s*data\b", "const uint8_t * p_data"),
    (r"\buint8_t\s*\*\s*out\b", "uint8_t * p_out"),
    (r"\bchar\s*\*\s*str\b", "char * p_str"),
    (r"\bconst\s+char\s*\*\s*str\b", "const char * p_str"),
    (r"\buint32_t\s*\*\s*mark\b", "uint32_t * p_mark"),
    (r"\buint32_t\s*\*\s*bomSize\b", "uint32_t * p_bom_size"),
    (r"\buint32_t\s*\*\s*lenWritten\b", "uint32_t * p_len_written"),
    (r"\buint8_t\s*\*\s*v\b", "uint8_t * p_val"),
    (r"\buint16_t\s*\*\s*v\b", "uint16_t * p_val"),
    (r"\buint32_t\s*\*\s*v\b", "uint32_t * p_val"),
    (r"\buint64_t\s*\*\s*v\b", "uint64_t * p_val"),
    (r"\bint8_t\s*\*\s*v\b", "int8_t * p_val"),
    (r"\bint16_t\s*\*\s*v\b", "int16_t * p_val"),
    (r"\bint32_t\s*\*\s*v\b", "int32_t * p_val"),
    (r"\bint64_t\s*\*\s*v\b", "int64_t * p_val"),
    (r"\bfloat\s*\*\s*v\b", "float * p_val"),
    (r"\bdouble\s*\*\s*v\b", "double * p_val"),
]


def format_section_banners(content: str) -> str:
    """
    Standardize all C section banners to exactly 110 characters width:
    /* SECTION_NAME *************************************************************
     ****************************************************************************/
    """
    pattern = re.compile(
        r"/\*\s*([A-Z0-9_/ ]+?)\s*\*{5,}\s*\n\s*\*{5,}/\s*",
        re.MULTILINE
    )

    def banner_repl(match):
        title = match.group(1).strip()
        prefix = f"/* {title} "
        remaining_stars = BANNER_WIDTH - len(prefix)
        if remaining_stars < 3:
            remaining_stars = 3
        top_line = prefix + ("*" * remaining_stars)
        bot_line = " " + ("*" * (BANNER_WIDTH - 2)) + "/"
        return f"\n\n{top_line}\n{bot_line}\n\n"

    return pattern.sub(banner_repl, content)


def apply_type_replacements(content: str) -> str:
    """
    Replace non-compliant PascalCase types with canonical snake_case_t types.
    """
    for pattern, replacement in TYPE_REPLACEMENTS.items():
        content = re.sub(pattern, replacement, content)
    return content


def apply_pointer_parameter_renaming(content: str) -> str:
    """
    Rename pointer function parameters and struct members to use p_ prefix (Renesas FSP).
    Updates parameter names, struct member accesses (p_cur->p_buf), and call arguments.
    """
    for pattern, replacement in POINTER_PARAM_REPLACEMENTS:
        content = re.sub(pattern, replacement, content)

    # Struct member accesses: cur->buf -> p_cur->p_buf
    content = re.sub(r"->buf\b", "->p_buf", content)

    # Replace parameter and local variable names
    content = re.sub(r"\bcur\b", "p_cur", content)
    content = re.sub(r"\bbomSize\b", "p_bom_size", content)
    content = re.sub(r"\blenWritten\b", "p_len_written", content)
    content = re.sub(r"\bbuf\b", "p_buf", content)
    content = re.sub(r"\bdata\b", "p_data", content)

    return content


def run_clang_format(file_path: str) -> bool:
    """
    Invoke clang-format to apply 4-space indentation and line breaks.
    """
    try:
        subprocess.run(["clang-format", "-i", file_path], check=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running clang-format on {file_path}: {e}", file=sys.stderr)
        return False
    except FileNotFoundError:
        print("Warning: clang-format executable not found. Skipping AST formatting.", file=sys.stderr)
        return False


def print_file_diff(file_path: str, orig_text: str, formatted_text: str) -> bool:
    """
    Compare original and formatted content and display detailed changes:
    - File path
    - [Original] content
    - [Formatted] content
    Returns True if file was changed, False otherwise.
    """
    if orig_text == formatted_text:
        return False

    orig_lines = orig_text.splitlines()
    form_lines = formatted_text.splitlines()

    matcher = difflib.SequenceMatcher(None, orig_lines, form_lines)
    changes = []

    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            continue
        orig_chunk = orig_lines[i1:i2]
        form_chunk = form_lines[j1:j2]
        changes.append((i1 + 1, orig_chunk, form_chunk))

    if not changes:
        return False

    print("\n" + "=" * 90)
    print(f"FILE: {file_path}")
    print(f"STATUS: MODIFIED ({len(changes)} block(s) updated)")
    print("-" * 90)

    for line_num, orig_chunk, form_chunk in changes:
        print(f"--> Around Line {line_num}:")
        if orig_chunk:
            print("    [Original]:")
            for l in orig_chunk:
                print(f"      - {l}")
        else:
            print("    [Original]: (none / added lines)")

        if form_chunk:
            print("    [Formatted]:")
            for l in form_chunk:
                print(f"      + {l}")
        else:
            print("    [Formatted]: (none / removed lines)")
        print()

    print("=" * 90 + "\n")
    return True


def format_file(file_path: str) -> bool:
    """
    Process, reformat, and display diff report for a single C or header file.
    Returns True if file was modified, False if unchanged.
    """
    if not os.path.isfile(file_path):
        print(f"Error: File '{file_path}' not found.", file=sys.stderr)
        return False

    with open(file_path, "r", encoding="utf-8") as f:
        original_content = f.read()

    # Apply rule transformations
    transformed = original_content
    transformed = format_section_banners(transformed)
    transformed = apply_type_replacements(transformed)
    transformed = apply_pointer_parameter_renaming(transformed)

    # Write transformed content temporarily to disk for clang-format
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(transformed)

    # Apply clang-format
    run_clang_format(file_path)

    # Read back fully formatted content
    with open(file_path, "r", encoding="utf-8") as f:
        formatted_content = f.read()

    # Print before/after diff report
    was_modified = print_file_diff(file_path, original_content, formatted_content)

    if not was_modified:
        print(f"  [OK] Clean: {file_path}")

    return was_modified


EXCLUDE_DIRS = {"build", ".template", ".git", "doxygen_html", ".cache"}


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 format_rules.py <file1.c> [file2.h ...]")
        sys.exit(1)

    total_files = 0
    modified_files = 0

    for target in sys.argv[1:]:
        if os.path.isdir(target):
            for root, dirs, files in os.walk(target):
                dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS and not d.startswith(".")]
                for f in files:
                    if f.endswith((".c", ".h")):
                        total_files += 1
                        if format_file(os.path.join(root, f)):
                            modified_files += 1
        elif os.path.isfile(target):
            total_files += 1
            if format_file(target):
                modified_files += 1

    clean_files = total_files - modified_files
    print("\n" + "=" * 90)
    print("FORMATTING SUMMARY REPORT:")
    print(f"  Total files scanned : {total_files}")
    print(f"  Files modified      : {modified_files}")
    print(f"  Files clean         : {clean_files}")
    print("=" * 90 + "\n")


if __name__ == "__main__":
    main()
