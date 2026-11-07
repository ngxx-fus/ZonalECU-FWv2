#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ==============================================================================
# Script: format_rules.py
# Purpose: Automated source code reformatter complying with ZonalECU-FWv2 rules:
#          - Aligns 110-character section banners
#          - Replaces outdated types with canonical snake_case_t types
#          - Enforces Renesas RA FSP pointer parameter naming (p_*)
#          - Formats indentation and wrapping using clang-format
# ==============================================================================

import os
import re
import sys
import subprocess

BANNER_WIDTH = 110

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
    # Regex matching: /* <NAME> ****...****\n *...**/
    pattern = re.compile(
        r"/\*\s*([A-Z0-9_/ ]+?)\s*\*{5,}\s*\n\s*\*{5,}/\s*",
        re.MULTILINE
    )

    def banner_repl(match):
        title = match.group(1).strip()
        # Header line: /* <TITLE> <asterisks>
        prefix = f"/* {title} "
        remaining_stars = BANNER_WIDTH - len(prefix)
        if remaining_stars < 3:
            remaining_stars = 3
        top_line = prefix + ("*" * remaining_stars)
        # Footer line:  <space><asterisks>/
        bot_line = " " + ("*" * (BANNER_WIDTH - 2)) + "/"
        return f"\n\n{top_line}\n{bot_line}\n\n"

    return pattern.sub(banner_repl, content)


def apply_type_replacements(content: str) -> str:
    """
    Replace non-compliant PascalCase types with snake_case_t types.
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


def format_file(file_path: str) -> bool:
    """
    Process and reformat a single C or header file.
    """
    if not os.path.isfile(file_path):
        print(f"Error: File '{file_path}' not found.", file=sys.stderr)
        return False

    print(f"Formatting: {file_path}")

    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Step 1: Standardize section banners
    content = format_section_banners(content)

    # Step 2: Canonical types
    content = apply_type_replacements(content)

    # Step 3: Renesas FSP pointer parameters
    content = apply_pointer_parameter_renaming(content)

    with open(file_path, "w", encoding="utf-8") as f:
        f.write(content)

    # Step 4: Run clang-format for indentation and braces
    success = run_clang_format(file_path)

    print(f"Finished formatting: {file_path}")
    return success


EXCLUDE_DIRS = {"build", ".template", ".git", "doxygen_html", ".cache"}


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 format_rules.py <file1.c> [file2.h ...]")
        sys.exit(1)

    for target in sys.argv[1:]:
        if os.path.isdir(target):
            for root, dirs, files in os.walk(target):
                # Prune excluded directories in-place so os.walk does not traverse them
                dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS and not d.startswith(".")]
                for f in files:
                    if f.endswith((".c", ".h")):
                        format_file(os.path.join(root, f))
        else:
            format_file(target)


if __name__ == "__main__":
    main()
