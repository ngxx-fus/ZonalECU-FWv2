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


# Pointer parameter declaration pattern: Type * var_name followed by , ) ; =
PTR_DECL_PATTERN = re.compile(
    r"(\b[a-zA-Z0-9_]+(?:\s+const)?)\s*\*\s*([a-zA-Z0-9_]+)\s*(?=[,\);=])"
)

# Custom type declaration pattern (handles both single-line and multiline typedefs)
TYPE_DECL_PATTERN = re.compile(
    r"\btypedef\s+(?:struct|enum|union)?[^{;]*?(?:\{[^}]*\})?\s*([A-Za-z0-9_]+_(?:t|e|st))\s*;",
    re.DOTALL
)

EXCLUDE_DIRS = {"build", ".template", ".git", "doxygen_html", ".cache"}


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
                            for m in TYPE_DECL_PATTERN.finditer(content):
                                type_name = m.group(1)
                                GLOBAL_DECL_REGISTRY[type_name] = f
                        except Exception:
                            pass
        elif os.path.isfile(target) and target.endswith(".h"):
            try:
                with open(target, "r", encoding="utf-8") as fh:
                    content = fh.read()
                for m in TYPE_DECL_PATTERN.finditer(content):
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
        stripped = line.strip()
        if stripped.startswith("/*") or stripped.startswith("*") or stripped.startswith("//"):
            continue

        for m in PTR_DECL_PATTERN.finditer(line):
            var_name = m.group(2)
            if var_name in ("const", "volatile", "restrict", "NULL", "void"):
                continue
            local_decls[var_name] = file_name
            if var_name.startswith(("p_", "pp_", "gp_", "sp_")):
                continue
            suggested = f"p_{to_snake_case_core(var_name)}"
            if suggested != var_name and var_name not in pointer_renames:
                pointer_renames[var_name] = suggested

        for tm in TYPE_DECL_PATTERN.finditer(line):
            type_name = tm.group(1)
            local_decls[type_name] = file_name
            suggested_type = normalize_type_identifier(type_name)
            if suggested_type != type_name and type_name not in type_renames:
                type_renames[type_name] = suggested_type

    if not pointer_renames and not type_renames:
        return content

    formatted_lines = []
    for line in lines:
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

            decl_pattern = re.compile(rf"(\*\s*){re.escape(var_old)}\b")
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
        r"^(\s*)([A-Za-z0-9_]+)\s*(?:=\s*([^,/\n]+?))?\s*(,)?\s*(/\*.*?\*/|//.*)?$"
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
    "RO", "WO", "RW", "ro", "wo", "rw", "PTR_RO", "PTR_WO", "PTR_RW", "ptr_ro", "ptr_wo", "ptr_rw",
    "__RO", "__WO", "__RW", "__ro", "__wo", "__rw", "__PTR_RO", "__PTR_WO", "__PTR_RW", "__ptr_ro", "__ptr_wo", "__ptr_rw", "const"
}

STRUCT_MEMBER_PATTERN = re.compile(
    r"^(\s*)"
    r"(?:(\b[a-zA-Z_][a-zA-Z0-9_]*\b)\s+)?"
    r"((?:struct\s+|enum\s+|union\s+|unsigned\s+|signed\s+)?[a-zA-Z_][a-zA-Z0-9_]*(?:\s*\*)?)"
    r"(?:\s+(\b[a-zA-Z_][a-zA-Z0-9_]*\b))?"
    r"\s+([a-zA-Z_][a-zA-Z0-9_]*(?:\[[^\]]*\])?)\s*;"
    r"(\s*/\*.*?\*/|\s*//.*)?$"
)


