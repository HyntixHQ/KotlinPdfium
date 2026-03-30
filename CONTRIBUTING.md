# Contributing to KotlinPdfium

Thank you for your interest in contributing! This document provides guidelines for contributing to KotlinPdfium.

## Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md).

## How to Contribute

### Reporting Bugs

1. Check existing [Issues](../../issues) to avoid duplicates
2. Use the bug report template
3. Include:
   - Android version and device
   - Library version
   - Minimal reproduction steps
   - Expected vs actual behavior
   - Stack traces if applicable

### Suggesting Features

1. Open an issue with the feature request template
2. Describe the use case and expected behavior
3. Consider if it aligns with the library's scope

### Pull Requests

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature`
3. Make your changes
4. Write/update tests if applicable
5. Ensure the build passes: `./gradlew build`
6. Commit with clear messages
7. Push and open a Pull Request

## Development Setup

### Prerequisites

- Android Studio Ladybug or newer
- NDK 29.0.14206865
- JDK 21

### Building

```bash
git clone https://github.com/hyntix/KotlinPdfium.git
cd KotlinPdfium
./gradlew assembleDebug
```

### Project Structure

```
KotlinPdfium/
├── src/main/
│   ├── java/com/hyntix/pdfium/  # Kotlin API
│   └── cpp/                      # JNI bindings
├── pdfium-android-arm64/         # Pre-built PDFium
└── features.md                   # Feature tracking
```

## Coding Guidelines

### Kotlin

- Follow [Kotlin coding conventions](https://kotlinlang.org/docs/coding-conventions.html)
- Use meaningful names
- Add KDoc for public APIs
- Prefer immutability

### C++/JNI

- Follow existing code style
- Check for null pointers
- Release JNI resources properly
- Log errors with appropriate tags

### Commits

- Use clear, descriptive commit messages
- Reference issues when applicable: `Fix #123`
- Keep commits focused and atomic

## Adding New PDFium Features

1. Check if the function exists in `pdfium-android-arm64/include/`
2. Add JNI binding in `pdfium_jni.cpp`
3. Add Kotlin wrapper in appropriate class
4. Update `features.md`
5. Add tests if possible

## Questions?

Open an issue with the question label or start a discussion.
