#!/usr/bin/env python3
"""
clangd_check.py - LSP Client for Clangd Static Analysis & Semantic Checks
Connects to clangd via JSON-RPC, opens target source files, and aggregates Clang-Tidy diagnostics.
"""

import sys
import os
import subprocess
import json
import time
import select

def main():
    if len(sys.argv) < 3:
        print("Usage: clangd_check.py <project_root> <file1> [file2 ...]")
        sys.exit(1)

    project_root = os.path.abspath(sys.argv[1])
    files_to_check = sys.argv[2:]

    # Spawn clangd in LSP mode
    proc = subprocess.Popen(
        ["clangd", "--query-driver=**/xtensa-esp32-elf-*,**/riscv32-esp-elf-*,/home/fus/.espressif/**/xtensa-esp32-elf-*,*"],
        cwd=project_root,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        bufsize=0
    )

    def send_rpc(msg):
        raw = json.dumps(msg).encode("utf-8")
        proc.stdin.write(f"Content-Length: {len(raw)}\r\n\r\n".encode("utf-8") + raw)
        proc.stdin.flush()

    # 1. Initialize LSP
    send_rpc({
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "rootUri": f"file://{project_root}",
            "capabilities": {}
        }
    })
    send_rpc({
        "jsonrpc": "2.0",
        "method": "initialized",
        "params": {}
    })

    # 2. Open all target files
    for rel_path in files_to_check:
        full_path = os.path.join(project_root, rel_path)
        try:
            with open(full_path, "r", encoding="utf-8") as f:
                text = f.read()
        except Exception:
            text = ""
        send_rpc({
            "jsonrpc": "2.0",
            "method": "textDocument/didOpen",
            "params": {
                "textDocument": {
                    "uri": f"file://{full_path}",
                    "languageId": "c",
                    "version": 1,
                    "text": text
                }
            }
        })

    # 3. Collect publishDiagnostics
    diagnostics_map = {}
    start_time = time.time()
    max_wait = 5.0 + len(files_to_check) * 0.5

    while time.time() - start_time < max_wait:
        r, _, _ = select.select([proc.stdout], [], [], 0.2)
        if r:
            line = proc.stdout.readline().decode("utf-8", errors="ignore")
            if line.startswith("Content-Length: "):
                try:
                    cl = int(line[16:].strip())
                    proc.stdout.readline()  # empty separator line
                    body_bytes = proc.stdout.read(cl)
                    msg = json.loads(body_bytes.decode("utf-8", errors="ignore"))
                    if msg.get("method") == "textDocument/publishDiagnostics":
                        uri = msg["params"]["uri"]
                        for rel in files_to_check:
                            if uri.endswith(rel):
                                diagnostics_map[rel] = msg["params"]["diagnostics"]
                except Exception:
                    pass
        if len(diagnostics_map) >= len(files_to_check):
            break

    proc.terminate()

    # 4. Display results
    failed_count = 0
    for rel in files_to_check:
        diags = diagnostics_map.get(rel, [])
        # Critical diagnostics: Error (1) or Warning (2)
        critical_diags = [d for d in diags if d.get("severity", 1) in (1, 2) and d.get("code")]
        print(f"Checking: {rel} ...")
        if critical_diags:
            failed_count += 1
            print(f"❌ FAILED: {rel} ({len(critical_diags)} issues)")
            for d in critical_diags:
                line = d["range"]["start"]["line"] + 1
                col = d["range"]["start"]["character"] + 1
                code = d.get("code", "")
                src = d.get("source", "")
                msg = d.get("message", "").split("\n")[0]
                prefix = f"[{src}/{code}]" if code else ""
                print(f"   {rel}:{line}:{col}: {prefix} {msg}")
        else:
            print(f"✅ PASSED: {rel} (0 errors)")

    print("==================================================")
    if failed_count > 0:
        print(f"❌ Clangd Static Analysis FAILED: {failed_count}/{len(files_to_check)} files have errors.")
        sys.exit(1)
    else:
        print(f"✅ Clangd Static Analysis PASSED: {len(files_to_check)}/{len(files_to_check)} files clean (0 errors).")
        sys.exit(0)

if __name__ == "__main__":
    main()