def align_struct_block(body_lines: list) -> list:
    """
    Format a single struct body:
    1. Fix glue like '*RO' -> '* RO'
    2. Align columns: [prefix_qual] [type] [ptr_qual] [name;] [comment]
    """
    parsed_lines = []
    has_member = False

    for line in body_lines:
        if line.strip().startswith("//") or line.strip().startswith("/*") or not line.strip():
            parsed_lines.append({"raw": line, "is_member": False})
            continue

        fixed_line = re.sub(
            r"\*(RO|WO|RW|ro|wo|rw|ptr_ro|ptr_wo|ptr_rw|PTR_RO|PTR_WO|PTR_RW|__RO|__WO|__RW|__ro|__wo|__rw|__ptr_ro|__ptr_wo|__ptr_rw|const)\b",
            r"* \1",
            line
        )

        m = STRUCT_MEMBER_PATTERN.match(fixed_line)
        if m and "(" not in m.group(3) and "(" not in m.group(5):
            indent, qual, typ, ptr_q, name, cmt = m.groups()
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

    out = []
    for p in parsed_lines:
        if not p["is_member"]:
            out.append(p["raw"])
            continue

        parts = [p["indent"]]
        if max_qual_len > 0:
            parts.append(p["qual"].ljust(max_qual_len) + " ")

        type_str = p["type"].ljust(max_type_len)
        parts.append(type_str + " ")

        if max_ptr_q_len > 0:
            ptr_q_str = p["ptr_q"].ljust(max_ptr_q_len)
            parts.append(ptr_q_str + " ")

        parts.append(p["name"] + ";")
        if p["cmt"]:
            parts.append(" " + p["cmt"])
        out.append("".join(parts))

    return out


def align_all_structs(text: str) -> str:
    """
    Find all struct definitions and align member types, qualifiers, and names.
    """
    lines = text.splitlines()
    out = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        if re.search(r"\b(struct\s+[A-Za-z0-9_]*\s*\{|typedef\s+struct\b[^{;]*\{)", line):
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



def align_macro_definitions_in_section(lines: list, max_line_width: int = BANNER_WIDTH) -> list:
    """
    Align single-line #define macros in a section:
    - Target value column is max(len(#define + name)) + 1
    - If #define + value + comment exceeds max_line_width, break to next line with trailing '\\'
    - Trailing comments are aligned to max(code_length) + 1
    """
    single_define_pattern = re.compile(
        r"^(\s*#\s*define\s+)([A-Za-z0-9_]+)(\s+)(.+?)(?:\s*(/\*.*?\*/|//.*))?$"
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


def format_preprocessor_indentation(text: str) -> str:
    """
    Format preprocessor directives with scope-aware indentation:
    - Header guard (#ifndef __..._H__, #define __..._H__, #endif) remains at column 0.
    - Top-level #include and #define remain at column 0.
    - Conditional blocks (#ifndef, #ifdef, #if) indent nested directives by 4 spaces.
    - Preprocessor directives inside functions/blocks align with the surrounding C code.
    """
    lines = text.splitlines()
    out = []

    # 1. Identify header guard if any (within first 35 lines)
    guard_macro = None
    for i in range(min(35, len(lines))):
        line = lines[i]
        m = re.match(r"^\s*#\s*(?:ifndef|if\s+!defined)\s+([A-Za-z0-9_]+)", line)
        if m:
            macro = m.group(1)
            for j in range(i + 1, min(i + 15, len(lines))):
                m_def = re.match(r"^\s*#\s*define\s+([A-Za-z0-9_]+)", lines[j])
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
                m_def = re.match(r"^#\s*define\s+([A-Za-z0-9_]+)", stripped)
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
                if effective_pp_indent > 0 and brace_depth > 0:
                    leading_spaces = len(line) - len(line.lstrip())
                    out.append((" " * (leading_spaces + effective_pp_indent)) + stripped)
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
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(transformed)

    # 4. Apply clang-format (indentation, line breaks)
    run_clang_format(file_path)

    # 5. Read back fully formatted content
    with open(file_path, "r", encoding="utf-8") as f:
        formatted_content = f.read()

    # 6. Post-processing:
    #    - Align #define macros within sections
    #    - Align enum member '=' and comments
    #    - Align multi-line Doxygen comments text column
    #    - Align macro trailing backslashes '\'
    formatted_content = align_all_macros(formatted_content, BANNER_WIDTH)
    formatted_content = align_all_enums(formatted_content)
    formatted_content = align_all_structs(formatted_content)
    formatted_content = align_all_doxygen_comments(formatted_content)
    formatted_content = align_macro_backslashes(formatted_content, BANNER_WIDTH)
    formatted_content = format_preprocessor_indentation(formatted_content)

    with open(file_path, "w", encoding="utf-8") as f:
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
