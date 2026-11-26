#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ==============================================================================
# Script: format_rules.py
# Purpose: Automated source code reformatter complying with ZonalECU-FWv2 rules:
#          - Aligns 110-character section banners
#          - Formats indentation and wrapping using clang-format
#          - Aligns macro backslashes (\) cleanly
#          - Renames identifiers in place with exact origin tracking comment:
#            /* RENAMED: <old> -> <new> | FILE: <declared_file> */
#          - Generalized suffix engine (_t, _e, _st) and prefix engine (p_, g_, s_)
# ==============================================================================

import os
import re
import sys
import shutil
import difflib
import subprocess

BANNER_WIDTH = 130

# Global registry mapping identifier -> filename where declared
GLOBAL_DECL_REGISTRY = {}


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

    prefixes.sort(key=len, reverse=True)
    return {"prefixes": prefixes, "reserved": reserved}


REGISTRY = load_keywords_registry()


def to_snake_case_core(name: str) -> str:
    """Convert PascalCase / camelCase / UPPER_SNAKE string to lowercase snake_case."""
    s = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", name)
    s = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1_\2", s)
    s = re.sub(r"_+", "_", s)
    return s.lower()


def normalize_type_identifier(ident: str) -> str:
    """
    Generalized type normalizer for custom types ending in _t, _e, _st.
    Preserves registered subsystem prefixes from keywords.yml.
    """
    m = re.match(r"^(.+)_(t|e|st)$", ident, re.IGNORECASE)
    if not m:
        return ident
    base = m.group(1)
    suffix = m.group(2).lower()

    prefix_matched = None
    remainder = base
    for p in REGISTRY["prefixes"]:
        p_pattern = rf"^{re.escape(p)}_?"
        pm = re.match(p_pattern, base, re.IGNORECASE)
        if pm:
            prefix_matched = p.lower()
            remainder = base[pm.end():]
            break

    if prefix_matched:
        clean_remainder = to_snake_case_core(remainder)
        if clean_remainder:
            return f"{prefix_matched}_{clean_remainder}_{suffix}"
        return f"{prefix_matched}_{suffix}"

    return f"{to_snake_case_core(base)}_{suffix}"


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


# Pointer qualifier macros recognized as pointer declarations
PTR_QUALIFIERS_REGEX = (
    r"ro_ptr|rw_ptr|wo_ptr|"
    r"RO_PTR|RW_PTR|WO_PTR|"
    r"__ro_ptr|__rw_ptr|__wo_ptr|"
    r"__RO_PTR|__RW_PTR|__WO_PTR"
)

# Pointer parameter declaration pattern: Type * var_name or Type ptr_* var_name followed by , ) ; =
PTR_DECL_PATTERN = re.compile(
    rf"(?:(?<![/(a-zA-Z0-9_])\*(?:\s*(?:const|volatile|\b(?:{PTR_QUALIFIERS_REGEX})\b))?|\b(?:{PTR_QUALIFIERS_REGEX})\b)\s*(?:const\s+|volatile\s+)?([a-zA-Z_][a-zA-Z0-9_]*)\s*(?=[,\);=])"
)

# Custom type declaration pattern (handles both single-line and multiline typedefs)
TYPE_DECL_PATTERN = re.compile(
    r"\btypedef\s+(?:struct|enum|union)?[^{;]*?(?:\{[^}]*\})?\s*([A-Za-z0-9_]+_(?:t|e|st))\s*;",
    re.DOTALL
)

# Exception comment pattern: e.g. /*ZE_EXCEPTION(...)*/ or /* ZE_EXCEPTION(...) */
ZE_EXCEPTION_PATTERN = re.compile(r"/\*\s*ZE_EXCEPTION\s*\((.*?)\)\s*\*/")

EXCLUDE_DIRS = {"build", ".template", ".git", "doxygen_html", ".cache", "unitytest"}


def preindex_declarations(targets: list):
    """
    First pass: Scan all header (.h) files to record where custom types and globals are declared.
    Enables accurate 'FILE: <declared_file>' resolution across multiple files.
    """
    for target in targets:
        if os.path.isdir(target):
            for root, dirs, files in os.walk(target):
                dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS and not d.startswith(".")]
                for f in files:
                    if f.endswith(".h"):
                        file_path = os.path.join(root, f)
                        try:
                            with open(file_path, "r", encoding="utf-8") as fh:
                                content = fh.read()
                            for line in content.splitlines():
                                if ZE_EXCEPTION_PATTERN.search(line):
                                    continue
                                for m in TYPE_DECL_PATTERN.finditer(line):
                                    type_name = m.group(1)
                                    GLOBAL_DECL_REGISTRY[type_name] = f
                        except Exception:
                            pass
        elif os.path.isfile(target) and target.endswith(".h"):
            try:
                with open(target, "r", encoding="utf-8") as fh:
                    content = fh.read()
                for line in content.splitlines():
                    if ZE_EXCEPTION_PATTERN.search(line):
                        continue
                    for m in TYPE_DECL_PATTERN.finditer(line):
                        type_name = m.group(1)
                        GLOBAL_DECL_REGISTRY[type_name] = os.path.basename(target)
            except Exception:
                pass


def apply_naming_transformations(content: str, file_name: str) -> str:
    """
    Automatically rename non-compliant pointers and types in place, and append an
    inline tracking comment with the exact origin file:
    e.g. uint8_t * p_buf /* RENAMED: buf -> p_buf | FILE: Atomic.h */;
    """
    lines = content.splitlines()

    pointer_renames = {}
    type_renames = {}
    local_decls = {}

    # Step 1: Detect all pointer parameters/variables and types declared locally in this file,
    # as well as non-compliant types indexed globally from headers
    for t_glob, t_file in GLOBAL_DECL_REGISTRY.items():
        t_norm = normalize_type_identifier(t_glob)
        if t_norm != t_glob and t_glob not in type_renames:
            type_renames[t_glob] = t_norm
    for line in lines:
        if ZE_EXCEPTION_PATTERN.search(line):
            continue
        stripped = line.strip()
        if stripped.startswith("/*") or stripped.startswith("*") or stripped.startswith("//"):
            continue

        # Strip inline comments and string literals so comments are never mistaken for code declarations
        code_only = re.sub(r'"(?:\\.|[^"\\])*"', '""', re.sub(r"//.*$", "", re.sub(r"/\*.*?\*/", "", line)))

        if stripped.startswith("typedef"):
            for tm in TYPE_DECL_PATTERN.finditer(code_only):
                type_name = tm.group(1)
                local_decls[type_name] = file_name
                suggested_type = normalize_type_identifier(type_name)
                if suggested_type != type_name and type_name not in type_renames:
                    type_renames[type_name] = suggested_type
            continue

        for m in PTR_DECL_PATTERN.finditer(code_only):
            var_name = m.group(1)
            if var_name.endswith(("_t", "_e", "_st")) or var_name.isupper() or var_name in ("const", "volatile", "restrict", "NULL", "void") or var_name.startswith("ZE_") or var_name in REGISTRY["reserved"]:
                continue
            local_decls[var_name] = file_name
            if var_name.startswith(("p_", "pp_", "gp_", "sp_")):
                continue
            suggested = f"p_{to_snake_case_core(var_name)}"
            if suggested != var_name and var_name not in pointer_renames:
                pointer_renames[var_name] = suggested

        for tm in TYPE_DECL_PATTERN.finditer(code_only):
            type_name = tm.group(1)
            local_decls[type_name] = file_name
            suggested_type = normalize_type_identifier(type_name)
            if suggested_type != type_name and type_name not in type_renames:
                type_renames[type_name] = suggested_type

    if not pointer_renames and not type_renames:
        return content

    formatted_lines = []
    for line in lines:
        if ZE_EXCEPTION_PATTERN.search(line):
            formatted_lines.append(line)
            continue
        stripped = line.strip()
        if stripped.startswith("/*") or stripped.startswith("*") or stripped.startswith("//"):
            formatted_lines.append(line)
            continue

        new_line = line

        # Rename pointers and tag with origin declaration file
        for var_old, var_new in pointer_renames.items():
            decl_file = local_decls.get(var_old) or GLOBAL_DECL_REGISTRY.get(var_old, "unknown")
            tag = f"/* RENAMED: {var_old} -> {var_new} | FILE: {decl_file} */"

            if tag in new_line or var_new in new_line:
                continue

            decl_pattern = re.compile(rf"((?:(?<!\()\*(?:\s*(?:const|volatile|\b(?:{PTR_QUALIFIERS_REGEX})\b))?|\b(?:{PTR_QUALIFIERS_REGEX})\b)\s*(?:const\s+|volatile\s+)?){re.escape(var_old)}\b")
            if decl_pattern.search(new_line):
                new_line = decl_pattern.sub(rf"\1{var_new} {tag}", new_line)
            else:
                usage_pattern = re.compile(rf"\b({re.escape(var_old)})\b(?!\s*/\*)")
                if usage_pattern.search(new_line):
                    new_line = usage_pattern.sub(rf"{var_new} {tag}", new_line)

        # Rename types and tag with origin declaration file
        for t_old, t_new in type_renames.items():
            decl_file = local_decls.get(t_old) or GLOBAL_DECL_REGISTRY.get(t_old, "unknown")
            tag = f"/* RENAMED: {t_old} -> {t_new} | FILE: {decl_file} */"

            if tag in new_line or t_new in new_line:
                continue

            type_pattern = re.compile(rf"\b({re.escape(t_old)})\b(?!\s*/\*)")
            if type_pattern.search(new_line):
                new_line = type_pattern.sub(rf"{t_new} {tag}", new_line)

        formatted_lines.append(new_line)

    return "\n".join(formatted_lines)


