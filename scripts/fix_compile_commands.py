#!/usr/bin/env python3
"""Sanitize compile_commands.json for clang-tidy on ESP-IDF / Xtensa projects.

ESP-IDF generates compile_commands.json with:
  1. The Xtensa cross-compiler path (e.g. xtensa-esp32-elf-gcc) which
     causes clang-tidy to infer the unsupported xtensa target triple.
  2. GCC-specific flags that Clang does not understand.

This script rewrites the file so that clang-tidy can process the code.
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
    "-fno-jump-tables",
    "-Wno-frame-address",
    "-Wlogical-op",
    "-Wformat-signedness",
    "-Wformat-overflow=2",
    "-Wformat-truncation",
}

# Regex: matches the xtensa cross-compiler path (the first token in the command).
# e.g. /path/to/xtensa-esp32-elf-gcc or xtensa-esp32-elf-g++
_XTENSA_CC_RE = re.compile(r"\S*xtensa-esp\S*-(?:gcc|g\+\+|cc|c\+\+)(?:\.exe)?")

# Regex that matches --target=xtensa-* or -target xtensa-*
_TARGET_RE = re.compile(r"--target=xtensa\S*|-target\s+xtensa\S*")


def _clean_command(cmd: str) -> str:
    """Remove unsupported flags, replace compiler path, and rewrite target."""
    # Replace the Xtensa compiler with a plain gcc/g++ so clang-tidy
    # does not try to infer the xtensa target triple from the binary name.
    cmd = _XTENSA_CC_RE.sub("gcc", cmd, count=1)

    for flag in REMOVE_FLAGS:
        cmd = cmd.replace(f" {flag}", "")

    # Replace any explicit Xtensa target with a Clang-supported one.
    cmd = _TARGET_RE.sub("--target=arm-none-eabi", cmd)
    return cmd


def _clean_arguments(args: list[str]) -> list[str]:
    """Clean the arguments list variant of compile_commands entries."""
    new_args: list[str] = []
    skip_next = False
    for arg in args:
        if skip_next:
            skip_next = False
            continue
        if arg in REMOVE_FLAGS:
            continue
        if _XTENSA_CC_RE.fullmatch(arg):
            new_args.append("gcc")
            continue
        if arg == "-target":
            new_args.extend(["--target=arm-none-eabi"])
            skip_next = True
            continue
        if arg.startswith("--target=xtensa"):
            new_args.append("--target=arm-none-eabi")
            continue
        new_args.append(arg)
    return new_args


def fix(path: Path) -> None:
    data = json.loads(path.read_text(encoding="utf-8"))

    for entry in data:
        if "command" in entry:
            entry["command"] = _clean_command(entry["command"])
        if "arguments" in entry:
            entry["arguments"] = _clean_arguments(entry["arguments"])

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
