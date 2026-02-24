# GridShield Fuzzing

Fuzz testing for the GridShield packet parser using [LibFuzzer](https://llvm.org/docs/LibFuzzer.html).

## Overview

The fuzz harness feeds random byte sequences into `SecurePacket::parse()` to discover:

- Buffer overflows and out-of-bounds reads
- Null pointer dereferences
- Undefined behavior (integer overflow, shift errors)
- Assertion failures in malformed packet handling

## Prerequisites

- **clang** 14+ (with LibFuzzer built-in)
- **cmake** 3.20+

### Ubuntu/Debian

```bash
sudo apt install clang cmake
```

### macOS

```bash
brew install llvm cmake
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
```

## Build

```bash
cd firmware/fuzz
cmake -B build -S . \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

## Run

```bash
# Quick run (1M iterations, max input 2KB)
./build/fuzz_packet_parse -max_len=2048 -runs=1000000

# Longer run with corpus directory
mkdir -p corpus
./build/fuzz_packet_parse corpus/ -max_len=2048

# Run with specific timeout per iteration (5 seconds)
./build/fuzz_packet_parse -max_len=2048 -timeout=5
```

## Corpus Management

LibFuzzer saves interesting inputs to the corpus directory. To minimize the corpus:

```bash
mkdir -p corpus_min
./build/fuzz_packet_parse -merge=1 corpus_min corpus/
```

## What Gets Tested

| Component | Function | Purpose |
|-----------|----------|---------|
| `SecurePacket` | `parse()` | Validates header magic, payload length, checksum, and signature from arbitrary bytes |

## Sanitizers

The build enables three sanitizers:

- **AddressSanitizer (ASan)** — Detects buffer overflows, use-after-free, memory leaks
- **UndefinedBehaviorSanitizer (UBSan)** — Detects integer overflow, null dereference, alignment errors
- **LibFuzzer** — Coverage-guided mutation fuzzing engine

## Interpreting Results

- **No crashes after 1M+ runs**: Good confidence in parser robustness
- **Crash found**: The crashing input is saved to `crash-<hash>`. Reproduce with:

```bash
./build/fuzz_packet_parse crash-<hash>
```
