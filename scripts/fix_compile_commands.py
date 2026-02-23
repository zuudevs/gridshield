#!/usr/bin/env python3
"""Sanitize compile_commands.json for clang-tidy on ESP-IDF / Xtensa projects.

ESP-IDF generates compile_commands.json with GCC-specific flags and
the xtensa target triple that upstream Clang does not understand.
This script strips those entries so clang-tidy can process the code.
"""

import json
import re
import sys
from pathlib import Path

# GCC-only flags that Clang / clang-tidy does not recognise.
REMOVE_FLAGS: set[str] = {
    "-mlongcalls",
    "-fno-shrink-wrap",
    "-fno-tree-switch-conversion",
    "-fstrict-volatile-bitfields",
    "-mtext-section-literals",
}

# Regex that matches --target=xtensa-* or -target xtensa-*
_TARGET_RE = re.compile(r"--target=xtensa\S*|-target\s+xtensa\S*")


def _clean_command(cmd: str) -> str:
    """Remove unsupported flags and rewrite the target triple."""
    for flag in REMOVE_FLAGS:
        cmd = cmd.replace(f" {flag}", "")

    # Replace the Xtensa target with a generic embedded target that
    # Clang can parse (we only care about static analysis, not codegen).
    cmd = _TARGET_RE.sub("--target=arm-none-eabi", cmd)
    return cmd


def fix(path: Path) -> None:
    data = json.loads(path.read_text(encoding="utf-8"))

    for entry in data:
        if "command" in entry:
            entry["command"] = _clean_command(entry["command"])
        if "arguments" in entry:
            new_args: list[str] = []
            skip_next = False
            for i, arg in enumerate(entry["arguments"]):
                if skip_next:
                    skip_next = False
                    continue
                if arg in REMOVE_FLAGS:
                    continue
                if arg == "-target":
                    # Next arg is the triple — replace both.
                    new_args.extend(["--target=arm-none-eabi"])
                    skip_next = True
                    continue
                if arg.startswith("--target=xtensa"):
                    new_args.append("--target=arm-none-eabi")
                    continue
                new_args.append(arg)
            entry["arguments"] = new_args

    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"✓ Sanitized {path}")


def main() -> None:
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <compile_commands.json>", file=sys.stderr)
        sys.exit(1)

    p = Path(sys.argv[1])
    if not p.exists():
        print(f"Error: {p} not found", file=sys.stderr)
        sys.exit(1)

    fix(p)


if __name__ == "__main__":
    main()
