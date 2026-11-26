#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
extract_magic_numbers.py

Scans C/C++ source or header files for magic numbers (excluding comments, strings,
preprocessor directives, and standard values like 0/1), extracts them into standardized
project macros:
    #define __FILENAME__MAGIC_NUMBER_XY   (<magic_number>)
and places them either in the /* MACROS ... */ comment section or right after the last
#include directive. Replaces occurrences in the source code with the generated macro name.
"""

import argparse
import os
import re
import sys
from typing import List, Tuple, Dict, Set


DEFAULT_IGNORED = {
    '0', '1', '2',
    '0U', '1U', '2U',
    '0u', '1u', '2u',
    '0UL', '1UL', '2UL',
    '0ul', '1ul', '2ul',
    '0ULL', '1ULL', '2ULL',
    '0ull', '1ull', '2ull',
    '0L', '1L', '2L',
    '0l', '1l', '2l',
    '0LL', '1LL', '2LL',
    '0ll', '1ll', '2ll',
    '0x0', '0x1', '0x00', '0x01',
    '0x0U', '0x1U', '0x0u', '0x1u',
}


def sanitize_filename_to_macro_prefix(file_path: str) -> str:
    """
    Convert a filename like 'ESP-FreeRTOS.c' into 'ESP_FREERTOS_C_'
    """
    basename = os.path.basename(file_path)
    clean = re.sub(r'[^A-Za-z0-9]', '_', basename).upper()
    return f"{clean}_"


def tokenize_c_code(text: str) -> List[Tuple[str, str, int]]:
    """
    Tokenizes C code into (type, value, start_pos) tuples.
    Types:
      - 'BLOCK_COMMENT'
      - 'LINE_COMMENT'
      - 'STRING'
      - 'CHAR'
      - 'PP_DIRECTIVE'
      - 'IDENTIFIER'
      - 'NUMBER'
      - 'OTHER'
    """
    tokens = []
    token_pattern = re.compile(
        r'(?P<BLOCK_COMMENT>/\*[\s\S]*?\*/)'
        r'|(?P<LINE_COMMENT>//[^\r\n]*)'
        r'|(?P<STRING>"(\\.|[^"\\])*")'
        r'|(?P<CHAR>\'(\\.|[^\'\\])*\')'
        r'|(?P<PP_DIRECTIVE>^[ \t]*#[^\r\n]*)'
        r'|(?P<HEX_BIN_OCT_NUMBER>\b0[xX][0-9a-fA-F]+[uUlL]*\b|\b0[bB][01]+[uUlL]*\b|\b0[0-7]+[uUlL]*\b)'
        r'|(?P<FLOAT_NUMBER>\b\d+\.\d+([eE][+-]?\d+)?[fFlL]?\b)'
        r'|(?P<DEC_NUMBER>\b\d+[uUlL]*\b)'
        r'|(?P<IDENTIFIER>\b[a-zA-Z_][a-zA-Z0-9_]*\b)'
        r'|(?P<OTHER>[\s\S])',
        re.MULTILINE
    )

    for m in token_pattern.finditer(text):
        for kind in [
            'BLOCK_COMMENT', 'LINE_COMMENT', 'STRING', 'CHAR', 'PP_DIRECTIVE',
            'HEX_BIN_OCT_NUMBER', 'FLOAT_NUMBER', 'DEC_NUMBER', 'IDENTIFIER', 'OTHER'
        ]:
            val = m.group(kind)
            if val is not None:
                if kind in ('HEX_BIN_OCT_NUMBER', 'FLOAT_NUMBER', 'DEC_NUMBER'):
                    tokens.append(('NUMBER', val, m.start()))
                else:
                    tokens.append((kind, val, m.start()))
                break

    return tokens


def process_file(file_path: str, dry_run: bool = False, ignored_nums: Set[str] = None) -> bool:
    if ignored_nums is None:
        ignored_nums = set(DEFAULT_IGNORED)

    if not os.path.isfile(file_path):
        print(f"[ERROR] File not found: {file_path}", file=sys.stderr)
        return False

    with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    prefix = sanitize_filename_to_macro_prefix(file_path)
    tokens = tokenize_c_code(content)

    # 1. Discover magic numbers to extract
    magic_num_map: Dict[str, str] = {}  # literal_value -> macro_name
    counter = 1

    for kind, val, _ in tokens:
        if kind == 'NUMBER':
            if val in ignored_nums:
                continue
            if val not in magic_num_map:
                macro_name = f"{prefix}MAGIC_NUM_{counter:02d}"
                magic_num_map[val] = macro_name
                counter += 1

    if not magic_num_map:
        print(f"[INFO] No magic numbers found in {file_path}")
        return False

    print(f"[INFO] Found {len(magic_num_map)} magic number(s) in {file_path}:")
    for val, name in magic_num_map.items():
        print(f"       {name} = ({val})")

    # 2. Reconstruct code with macro replacements
    new_code_parts = []
    for kind, val, _ in tokens:
        if kind == 'NUMBER' and val in magic_num_map:
            new_code_parts.append(magic_num_map[val])
        else:
            new_code_parts.append(val)

    modified_code = "".join(new_code_parts)

    # 3. Format macro declaration block
    macro_lines = []
    for val, name in magic_num_map.items():
        macro_lines.append(f"#define {name} ({val})")

    macros_block_str = "\n".join(macro_lines) + "\n"

    # 4. Insert macro block at proper section
    # Strategy A: Check for existing /* MACROS ... */ section banner
    macro_section_pattern = re.compile(
        r'(/\*\s*MACROS[\s\S]*?\*/)',
        re.MULTILINE
    )

    match = macro_section_pattern.search(modified_code)
    if match:
        insert_pos = match.end()
        # If line ends with newline, advance past newline
        if insert_pos < len(modified_code) and modified_code[insert_pos] == '\r':
            insert_pos += 1
        if insert_pos < len(modified_code) and modified_code[insert_pos] == '\n':
            insert_pos += 1

        final_code = modified_code[:insert_pos] + "\n" + macros_block_str + modified_code[insert_pos:]
        print("       [PLACED] Inserted into '/* MACROS ... */' comment section.")
    else:
        # Strategy B: Fallback - insert after the last #include line
        include_matches = list(re.finditer(r'^[ \t]*#include[^\r\n]*', modified_code, re.MULTILINE))
        if include_matches:
            last_inc = include_matches[-1]
            insert_pos = last_inc.end()
            # If line ends with newline, insert after it
            if insert_pos < len(modified_code) and modified_code[insert_pos] == '\r':
                insert_pos += 1
            if insert_pos < len(modified_code) and modified_code[insert_pos] == '\n':
                insert_pos += 1

            final_code = (
                modified_code[:insert_pos]
                + "\n/* MACROS */\n"
                + macros_block_str
                + modified_code[insert_pos:]
            )
            print("       [PLACED] Inserted after the last #include directive.")
        else:
            # Fallback C: At the very top
            final_code = macros_block_str + "\n" + modified_code
            print("       [PLACED] Inserted at the top of the file.")

    if dry_run:
        print("[DRY-RUN] File not modified. Pass without --dry-run to apply.")
        return True

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(final_code)

    print(f"[SUCCESS] Updated {file_path}")
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Extract magic numbers into standardized project macros."
    )
    parser.add_argument("files", nargs="+", help="C/C++ file(s) to process")
    parser.add_argument(
        "--dry-run", action="store_true", help="Preview changes without modifying files"
    )
    parser.add_argument(
        "--include-zero-one",
        action="store_true",
        help="Include 0, 1, 2 as magic numbers (default is to ignore them)"
    )

    args = parser.parse_args()

    ignored = set() if args.include_zero_one else set(DEFAULT_IGNORED)

    for target in args.files:
        process_file(target, dry_run=args.dry_run, ignored_nums=ignored)


if __name__ == "__main__":
    main()
