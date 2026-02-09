# Contributing to GridShield

Thank you for your interest in contributing to GridShield! This document provides guidelines for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [How to Contribute](#how-to-contribute)
- [Pull Request Process](#pull-request-process)
- [Coding Standards](#coding-standards)
- [Commit Message Guidelines](#commit-message-guidelines)
- [Testing](#testing)
- [Documentation](#documentation)

## Code of Conduct

This project adheres to the Contributor Covenant [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code.

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/gridshield.git
   cd gridshield
   ```
3. **Add upstream remote**:
   ```bash
   git remote add upstream https://github.com/ORIGINAL_OWNER/gridshield.git
   ```
4. **Create a branch** for your feature/fix:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Setup

### Prerequisites

- CMake ≥ 3.20
- C++17 compiler (GCC ≥ 7.0, Clang ≥ 5.0, MSVC 2017+)
- Arduino CLI ≥ 1.4.1 (for AVR builds)
- Git

### Build for Development

```bash
# Native development build with sanitizers
cmake --preset native-debug
cmake --build --preset native-debug

# Run tests (if applicable)
./bin/NATIVE/GridShield
```

See [BUILD.md](BUILD.md) for detailed build instructions.

## How to Contribute

### Reporting Bugs

Before submitting a bug report:
- Check the [existing issues](https://github.com/gridshield/issues)
- Verify it's not a security vulnerability (use [SECURITY.md](SECURITY.md) instead)

**Bug Report Template**:
```markdown
### Description
Clear description of the bug

### Steps to Reproduce
1. Step 1
2. Step 2
3. ...

### Expected Behavior
What should happen

### Actual Behavior
What actually happens

### Environment
- OS: [e.g., Windows 10, Ubuntu 22.04]
- Compiler: [e.g., GCC 11.2]
- CMake Version: [e.g., 3.25]
- Build Type: [e.g., native-debug, avr-mega]

### Additional Context
Screenshots, logs, etc.
```

### Suggesting Features

Feature requests are welcome! Please:
- Check if it aligns with project goals (AMI security)
- Search existing feature requests
- Provide clear use case and benefits

**Feature Request Template**:
```markdown
### Problem Statement
What problem does this solve?

### Proposed Solution
How should it work?

### Alternatives Considered
Other approaches you've thought about

### Additional Context
Any relevant context
```

## Pull Request Process

### 1. Before You Code

- **Discuss major changes** in an issue first
- **Check existing PRs** to avoid duplication
- **Read the codebase** to understand conventions

### 2. During Development

- Keep changes **focused and atomic** (one feature per PR)
- Write **clear commit messages** (see below)
- Add **tests** if applicable
- Update **documentation** if needed
- Follow **coding standards** (see below)

### 3. Submitting PR

1. **Update your branch** with latest upstream:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Run checks locally**:
   ```bash
   # Build and test
   cmake --preset native-debug
   cmake --build --preset native-debug
   
   # Format check (if formatter is configured)
   # clang-format -i src/**/*.cpp include/**/*.hpp
   ```

3. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

4. **Open Pull Request** on GitHub with:
   - **Title**: Concise summary (`Add tamper detection for multi-sensor arrays`)
   - **Description**: 
     - What does this PR do?
     - Why is it needed?
     - How was it tested?
     - Related issues (e.g., `Fixes #123`)
   
   **PR Template**:
   ```markdown
   ### Changes
   - Added X
   - Fixed Y
   - Updated Z

   ### Motivation
   Why this change is needed

   ### Testing
   How it was tested

   ### Checklist
   - [ ] Code follows style guidelines
   - [ ] Self-reviewed the code
   - [ ] Commented complex sections
   - [ ] Updated documentation
   - [ ] No new warnings
   - [ ] Added tests (if applicable)
   - [ ] All tests pass
   ```

### 4. Review Process

- Maintainers will review your PR
- Address feedback with new commits (don't force-push during review)
- Once approved, maintainer will merge (squash merge preferred)

## Coding Standards

### C++17 Compliance

- **Standard**: C++17 (no C++20/23 features)
- **Compatibility**: Must compile on GCC 7+, Clang 5+, MSVC 2017+
- **No exceptions**: Use `Result<T>` monad for error handling

### Style Guide

#### Naming Conventions

```cpp
// Classes, Structs, Enums: PascalCase
class GridShieldSystem { };
struct TamperConfig { };
enum class ErrorCode { };

// Functions, Variables: snake_case
void initialize_system();
uint32_t packet_count;

// Constants, Macros: UPPER_SNAKE_CASE
constexpr size_t MAX_BUFFER_SIZE = 512;
#define GS_MAKE_ERROR(code) ...

// Namespaces: lowercase
namespace gridshield::core { }
```

#### File Organization

```cpp
// Header (.hpp)
#pragma once

#include "dependencies.hpp"  // External first
#include "core/types.hpp"    // Internal after

namespace gridshield {
namespace module {

class MyClass {
public:
    // Public interface first
    MyClass() noexcept;
    void public_method() noexcept;
    
private:
    // Private implementation
    void private_helper() noexcept;
    int member_variable_;
};

} // namespace module
} // namespace gridshield
```

#### Formatting

- **Indentation**: 4 spaces (no tabs)
- **Line Length**: 100 characters max
- **Braces**: Allman style for functions, K&R for control flow
  ```cpp
  void function()  // Allman
  {
      if (condition) {  // K&R
          // code
      }
  }
  ```
- **Spacing**: Space after keywords, around operators
  ```cpp
  if (x == y) { }    // Good
  if(x==y){}         // Bad
  ```

#### Best Practices

```cpp
// Use const/constexpr where possible
constexpr size_t SIZE = 10;
void process(const Data& data) noexcept;

// Prefer noexcept for move/destructors
MyClass(MyClass&& other) noexcept;
~MyClass() noexcept;

// Use GS_MOVE for clarity (defined in gs_macros.hpp)
return GS_MOVE(result);

// Check errors explicitly
auto result = operation();
if (result.is_error()) {
    return result.error();
}

// Comment non-obvious code
// Calculate checksum using modified FNV-1a algorithm
uint32_t checksum = compute_hash(data);
```

### Platform-Specific Code

```cpp
// Use platform macros
#if GS_PLATFORM_NATIVE
    #include <iostream>
    std::cout << "Native build\n";
#elif GS_PLATFORM_ARDUINO
    #include <Arduino.h>
    Serial.println("Arduino build");
#endif
```

### Memory Management

```cpp
// NO heap allocation in core library
// Use StaticBuffer<T, N> instead of std::vector<T>
core::StaticBuffer<uint8_t, 256> buffer;

// NO exceptions - use Result<T>
core::Result<void> operation() noexcept;

// NO raw pointers for ownership - use RAII
class Resource {
    ~Resource() noexcept { cleanup(); }
};
```

## Commit Message Guidelines

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style (formatting, no logic change)
- `refactor`: Code restructuring (no behavior change)
- `perf`: Performance improvement
- `test`: Add/update tests
- `build`: Build system changes
- `ci`: CI/CD changes
- `chore`: Maintenance tasks

### Examples

```bash
# Good commits
feat(tamper): add multi-sensor support for physical security layer
fix(crypto): prevent buffer overflow in AES-GCM encryption
docs(readme): update build instructions for Arduino Mega
refactor(core): simplify error handling in system orchestrator

# Bad commits
update stuff
fixed bug
wip
```

### Scope

Use module names:
- `core`, `security`, `hardware`, `network`, `analytics`
- `platform`, `arduino`, `native`
- `build`, `docs`, `tests`

## Testing

### Manual Testing

```bash
# Native build
cmake --preset native-debug
cmake --build --preset native-debug
./bin/NATIVE/GridShield

# Arduino build (verify only)
cmake --preset avr-mega
cmake --build --preset avr-mega
```

### Unit Tests (Future)

Once test framework is added:
```bash
cmake --preset native-debug -DBUILD_TESTS=ON
cmake --build --preset native-debug
ctest --preset native-debug
```

### Hardware-in-Loop Testing

For Arduino contributions:
1. Upload to actual hardware
2. Test all affected features
3. Monitor serial output
4. Document results in PR

## Documentation

### When to Update Docs

- **Always**: Public API changes
- **Usually**: New features, behavior changes
- **Sometimes**: Refactoring (if architecture changes)

### Documentation Files

- `README.md`: Update if core features change
- `docs/API.md`: Update for public API changes
- `docs/ARCHITECTURE.md`: Update for structural changes
- `BUILD.md`: Update for build process changes
- `docs/CHANGELOG.md`: Update for every release

### Code Comments

```cpp
/**
 * @file myfile.hpp
 * @brief Brief description
 * @version 1.0
 * @date 2025-02-09
 * 
 * Detailed description if needed
 */

/**
 * @brief Process tamper events from hardware sensors
 * @param event Tamper event structure
 * @return Result<void> Success or error code
 */
core::Result<void> process_tamper(const TamperEvent& event) noexcept;
```

## Project Structure

```
gridshield/
├── include/
│   ├── common/          # Platform-agnostic headers
│   ├── platform/        # HAL interfaces
│   ├── native/          # PC implementation
│   └── arduino/         # AVR implementation
├── src/
│   ├── common/          # Cross-platform implementations
│   ├── native/          # PC entry point
│   └── arduino/         # Arduino entry point
├── docs/                # Documentation
├── CMakeLists.txt       # Build config
└── CMakePresets.json    # Build presets
```

## Communication

- **GitHub Issues**: Bug reports, feature requests
- **Pull Requests**: Code contributions
- **Discussions**: General questions, ideas
- **Email**: Security issues (see SECURITY.md)

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).

## Recognition

Contributors will be acknowledged in:
- `README.md` (major contributions)
- Release notes
- Project documentation

Thank you for making GridShield better! 🛡️