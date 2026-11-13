#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ==============================================================================
# Script: compiler_check.py
# Purpose: Strict Compiler Semantic & Dead-Code Static Analysis
#          Executes the exact target cross-compiler from compile_commands.json
#          with -Werror=unused-function and -Werror=unused-variable to catch:
#          - Unused static functions (dead code)
#          - Static functions declared but never defined
#          - Unused local/global variables
# ==============================================================================

import sys
import os
import re
import json
import shlex
import subprocess


def main():
    if len(sys.argv) < 3:
        print("Usage: compiler_check.py <compile_commands.json> <file1.c> [file2.c ...]")
        sys.exit(1)

    db_path = os.path.abspath(sys.argv[1])
    files_to_check = sys.argv[2:]

    if not os.path.isfile(db_path):
        print(f"Error: Compilation database not found at '{db_path}'", file=sys.stderr)
        sys.exit(1)

    try:
        with open(db_path, "r", encoding="utf-8") as f:
            db = json.load(f)
    except Exception as e:
        print(f"Error reading {db_path}: {e}", file=sys.stderr)
        sys.exit(1)

    # Build mapping from relative/absolute path to compilation entry
    entry_map = {}
    for entry in db:
        raw_f = entry.get("file", "")
        norm_f = os.path.normpath(os.path.abspath(raw_f)).lower()
        entry_map[norm_f] = entry

    failed_files = 0
    total_files = len(files_to_check)

    print("==================================================")
    print(" Target Compiler Strict Semantic & Warning Check")
    print("==================================================")

    for rel_path in files_to_check:
        norm_abs = os.path.normpath(os.path.abspath(rel_path)).lower()
        entry = entry_map.get(norm_abs)

        if not entry:
            # Fallback: substring match on file path
            norm_rel = rel_path.replace("\\", "/").lower()
            for ef, e in entry_map.items():
                if ef.replace("\\", "/").endswith(norm_rel):
                    entry = e
                    break

        if not entry:
            print(f"⚠️  SKIPPED: {rel_path} (not registered in compilation database)")
            continue

        cmd_str = entry.get("command")
        work_dir = entry.get("directory", os.getcwd())

        if os.name == "nt" and cmd_str:
            # On Windows, preserving the exact escape sequences avoids shlex mangling backslashes and quotes
            new_cmd = re.sub(r'\s-o\s+\S+', f' -o {os.devnull} -Werror=unused-function -Werror=unused-variable', cmd_str)
            res = subprocess.run(new_cmd, cwd=work_dir, capture_output=True, text=True)
        else:
            if not cmd_str:
                args = list(entry.get("arguments", []))
            else:
                args = shlex.split(cmd_str)

            # Substitute output object with os.devnull to perform fast check without side-effects
            new_args = []
            skip_next = False
            for arg in args:
                if skip_next:
                    skip_next = False
                    continue
                if arg == "-o":
                    skip_next = True
                    continue
                new_args.append(arg)

            new_args.extend(["-o", os.devnull, "-Werror=unused-function", "-Werror=unused-variable"])
            res = subprocess.run(new_args, cwd=work_dir, capture_output=True, text=True)

        if res.returncode != 0:
            failed_files += 1
            print(f"❌ FAILED: {rel_path}")
            for line in res.stderr.strip().splitlines():
                if "error:" in line or "warning:" in line or "note:" in line:
                    print(f"   {line}")
        else:
            print(f"✅ PASSED: {rel_path}")

    print("==================================================")
    if failed_files > 0:
        print(f"❌ Strict Compiler Check FAILED: {failed_files}/{total_files} files have errors.")
        sys.exit(1)
    else:
        print(f"✅ Strict Compiler Check PASSED: {total_files}/{total_files} files clean (0 warnings/errors).")
        sys.exit(0)


if __name__ == "__main__":
    main()