def find_clang_format() -> str:
    """
    Locate the clang-format executable across platforms:
    - Checks system PATH via shutil.which.
    - On Windows, checks standard LLVM and ESP-IDF tool paths.
    """
    found = shutil.which("clang-format")
    if found:
        return found

    if sys.platform == "win32":
        candidates = [
            r"C:\Program Files\LLVM\bin\clang-format.exe",
            r"C:\Program Files (x86)\LLVM\bin\clang-format.exe",
        ]
        # Check ESP-IDF tools directory for esp-clang
        espressif_tools = r"C:\Espressif\tools"
        if os.path.isdir(espressif_tools):
            for root, _, files in os.walk(espressif_tools):
                if "clang-format.exe" in files:
                    candidates.append(os.path.join(root, "clang-format.exe"))
                    break

        for c in candidates:
            if os.path.isfile(c):
                return c

    return "clang-format"


def run_clang_format(file_path: str) -> bool:
    """
    Invoke clang-format to apply 4-space indentation and line breaks.
    """
    clang_fmt = find_clang_format()
    try:
        subprocess.run([clang_fmt, "-i", file_path], check=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running {clang_fmt} on {file_path}: {e}", file=sys.stderr)
        return False
    except FileNotFoundError:
        print(f"Warning: {clang_fmt} executable not found. Skipping AST formatting.", file=sys.stderr)
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


def align_enum_block(body_lines: list) -> list:
    """
    Format a single enum body:
    1. Sub-section divider comments (e.g. /* Reserved ranges */ without @brief/@note):
       Ensure at least 1 empty line before and after.
    2. Member lines:
       Align '=' after the longest member identifier + 1 space.
    3. Trailing comments (/*!< ... */ or /* ... */ or // ...):
       Align all trailing comments after the longest code part + 1 space.
    """
    member_pattern = re.compile(
        r"^(\s*)([A-Za-z0-9_]+)\s*(?:=\s*(.+?))?\s*(,)?\s*(/\*.*?\*/|//.*)?$"
    )
    divider_pattern = re.compile(r"^\s*/\*(?!\*).*\*/\s*$")

    # Step 1: Normalize empty lines around sub-section divider comments
    norm_lines = []
    for i, line in enumerate(body_lines):
        if divider_pattern.match(line):
            if norm_lines and norm_lines[-1].strip() != "":
                norm_lines.append("")
            norm_lines.append(line)
            if i + 1 < len(body_lines) and body_lines[i + 1].strip() != "":
                norm_lines.append("")
        else:
            norm_lines.append(line)

    # Step 2: Parse member lines
    parsed = []
    max_name_len = 0
    has_assignment = False

    for line in norm_lines:
        m = member_pattern.match(line)
        if m and m.group(2) not in ("typedef", "enum", "struct", "union"):
            indent, name, val, comma, comment = m.groups()
            if val is not None:
                has_assignment = True
            max_name_len = max(max_name_len, len(name))
            parsed.append((True, indent, name, val, comma or "", comment or ""))
        else:
            parsed.append((False, line))

    if not has_assignment and max_name_len == 0:
        return norm_lines

    # Step 3: Reconstruct code parts
    reconstructed = []
    max_code_len = 0

    for item in parsed:
        if not item[0]:
            reconstructed.append(item)
            continue
        _, indent, name, val, comma, comment = item
        if val is not None:
            code_part = f"{indent}{name.ljust(max_name_len)} = {val.strip()}{comma}"
        else:
            code_part = f"{indent}{name}{comma}"

        if comment:
            max_code_len = max(max_code_len, len(code_part))
        reconstructed.append((True, code_part, comment))

    # Step 4: Assemble with aligned comments
    out_lines = []
    for item in reconstructed:
        if not item[0]:
            out_lines.append(item[1])
            continue
        _, code_part, comment = item
        if comment:
            line = f"{code_part.ljust(max_code_len)} {comment.strip()}"
        else:
            line = code_part
        out_lines.append(line)

    return out_lines


KNOWN_HEADER_TAGS = {"@brief", "@details", "@note", "@warning", "@see", "@todo", "@bug", "@attention"}


def align_doxygen_block(raw_block_lines: list) -> list:
    """
    Format a multi-line Doxygen comment block following project conventions:
    .1. StartCol = 4 via '/** ' or ' *  '
    .2. Tag alignment for header tags (@brief, @details, @note, etc.):
        Col begins at 4 + max_header_tag_len + 1 (e.g. 13 for @details)
    .3. List characters (-, +, *) indented with continuation lines aligned to list text
    .4. Align parameter names in @param lines
    .5. Align parameter descriptions after @param
    .6. Align return values and descriptions in @retval lines
    """
    if not raw_block_lines or not re.match(r"^\s*/\*\*\s*@", raw_block_lines[0]):
        return raw_block_lines

    indent_m = re.match(r"^(\s*)", raw_block_lines[0])
    indent = indent_m.group(1) if indent_m else ""
    has_end_marker = any("*/" in l for l in raw_block_lines)

    cleaned = []
    in_verbatim = False
    for idx, line in enumerate(raw_block_lines):
        if "@verbatim" in line:
            in_verbatim = True
        elif "@endverbatim" in line:
            in_verbatim = False

        if in_verbatim or "@endverbatim" in line:
            cleaned.append({"is_verbatim": True, "raw": line.rstrip()})
            continue

        l = line.strip()
        if idx == 0:
            l = re.sub(r"^/\*\*\s*", "", l)
        else:
            l = re.sub(r"^\*\s*", "", l)
        if idx == len(raw_block_lines) - 1:
            l = re.sub(r"\s*\*/$", "", l)
        cleaned.append({"is_verbatim": False, "text": l})

    elements = []
    current_elem = None

    for item in cleaned:
        if item["is_verbatim"]:
            current_elem = None
            elements.append({"type": "verbatim", "line": item["raw"]})
            continue

        l = item["text"]
        if not l:
            continue

        tag_match = re.match(r"^(@[a-zA-Z0-9_]+)\s*(.*)$", l)
        if tag_match and tag_match.group(1) in (KNOWN_HEADER_TAGS | {"@param", "@tparam", "@retval", "@return", "@returns"}):
            tag, rest = tag_match.groups()
            if tag in ("@param", "@tparam"):
                pm = re.match(r"^(?:(\[[a-zA-Z,]+\])\s+)?([A-Za-z0-9_]+)\s*(.*)$", rest)
                if pm:
                    direction, name, desc = pm.groups()
                    current_elem = {
                        "type": "param",
                        "tag": tag,
                        "dir": direction or "",
                        "name": name,
                        "desc_lines": [desc] if desc else [],
                    }
                else:
                    current_elem = {"type": "raw", "line": l}
                elements.append(current_elem)
            elif tag in ("@retval", "@return", "@returns"):
                rm = re.match(r"^([A-Za-z0-9_]+)\s*(.*)$", rest)
                if rm and tag == "@retval":
                    val, desc = rm.groups()
                    current_elem = {
                        "type": "retval",
                        "tag": tag,
                        "val": val,
                        "desc_lines": [desc] if desc else [],
                    }
                else:
                    current_elem = {
                        "type": "return",
                        "tag": tag,
                        "desc_lines": [rest] if rest else [],
                    }
                elements.append(current_elem)
            else:
                current_elem = {
                    "type": "header",
                    "tag": tag,
                    "desc_lines": [rest] if rest else [],
                }
                elements.append(current_elem)
        else:
            if current_elem and current_elem["type"] in ("param", "retval", "return", "header"):
                current_elem["desc_lines"].append(l)
            else:
                current_elem = {"type": "desc_cont", "line": l}
                elements.append(current_elem)

    all_tags = [e["tag"] for e in elements if "tag" in e]
    max_tag_len = max([len(t) for t in all_tags] + [8])
    tag_field_width = max(9, max_tag_len + 1)

    param_elems = [e for e in elements if e["type"] == "param"]
    max_param_len = max([len(e["name"]) for e in param_elems] + [0])

    retval_elems = [e for e in elements if e["type"] == "retval"]
    max_retval_len = max([len(e["val"]) for e in retval_elems] + [0])

    out_lines = []
    is_first_line = True

    for elem in elements:
        t = elem["type"]
        if t == "verbatim":
            out_lines.append(elem["line"])
        elif t == "header":
            tag = elem["tag"]
            tag_padded = tag + (" " * (tag_field_width - len(tag)))
            desc_lines = elem["desc_lines"]

            first_text = desc_lines[0] if desc_lines else ""
            if is_first_line:
                out_lines.append(f"{indent}/** {tag_padded}{first_text}")
                is_first_line = False
            else:
                out_lines.append(f"{indent} *  {tag_padded}{first_text}")

            in_list = False
            base_spaces = " " * tag_field_width
            for cl in desc_lines[1:]:
                list_m = re.match(r"^(\s*)([-+*])\s+(.*)$", cl)
                if list_m:
                    in_list = True
                    bullet = list_m.group(2)
                    list_text = list_m.group(3)
                    out_lines.append(f"{indent} *  {base_spaces}  {bullet} {list_text}")
                elif in_list:
                    out_lines.append(f"{indent} *  {base_spaces}    {cl}")
                else:
                    out_lines.append(f"{indent} *  {base_spaces}{cl}")

        elif t == "param":
            tag = elem["tag"]
            tag_padded = tag + (" " * (tag_field_width - len(tag)))
            name = elem["name"]
            name_padded = name + (" " * (max_param_len - len(name)))
            first_text = elem["desc_lines"][0] if elem["desc_lines"] else ""
            out_lines.append(f"{indent} *  {tag_padded}{name_padded} {first_text}")

            param_cont_spaces = " " * (tag_field_width + max_param_len + 1)
            for cl in elem["desc_lines"][1:]:
                out_lines.append(f"{indent} *  {param_cont_spaces}{cl}")

        elif t == "retval":
            tag = elem["tag"]
            tag_padded = tag + (" " * (tag_field_width - len(tag)))
            val = elem["val"]
            val_padded = val + (" " * (max_retval_len - len(val)))
            first_text = elem["desc_lines"][0] if elem["desc_lines"] else ""
            out_lines.append(f"{indent} *  {tag_padded}{val_padded} {first_text}")

            retval_cont_spaces = " " * (tag_field_width + max_retval_len + 1)
            for cl in elem["desc_lines"][1:]:
                out_lines.append(f"{indent} *  {retval_cont_spaces}{cl}")

        elif t == "return":
            tag = elem["tag"]
            tag_padded = tag + (" " * (tag_field_width - len(tag)))
            first_text = elem["desc_lines"][0] if elem["desc_lines"] else ""
            out_lines.append(f"{indent} *  {tag_padded}{first_text}")
            cont_sp = " " * tag_field_width
            for cl in elem["desc_lines"][1:]:
                out_lines.append(f"{indent} *  {cont_sp}{cl}")

        elif t == "desc_cont":
            base_spaces = " " * tag_field_width
            out_lines.append(f"{indent} *  {base_spaces}{elem['line']}")

        elif t == "raw":
            out_lines.append(f"{indent} *  {elem['line']}")

    if has_end_marker and out_lines:
        if not out_lines[-1].endswith("*/"):
            out_lines[-1] = out_lines[-1].rstrip() + " */"

    return out_lines


def align_all_doxygen_comments(text: str) -> str:
    """
    Scan text for all multi-line Doxygen comments and format their content columns.
    """
    lines = text.splitlines()
    out = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        if re.match(r"^\s*/\*\*\s*@", line):
            block = [line]
            if "*/" in line:
                out.append(line)
                i += 1
                continue
            i += 1
            while i < n:
                cur = lines[i]
                block.append(cur)
                i += 1
                if "*/" in cur:
                    break
            aligned = align_doxygen_block(block)
            out.extend(aligned)
        else:
            out.append(line)
            i += 1
    return "\n".join(out)


def align_all_enums(text: str) -> str:
    """
    Find all enum definitions and align member assignments and comments.
    """
    lines = text.splitlines()
    out = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        if re.search(r"\b(enum\s+[A-Za-z0-9_]*\s*\{|typedef\s+enum\b[^{;]*\{)", line):
            out.append(line)
            body = []
            i += 1
            brace_count = 1
            while i < n:
                cur = lines[i]
                brace_count += cur.count("{") - cur.count("}")
                if brace_count == 0:
                    aligned_body = align_enum_block(body)
                    out.extend(aligned_body)
                    out.append(cur)
                    i += 1
                    break
                else:
                    body.append(cur)
                    i += 1
        else:
            out.append(line)
            i += 1
    return "\n".join(out)


STRUCT_QUALIFIERS = {
    "RO", "WO", "RW", "ro", "wo", "rw", "IN", "OUT", "INOUT", "in", "out", "inout",
    "__RO", "__WO", "__RW", "__ro", "__wo", "__rw", "__IN", "__OUT", "__INOUT", "__in", "__out", "__inout"
}

STRUCT_PTR_QUALIFIERS = {
    "RO", "WO", "RW", "ro", "wo", "rw", "RO_PTR", "WO_PTR", "RW_PTR", "ro_ptr", "wo_ptr", "rw_ptr",
    "__RO", "__WO", "__RW", "__ro", "__wo", "__rw", "__RO_PTR", "__WO_PTR", "__RW_PTR", "__ro_ptr", "__wo_ptr", "__rw_ptr", "const"
}

STRUCT_MEMBER_PATTERN = re.compile(
    r"^(\s*)"
    r"(?:(\b[a-zA-Z_][a-zA-Z0-9_]*\b)\s+)?"
    r"((?:struct\s+|enum\s+|union\s+|unsigned\s+|signed\s+)?[a-zA-Z_][a-zA-Z0-9_]*(?:\s*\*)?)"
    r"(?:\s+(\b[a-zA-Z_][a-zA-Z0-9_]*\b))?"
    r"\s+([a-zA-Z_][a-zA-Z0-9_]*)(?:\s*(\[[^\]]*\]))?\s*;"
    r"(\s*/\*.*?\*/|\s*//.*)?$"
)


def align_struct_block(body_lines: list) -> list:
    """
    Format a single struct body:
    1. Fix glue like '*RO' -> '* RO'
    2. Align columns: [prefix_qual] [type] [ptr_qual] [name;] [comment]
    3. If any member has array dimension [INDEX], align the '[' bracket column.
    """
    parsed_lines = []
    has_member = False

    for line in body_lines:
        if line.strip().startswith("//") or line.strip().startswith("/*") or not line.strip():
            parsed_lines.append({"raw": line, "is_member": False})
            continue

        fixed_line = re.sub(
            r"\*(RO|WO|RW|ro|wo|rw|ro_ptr|wo_ptr|rw_ptr|RO_PTR|WO_PTR|RW_PTR|__RO|__WO|__RW|__ro|__wo|__rw|__ro_ptr|__wo_ptr|__rw_ptr|const)\b",
            r"* \1",
            line
        )

        m = STRUCT_MEMBER_PATTERN.match(fixed_line)
        if m and "(" not in m.group(3) and "(" not in m.group(5):
            indent, qual, typ, ptr_q, name, arr, cmt = m.groups()
            if qual and qual not in STRUCT_QUALIFIERS:
                typ = qual + " " + typ
                qual = ""
            if ptr_q and ptr_q not in STRUCT_PTR_QUALIFIERS:
                typ = typ + " " + ptr_q
                ptr_q = ""
            parsed_lines.append({
                "is_member": True,
                "indent": indent,
                "qual": qual or "",
                "type": typ.strip(),
                "ptr_q": ptr_q or "",
                "name": name.strip(),
                "arr": arr.strip() if arr else "",
                "cmt": cmt.strip() if cmt else ""
            })
            has_member = True
        else:
            parsed_lines.append({"raw": line, "is_member": False})

    if not has_member:
        return body_lines

    members = [p for p in parsed_lines if p["is_member"]]
    max_qual_len = max(len(p["qual"]) for p in members) if any(p["qual"] for p in members) else 0
    max_type_len = max(len(p["type"]) for p in members)
    max_ptr_q_len = max(len(p["ptr_q"]) for p in members) if any(p["ptr_q"] for p in members) else 0

    has_arr = any(p["arr"] for p in members)
    max_name_len = max(len(p["name"]) for p in members if p["arr"]) if has_arr else 0

    code_lines = []
    for p in parsed_lines:
        if not p["is_member"]:
            code_lines.append(None)
            continue

        parts = [p["indent"]]
        if max_qual_len > 0:
            parts.append(p["qual"].ljust(max_qual_len) + " ")

        type_str = p["type"].ljust(max_type_len)
        parts.append(type_str + " ")

        if max_ptr_q_len > 0:
            ptr_q_str = p["ptr_q"].ljust(max_ptr_q_len)
            parts.append(ptr_q_str + " ")

        if p["arr"]:
            name_str = p["name"].ljust(max_name_len) + " " + p["arr"] + ";"
        else:
            name_str = p["name"] + ";"

        parts.append(name_str)
        code_lines.append("".join(parts))

    has_comments = any(p["cmt"] for p in members)
    max_code_len = max(len(code) for code, p in zip(code_lines, parsed_lines) if p.get("cmt")) if has_comments else 0

    out = []
    for p, code in zip(parsed_lines, code_lines):
        if not p["is_member"]:
            out.append(p["raw"])
            continue

        if p["cmt"]:
            out.append(f"{code.ljust(max_code_len)} {p['cmt']}")
        else:
            out.append(code)

    return out


def align_all_structs(text: str) -> str:
    """
    Find all struct/union definitions and align member types, qualifiers, and names.
    """
    lines = text.splitlines()
    out = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        if re.search(r"\b((?:struct|union)\s+[A-Za-z0-9_]*\s*\{|typedef\s+(?:struct|union)\b[^{;]*\{)", line):
            out.append(line)
            body = []
            i += 1
            brace_count = 1
            while i < n:
                cur = lines[i]
                brace_count += cur.count("{") - cur.count("}")
                if brace_count == 0:
                    aligned_body = align_struct_block(body)
                    out.extend(aligned_body)
                    out.append(cur)
                    i += 1
                    break
                else:
                    body.append(cur)
                    i += 1
        else:
            out.append(line)
            i += 1
    return "\n".join(out)


def align_typedef_block(lines: list) -> list:
    """
    Format a contiguous block of single-line typedef declarations:
    - 4-column pattern: typedef [qualifier/macro] [type/args] [name;]
      e.g. typedef ze_atomic (uint32_t) ze_atomic_uint32_t;
           typedef _Atomic   uint32_t   ze_atomic_uint32_t;
    - 3-column pattern: typedef [type] [name;] (with optional pointer '*')
      e.g. typedef uint16_t someip_service_id_t;
           typedef uint64_t * uint64ptr_t;
    - Aligns inline trailing comments.
    """
    parsed = []
    has_4_cols = False
    indent = ""
    for l in lines:
        if l.strip():
            indent = " " * (len(l) - len(l.lstrip()))
            break

    for line in lines:
        stripped = line.strip()
        if not stripped:
            parsed.append((0, "", "", "", "", ""))
            continue

        cmt_match = re.search(r"\s*(/\*.*?\*/|//.*)$", stripped)
        cmt = cmt_match.group(1).strip() if cmt_match else ""
        code = stripped[:cmt_match.start()].strip() if cmt_match else stripped

        if not code.startswith("typedef") or not code.endswith(";"):
            return lines

        inner = code[7:-1].strip()

        # Check 4-col pattern: Macro with args, e.g. ze_atomic(uint32_t) or ze_atomic (uint32_t)
        m4_macro = re.match(r"^([A-Za-z0-9_]+)\s*(\([^)]*\))\s+([A-Za-z0-9_]+)$", inner)
        # Check 4-col pattern: Qualifier/Keyword + type, e.g. _Atomic uint32_t or struct foo_s
        m4_kw = re.match(r"^(_Atomic|struct|enum|union|const|unsigned|signed)\s+([A-Za-z0-9_]+(?:\s*\*|\*\s*)?)\s+([A-Za-z0-9_]+)$", inner)
        m3_ptr = re.match(r"^([A-Za-z0-9_]+)\s*(\*)\s*([A-Za-z0-9_]+)$", inner)
        m3 = re.match(r"^(.+?)\s+([A-Za-z0-9_]+)$", inner)

        if m4_macro:
            has_4_cols = True
            parsed.append((4, "typedef", m4_macro.group(1), m4_macro.group(2), m4_macro.group(3) + ";", cmt))
        elif m4_kw:
            has_4_cols = True
            parsed.append((4, "typedef", m4_kw.group(1), m4_kw.group(2), m4_kw.group(3) + ";", cmt))
        elif m3_ptr:
            parsed.append((3, "typedef", m3_ptr.group(1), "*", m3_ptr.group(3) + ";", cmt))
        elif m3:
            parsed.append((3, "typedef", m3.group(1), "", m3.group(2) + ";", cmt))
        else:
            return lines

    valid = [p for p in parsed if p[0] != 0]
    if not valid:
        return lines

    if has_4_cols and all(p[0] in (0, 4) for p in parsed):
        w0 = max(len(p[1]) for p in valid)
        w1 = max(len(p[2]) for p in valid)
        w2 = max(len(p[3]) for p in valid)
        out = []
        max_code_len = 0
        for p in parsed:
            if p[0] == 0:
                out.append(("", ""))
                continue
            code_line = f"{indent}{p[1].ljust(w0)} {p[2].ljust(w1)} {p[3].ljust(w2)} {p[4]}"
            max_code_len = max(max_code_len, len(code_line))
            out.append((code_line, p[5]))
        res = []
        for code_line, cmt in out:
            if not code_line:
                res.append("")
            elif cmt:
                res.append(f"{code_line.ljust(max_code_len)} {cmt}")
            else:
                res.append(code_line)
        return res
    else:
        w0 = max(len(p[1]) for p in valid)
        has_ptr = any(p[3] == "*" for p in valid)
        if has_ptr:
            w1 = max(len(p[2]) for p in valid)
            out = []
            max_code_len = 0
            for p in parsed:
                if p[0] == 0:
                    out.append(("", ""))
                    continue
                if p[3] == "*":
                    code_line = f"{indent}{p[1].ljust(w0)} {p[2].ljust(w1)} * {p[4]}"
                else:
                    code_line = f"{indent}{p[1].ljust(w0)} {p[2].ljust(w1)}   {p[4]}"
                max_code_len = max(max_code_len, len(code_line))
                out.append((code_line, p[5]))
            res = []
            for code_line, cmt in out:
                if not code_line:
                    res.append("")
                elif cmt:
                    res.append(f"{code_line.ljust(max_code_len)} {cmt}")
                else:
                    res.append(code_line)
            return res
        else:
            w1 = max(len(p[2]) for p in valid)
            out = []
            max_code_len = 0
            for p in parsed:
                if p[0] == 0:
                    out.append(("", ""))
                    continue
                code_line = f"{indent}{p[1].ljust(w0)} {p[2].ljust(w1)} {p[4]}"
                max_code_len = max(max_code_len, len(code_line))
                out.append((code_line, p[5]))
            res = []
            for code_line, cmt in out:
                if not code_line:
                    res.append("")
                elif cmt:
                    res.append(f"{code_line.ljust(max_code_len)} {cmt}")
                else:
                    res.append(code_line)
            return res


def align_all_typedefs(text: str) -> str:
    """
    Find contiguous blocks of single-line typedef declarations
    (allowing single blank lines between typedef groups)
    and align them in columns (4 columns or 3 columns).
    """
    lines = text.splitlines()
    out = []
    i = 0
    n = len(lines)

    while i < n:
        line = lines[i]
        stripped = line.strip()

        if stripped.startswith("typedef") and ";" in stripped and not ("{" in stripped or "}" in stripped):
            typedef_lines = []
            while i < n:
                cur = lines[i]
                cur_str = cur.strip()
                if cur_str.startswith("typedef") and ";" in cur_str and not ("{" in cur_str or "}" in cur_str):
                    if ZE_EXCEPTION_PATTERN.search(cur):
                        break
                    typedef_lines.append(cur)
                    i += 1
                elif not cur_str and typedef_lines:
                    j = i + 1
                    while j < n and not lines[j].strip():
                        j += 1
                    if j < n and lines[j].strip().startswith("typedef") and ";" in lines[j] and not ("{" in lines[j] or "}" in lines[j]):
                        typedef_lines.append(cur)
                        i += 1
                    else:
                        break
                else:
                    break
            if len(typedef_lines) >= 1:
                aligned = align_typedef_block(typedef_lines)
                out.extend(aligned)
            else:
                out.append(line)
                i += 1
        else:
            out.append(line)
            i += 1

    return "\n".join(out)


def align_all_alias_blocks(text: str) -> str:
    """
    Find and align mixed blocks of single-line typedef and #define alias declarations:
    Aligns Column 2 (type / macro name), Column 3 (alias / macro value), and Column 4 (comments).
    """
    typedef_pat = re.compile(
        r"^(\s*)typedef\s+([A-Za-z0-9_]+(?:\s*\*|\*\s*)?)\s+([A-Za-z0-9_]+;)(?:\s*(/\*.*?\*/|//.*))?$"
    )
    define_pat = re.compile(
        r"^(\s*)#\s*define\s+([A-Za-z0-9_]+)\s+([A-Za-z0-9_]+)(?:\s*(/\*.*?\*/|//.*))?$"
    )

    lines = text.splitlines()
    out = []
    i = 0
    n = len(lines)

    while i < n:
        line = lines[i]
        m_t = typedef_pat.match(line)
        m_d = define_pat.match(line)

        if m_t or m_d:
            block_lines = []
            has_t = bool(m_t)
            has_d = bool(m_d)

            while i < n:
                cur = lines[i]
                cur_str = cur.strip()
                cur_m_t = typedef_pat.match(cur)
                cur_m_d = define_pat.match(cur)

                if cur_m_t or cur_m_d:
                    if cur_m_t:
                        has_t = True
                    if cur_m_d:
                        has_d = True
                    block_lines.append((cur, cur_m_t, cur_m_d))
                    i += 1
                elif not cur_str and block_lines:
                    j = i + 1
                    while j < n and not lines[j].strip():
                        j += 1
                    if j < n and (typedef_pat.match(lines[j]) or define_pat.match(lines[j]) or lines[j].strip().startswith("/**") or lines[j].strip().startswith("/*")):
                        if lines[j].strip().startswith("/*") and "***" in lines[j] and "Wrap" not in lines[j] and ("EXPORTED" in lines[j] or "SHARED" in lines[j]):
                            break
                        block_lines.append((cur, None, None))
                        i += 1
                    else:
                        break
                elif cur_str.startswith("/**") or (cur_str.startswith("/*") and not ("***" in cur_str and len(cur_str) > 50)):
                    block_lines.append((cur, None, None))
                    i += 1
                else:
                    break

            valid = [b for b in block_lines if b[1] or b[2]]
            if (has_t and has_d) or len(valid) > 2:
                max_col2 = 0
                for b in valid:
                    if b[1]:
                        max_col2 = max(max_col2, len(b[1].group(2).strip()))
                    else:
                        max_col2 = max(max_col2, len(b[2].group(2).strip()))

                code_parts = []
                max_code_len = 0
                for b in block_lines:
                    if not b[1] and not b[2]:
                        code_parts.append((b[0], None))
                        continue
                    if b[1]:
                        indent, c2, c3, cmt = b[1].groups()
                        code = f"{indent}typedef {c2.strip().ljust(max_col2)} {c3.strip()}"
                    else:
                        indent, c2, c3, cmt = b[2].groups()
                        code = f"{indent}#define {c2.strip().ljust(max_col2)} {c3.strip()}"
                    cmt_clean = cmt.strip() if cmt else ""
                    if cmt_clean:
                        max_code_len = max(max_code_len, len(code))
                    code_parts.append((code, cmt_clean))

                for code, cmt in code_parts:
                    if cmt is None:
                        out.append(code)
                    elif cmt:
                        out.append(f"{code.ljust(max_code_len)} {cmt}")
                    else:
                        out.append(code)
            else:
                for b in block_lines:
                    out.append(b[0])
        else:
            out.append(line)
            i += 1

    return "\n".join(out)


def align_designated_initializer_block(lines: list) -> list:
    """
    Format a struct designated initializer block (both single-line and multiline):
    - Decl line: <indent><decl> = {
    - Put each .field assignment on its own line indented by 4 spaces.
    - Align '=' signs to the longest field identifier.
    - Guarantee trailing commas for clean version control diffs.
    - Keep comments aligned.
    - Closing line: <indent>};
    """
    full_text = "\n".join(lines)
    first_brace = full_text.find("{")
    last_brace = full_text.rfind("}")
    if first_brace == -1 or last_brace == -1 or last_brace <= first_brace:
        return lines

    decl = full_text[:first_brace].strip()
    inner = full_text[first_brace + 1:last_brace]
    tail = full_text[last_brace:].strip()

    m_indent = re.match(r"^([ \t]*)", lines[0])
    indent = m_indent.group(1) if m_indent else ""
    field_indent = indent + "    "

    raw_lines = inner.splitlines()
    all_items = []
    current = []
    depth = 0
    in_str = False
    escape = False
    current_comment = ""

    for line in raw_lines:
        line_str = line.strip()
        if not line_str:
            continue
        m_cmt = re.search(r"\s*(/\*.*?\*/|//.*)$", line_str)
        comment = ""
        if m_cmt:
            comment = m_cmt.group(1).strip()
            line_str = line_str[:m_cmt.start()].strip()

        for ch in line_str:
            if escape:
                current.append(ch)
                escape = False
                continue
            if ch == "\\":
                escape = True
                current.append(ch)
                continue
            if ch == '"':
                in_str = not in_str
                current.append(ch)
                continue
            if not in_str:
                if ch in ("{", "(", "["):
                    depth += 1
                elif ch in ("}", ")", "]"):
                    depth -= 1
                elif ch == "," and depth == 0:
                    item_text = "".join(current).strip()
                    if item_text or comment:
                        all_items.append((item_text, comment))
                        comment = ""
                    current = []
                    continue
            current.append(ch)
        if comment and not current:
            all_items.append(("", comment))
        elif comment:
            current_comment = comment

    item_text = "".join(current).strip()
    if item_text or current_comment:
        all_items.append((item_text, current_comment))

    parsed = []
    for it, cmt in all_items:
        if not it and cmt:
            parsed.append(("CMT", "", "", cmt))
            continue
        it = it.strip().rstrip(",")
        m_nested = re.match(r"^((?:\[[^\]]+\]|\.[A-Za-z0-9_]+)\s*=\s*)?\{(.*)\}$", it, re.DOTALL)
        if m_nested and re.search(r"\.[A-Za-z0-9_]+\s*=", m_nested.group(2)):
            raw_prefix = m_nested.group(1) or ""
            norm_prefix = re.sub(r"\s*=\s*$", " = ", raw_prefix.strip()) if raw_prefix else ""
            nested_inner = m_nested.group(2).strip()
            single_line = f"{field_indent}{norm_prefix}{{{nested_inner}}},"
            if len(single_line) > BANNER_WIDTH:
                sub_lines = [f"{field_indent}{norm_prefix}{{", f"{field_indent}    {nested_inner}", f"{field_indent}}},"]
                aligned_sub = align_designated_initializer_block(sub_lines)
                parsed.append(("MULTI", aligned_sub, "", cmt))
                continue
        m_field = re.match(r"^(\.[A-Za-z0-9_]+)\s*=\s*(.*)$", it)
        if m_field:
            fname, fval = m_field.groups()
            parsed.append(("FIELD", fname, fval.strip(), cmt))
        else:
            parsed.append(("RAW", "", it, cmt))

    if not parsed:
        return lines

    max_fname_len = max((len(p[1]) for p in parsed if p[0] == "FIELD"), default=0)
    rebuilt = []
    for kind, fname, fval, cmt in parsed:
        if kind == "MULTI":
            rebuilt.append(("MULTI", fname, cmt))
            continue
        if kind == "FIELD":
            code_part = f"{field_indent}{fname.ljust(max_fname_len)} = {fval},"
        elif kind == "RAW" and fval:
            code_part = f"{field_indent}{fval},"
        else:
            code_part = ""

        if cmt:
            if code_part:
                rebuilt.append(("SINGLE", code_part, cmt))
            else:
                rebuilt.append(("SINGLE", f"{field_indent}{cmt}", ""))
        else:
            rebuilt.append(("SINGLE", code_part, ""))

    code_with_cmts = [r[1] for r in rebuilt if r[0] == "SINGLE" and r[1] and r[2]]
    comment_col = max((len(c) for c in code_with_cmts), default=0) + 1 if code_with_cmts else 0

    header_line = f"{indent}{decl} {{" if decl else f"{indent}{{"
    out = [header_line]
    for kind, code_part, cmt in rebuilt:
        if kind == "MULTI":
            out.extend(code_part)
        elif not cmt:
            out.append(code_part)
        elif not code_part:
            out.append(cmt)
        else:
            out.append(f"{code_part.ljust(comment_col)}{cmt}")
    out.append(f"{indent}{tail}")
    return out


def align_all_designated_initializers(text: str) -> str:
    """
    Find all designated initializer blocks (variable = { ... };) and format them with aligned '='.
    Handles both multiline initializers and single-line initializers that clang-format collapsed.
    """
    lines = text.splitlines()
    out = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        # Match single-line designated initializer
        if re.search(r"=\s*\{\s*\.[A-Za-z0-9_]+\s*=", line) and line.rstrip().endswith("};"):
            aligned = align_designated_initializer_block([line])
            out.extend(aligned)
            i += 1
            continue

        # Match start of designated initializer
        if re.search(r"=\s*\{\s*$", line) or re.search(r"=\s*\{\s*\.[A-Za-z0-9_]+", line):
            init_lines = [line]
            brace_count = line.count("{") - line.count("}")
            i += 1
            while i < n and brace_count > 0:
                cur = lines[i]
                brace_count += cur.count("{") - cur.count("}")
                init_lines.append(cur)
                i += 1
            if any(re.search(r"\.[A-Za-z0-9_]+\s*=", l) for l in init_lines) and init_lines[-1].strip().endswith("};"):
                aligned = align_designated_initializer_block(init_lines)
                out.extend(aligned)
            else:
                out.extend(init_lines)
        else:
            out.append(line)
            i += 1

    return "\n".join(out)


# Function Prototype Alignment Support
FUNC_PARAM_COLUMNS_REGEX = re.compile(
    rf"^(?:\b(in|out|inout|ro|wo|rw|shared|IN|OUT|INOUT|RO|WO|RW|SHARED|__in|__out|__inout|__ro|__wo|__rw|__shared|__IN|__OUT|__INOUT|__RO|__WO|__RW|__SHARED)\b\s+)?([A-Za-z0-9_]+(?:\s+const)?)\s+(?:\b({PTR_QUALIFIERS_REGEX})\b\s+)?([A-Za-z_][A-Za-z0-9_]*)$"
)

FUNC_PROTO_PATTERN = re.compile(
    r"^([ \t]*)([A-Za-z0-9_\*\s]+?)\s+([A-Za-z_][A-Za-z0-9_]*)\((.*)$"
)

FUNC_CONTROL_KEYWORDS = {
    "if", "for", "while", "switch", "catch", "return", "sizeof",
    "case", "do", "else", "typedef", "struct", "enum", "union"
}


def split_function_parameters(param_str: str) -> list:
    """Split comma-separated parameters honoring nested parentheses and brackets."""
    params = []
    current = []
    depth = 0
    in_str = False
    escape = False

    for char in param_str:
        if escape:
            current.append(char)
            escape = False
            continue
        if char == "\\":
            escape = True
            current.append(char)
            continue
        if char == '"':
            in_str = not in_str
            current.append(char)
            continue
        if not in_str:
            if char in ("(", "{", "["):
                depth += 1
            elif char in (")", "}", "]"):
                depth -= 1
            elif char == "," and depth == 0:
                p = "".join(current).strip()
                if p:
                    params.append(p)
                current = []
                continue
        current.append(char)

    last_p = "".join(current).strip()
    if last_p:
        params.append(last_p)
    return params


def format_single_function_declaration(
    prefix: str,
    params_str: str,
    suffix: str,
    max_col: int = BANNER_WIDTH
) -> list:
    """
    Format a function declaration:
    If prefix + params_str + suffix fits within max_col, keep as single line.
    Otherwise, wrap each parameter on its own line aligned in 4 columns:
    [dir_qual] [type] [ptr_qual] [param_name]
    and place continuation backslash '\' at column max_col for all lines except the last.
    """
    raw_params = split_function_parameters(params_str)
    if not raw_params or raw_params == ["void"]:
        single_line = f"{prefix}{params_str}{suffix}"
        return [single_line]

    single_line = f"{prefix}{', '.join(raw_params)}{suffix}"
    if len(single_line) < max_col:
        return [single_line]

    # Multiline wrapping required
    parsed_params = []
    for p in raw_params:
        p_clean = " ".join(p.split())
        m = FUNC_PARAM_COLUMNS_REGEX.match(p_clean)
        if m:
            dir_q, p_type, ptr_q, p_name = m.groups()
            parsed_params.append((dir_q or "", p_type or "", ptr_q or "", p_name or ""))
        else:
            parsed_params.append(("", p_clean, "", ""))

    max_dir = max(len(p[0]) for p in parsed_params)
    max_type = max(len(p[1]) for p in parsed_params)
    max_ptr = max(len(p[2]) for p in parsed_params)

    lines = []
    param_indent = " " * len(prefix)
    n_params = len(parsed_params)

    for idx, p in enumerate(parsed_params):
        dir_q, p_type, ptr_q, p_name = p
        parts = []
        if max_dir > 0:
            parts.append(dir_q.ljust(max_dir) if dir_q else " " * max_dir)
        if max_type > 0:
            parts.append(p_type.ljust(max_type) if p_type else " " * max_type)
        if max_ptr > 0:
            parts.append(ptr_q.ljust(max_ptr) if ptr_q else " " * max_ptr)
        if p_name:
            parts.append(p_name)

        param_formatted = " ".join([pt for pt in parts if pt])
        comma = "," if idx < n_params - 1 else ""

        if idx == 0:
            code_line = f"{prefix}{param_formatted}{comma}"
        else:
            code_line = f"{param_indent}{param_formatted}{comma}"

        if idx == n_params - 1:
            code_line = f"{code_line}{suffix}"
            lines.append(code_line)
        else:
            # Pad with spaces so '\' is at max_col (1-indexed col 130 -> 129 spaces before \)
            pad = max(1, (max_col - 1) - len(code_line))
            lines.append(f"{code_line}{' ' * pad}\\")

    return lines


def align_all_function_declarations(text: str, max_col: int = BANNER_WIDTH) -> str:
    """
    Align contiguous blocks of function prototypes / declarations in .h and .c files:
    - Normalizes '(' alignment across contiguous blocks.
    - If line fits within max_col, keeps on single line.
    - If >= max_col, wraps parameters into 4 columns with '\' continuation at column max_col.
    """
    lines = text.splitlines()
    out = []
    i = 0
    n = len(lines)

    while i < n:
        line = lines[i]
        stripped = line.strip()

        if not stripped or stripped.startswith(("/*", "*", "//", "#", "typedef")):
            out.append(line)
            i += 1
            continue

        m = FUNC_PROTO_PATTERN.match(line)
        if not m:
            out.append(line)
            i += 1
            continue

        indent, ret_type, func_name, remainder = m.groups()
        if any(k in ret_type.split() for k in FUNC_CONTROL_KEYWORDS) or func_name in FUNC_CONTROL_KEYWORDS or "=" in ret_type or "=" in func_name:
            out.append(line)
            i += 1
            continue

        # Check if single line declaration ending with ';'
        if ")" not in remainder:
            out.append(line)
            i += 1
            continue

        idx = remainder.rfind(")")
        params = remainder[:idx]
        suffix = remainder[idx:].strip()

        block = [(indent, ret_type.strip(), func_name.strip(), params, suffix)]
        i += 1

        # Check contiguous single-line declarations with same indent ending with ';'
        if suffix.endswith(";"):
            while i < n:
                next_line = lines[i]
                next_stripped = next_line.strip()
                if not next_stripped or next_stripped.startswith(("/*", "*", "//", "#", "typedef")):
                    break
                m_next = FUNC_PROTO_PATTERN.match(next_line)
                if not m_next:
                    break
                n_indent, n_ret, n_name, n_rem = m_next.groups()
                if any(k in n_ret.split() for k in FUNC_CONTROL_KEYWORDS) or n_name in FUNC_CONTROL_KEYWORDS or "=" in n_ret or "=" in n_name:
                    break
                if n_indent != indent:
                    break
                if ")" not in n_rem or not n_rem.strip().endswith(";"):
                    break
                idx_next = n_rem.rfind(")")
                n_params = n_rem[:idx_next]
                n_suffix = n_rem[idx_next:].strip()
                block.append((n_indent, n_ret.strip(), n_name.strip(), n_params, n_suffix))
                i += 1

        max_ret = max(len(b[1]) for b in block)
        max_name = max(len(b[2]) for b in block)
        for b in block:
            b_indent, b_ret, b_name, b_params, b_suffix = b
            prefix = f"{b_indent}{b_ret.ljust(max_ret)} {b_name.ljust(max_name)}("
            out.extend(format_single_function_declaration(prefix, b_params, b_suffix, max_col))

    return "\n".join(out)


def align_macro_definitions_in_section(lines: list, max_line_width: int = BANNER_WIDTH) -> list:
    """
    Align single-line #define macros in a section:
    - Matches both object-like and function-like / parameterized macros:
      #define NAME val
      #define NAME(args...) val
    - Target value column is max(len(#define + name)) + 1 across the section.
    - If #define + value + comment exceeds max_line_width, break to next line with trailing '\\'.
    - Trailing comments are aligned to max(code_length) + 1.
    """
    single_define_pattern = re.compile(
        r"^(\s*#\s*define\s+)([A-Za-z0-9_]+(?:\([^)]*\))?)(\s+)(.+?)(?:\s*(/\*.*?\*/|//.*))?$"
    )
    candidates = []
    for idx, line in enumerate(lines):
        m = single_define_pattern.match(line)
        if m and not line.rstrip().endswith("\\"):
            prefix, name, spaces, val_and_comment, comment = m.groups()
            full_val = val_and_comment.strip()
            comment_str = comment.strip() if comment else ""
            if not comment_str:
                cm = re.search(r"\s*(/\*.*?\*/|//.*)$", full_val)
                if cm:
                    comment_str = cm.group(1).strip()
                    val = full_val[:cm.start()].strip()
                else:
                    val = full_val
            else:
                val = full_val
            val = re.sub(r"\*(const|RO|WO|RW|ro|wo|rw)\b", r"* \1", val)
            candidates.append((idx, prefix, name, val, comment_str))

    if not candidates:
        return lines

    max_prefix_len = max(len(c[1] + c[2]) for c in candidates)
    target_val_col = max_prefix_len + 1

    formatted_dict = {}
    max_code_len = 0
    for idx, prefix, name, val, comment in candidates:
        name_part = prefix + name
        total_len = target_val_col + len(val) + (len(comment) + 1 if comment else 0)
        if total_len > max_line_width:
            line1 = name_part.ljust(max_line_width - 1) + "\\"
            line2 = "    " + val + (f" {comment}" if comment else "")
            formatted_dict[idx] = f"{line1}\n{line2}"
        else:
            padding = " " * (target_val_col - len(name_part))
            code_part = f"{name_part}{padding}{val}"
            if comment:
                max_code_len = max(max_code_len, len(code_part))
            formatted_dict[idx] = (code_part, comment)

    out_lines = []
    for idx, line in enumerate(lines):
        if idx in formatted_dict:
            item = formatted_dict[idx]
            if isinstance(item, str):
                out_lines.append(item)
            else:
                code_part, comment = item
                if comment:
                    out_lines.append(f"{code_part.ljust(max_code_len)} {comment}")
                else:
                    out_lines.append(code_part)
        else:
            out_lines.append(line)
    return out_lines


def align_all_macros(text: str, max_line_width: int = BANNER_WIDTH) -> str:
    """
    Split text by section banners and align #define macros within each section.
    """
    banner_pattern = re.compile(r"/\*\s*[A-Z0-9_/ ]+?\s*\*{5,}\s*\n\s*\*{5,}/\s*")

    chunks = []
    last_end = 0
    for m in banner_pattern.finditer(text):
        chunk = text[last_end:m.start()]
        chunks.append(chunk)
        chunks.append(m.group(0))
        last_end = m.end()
    chunks.append(text[last_end:])

    formatted_chunks = []
    for chunk in chunks:
        if "#define" in chunk and not banner_pattern.match(chunk):
            suffix_match = re.search(r"(\n*)$", chunk)
            suffix = suffix_match.group(1) if suffix_match else ""
            lines = chunk.splitlines()
            formatted_lines = align_macro_definitions_in_section(lines, max_line_width)
            formatted_chunks.append("\n".join(formatted_lines).rstrip("\r\n") + suffix)
        else:
            formatted_chunks.append(chunk)

    return "".join(formatted_chunks)


def align_macro_backslashes(content: str, target_col: int = BANNER_WIDTH) -> str:
    """
    Align backslashes of multi-line #define macros cleanly at target_col.
    """
    lines = content.splitlines()
    out = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        if re.match(r"^\s*#\s*define\b", line) and line.rstrip().endswith("\\"):
            macro_lines = []
            while i < n and lines[i].rstrip().endswith("\\"):
                macro_lines.append(lines[i])
                i += 1
            if i < n:
                last_line = lines[i]
                i += 1
            else:
                last_line = None

            stripped_codes = [l.rstrip()[:-1].rstrip() for l in macro_lines]
            max_code = max(len(c) for c in stripped_codes) if stripped_codes else 0
            align_col = max(target_col, max_code + 2)

            for c in stripped_codes:
                out.append(c.ljust(align_col - 1) + "\\")
            if last_line is not None:
                out.append(last_line)
        else:
            out.append(line)
            i += 1
    return "\n".join(out)


def format_preprocessor_indentation(text: str, file_name: str = "") -> str:
    """
    Format preprocessor directives with scope-aware indentation:
    - Header guard (#ifndef __..._H__, #define __..._H__, #endif) remains at column 0.
    - Top-level #include and #define remain at column 0.
    - Conditional blocks (#ifndef, #ifdef, #if) indent nested directives by 4 spaces.
    - Preprocessor directives inside functions/blocks align with the surrounding C code.
    """
    lines = text.splitlines()
    out = []

    # 1. Identify header guard if any (only in header files and strictly parameterless empty define)
    guard_macro = None
    if file_name.endswith((".h", ".hpp", ".H")) or not file_name:
        for i in range(min(35, len(lines))):
            line = lines[i]
            m = re.match(r"^\s*#\s*(?:ifndef|if\s+!defined)\s+([A-Za-z0-9_]+)", line)
            if m:
                macro = m.group(1)
                for j in range(i + 1, min(i + 15, len(lines))):
                    m_def = re.match(r"^\s*#\s*define\s+([A-Za-z0-9_]+)\s*(?:/\*.*?\*/|//.*)?$", lines[j])
                    if m_def and m_def.group(1) == macro:
                        guard_macro = macro
                        break
                if guard_macro:
                    break

    brace_depth = 0
    pp_stack = []  # list of (is_guard, base_indent)

    for line in lines:
        stripped = line.strip()

        if stripped.startswith("#"):
            pp_cmd = re.match(r"^#\s*([a-zA-Z]+)", stripped)
            cmd = pp_cmd.group(1) if pp_cmd else ""

            code_indent = brace_depth * 4

            if cmd in ("if", "ifdef", "ifndef"):
                m_guard = re.match(r"^#\s*(?:ifndef|if\s+!defined)\s+([A-Za-z0-9_]+)", stripped)
                if m_guard and m_guard.group(1) == guard_macro and not pp_stack:
                    pp_stack.append((True, 0))
                    out.append(stripped)
                    continue

                current_pp_indent = code_indent + len([x for x in pp_stack if not x[0]]) * 4
                indented_line = (" " * current_pp_indent) + stripped
                out.append(indented_line)
                pp_stack.append((False, current_pp_indent))

            elif cmd in ("elif", "else"):
                if pp_stack and pp_stack[-1][0]:
                    out.append(stripped)
                else:
                    parent_indent = pp_stack[-1][1] if pp_stack else code_indent
                    out.append((" " * parent_indent) + stripped)

            elif cmd == "endif":
                if pp_stack:
                    is_guard, parent_indent = pp_stack.pop()
                    if is_guard:
                        out.append(stripped)
                    else:
                        out.append((" " * parent_indent) + stripped)
                else:
                    out.append(stripped)

            elif cmd == "define":
                m_def = re.match(r"^#\s*define\s+([A-Za-z0-9_]+)\s*(?:/\*.*?\*/|//.*)?$", stripped)
                if m_def and m_def.group(1) == guard_macro and pp_stack and pp_stack[-1][0]:
                    out.append(stripped)
                else:
                    current_pp_indent = code_indent + len([x for x in pp_stack if not x[0]]) * 4
                    out.append((" " * current_pp_indent) + stripped)

            else:  # #include, #pragma, #error, etc.
                current_pp_indent = code_indent + len([x for x in pp_stack if not x[0]]) * 4
                out.append((" " * current_pp_indent) + stripped)

        else:
            clean_line = re.sub(r"//.*$", "", re.sub(r"/\*.*?\*/", "", line))
            open_braces = clean_line.count("{")
            close_braces = clean_line.count("}")

            effective_pp_indent = len([x for x in pp_stack if not x[0]]) * 4
            if stripped:
                if effective_pp_indent > 0:
                    leading_spaces = len(line) - len(line.lstrip())
                    if brace_depth > 0:
                        out.append((" " * (leading_spaces + effective_pp_indent)) + stripped)
                    else:
                        out.append((" " * (effective_pp_indent + leading_spaces)) + stripped)
                else:
                    out.append(line)
            else:
                out.append(line)

            brace_depth += open_braces - close_braces
            if brace_depth < 0:
                brace_depth = 0

    return "\n".join(out)


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

    file_name = os.path.basename(file_path)

    # 1. Standardize 130-character section banners
    transformed = format_section_banners(original_content)

    # 2. Rename pointers and types in place while appending inline tracking comments
    transformed = apply_naming_transformations(transformed, file_name)

    # 3. Write transformed content temporarily to disk for clang-format
    with open(file_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(transformed)

    # 4. Apply clang-format (indentation, line breaks)
    run_clang_format(file_path)

    # 5. Read back fully formatted content
    with open(file_path, "r", encoding="utf-8") as f:
        formatted_content = f.read()

    # 6. Post-processing:
    #    - Align #define macros within sections
    #    - Align single-line typedefs in columns (4-col / 3-col)
    #    - Align enum member '=' and comments
    #    - Align multi-line Doxygen comments text column
    #    - Align macro trailing backslashes '\'
    formatted_content = align_all_macros(formatted_content, BANNER_WIDTH)
    formatted_content = align_all_typedefs(formatted_content)
    formatted_content = align_all_alias_blocks(formatted_content)
    formatted_content = align_all_enums(formatted_content)
    formatted_content = align_all_structs(formatted_content)
    formatted_content = align_all_designated_initializers(formatted_content)
    formatted_content = align_all_function_declarations(formatted_content, BANNER_WIDTH)
    formatted_content = align_all_doxygen_comments(formatted_content)
    formatted_content = align_macro_backslashes(formatted_content, BANNER_WIDTH)
    formatted_content = format_preprocessor_indentation(formatted_content, file_name)

    with open(file_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(formatted_content)

    # 7. Print before/after diff report
    was_modified = print_file_diff(file_path, original_content, formatted_content)

    if not was_modified:
        print(f"  [OK] Clean: {file_path}")

    return was_modified


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 format_rules.py <file1.c> [file2.h ...]")
        sys.exit(1)

    targets = sys.argv[1:]

    # Pass 1: Pre-index declarations across all target header files
    preindex_declarations(targets)

    # Pass 2: Format files and resolve declaration origins
    total_files = 0
    modified_files = 0

    for target in targets:
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
