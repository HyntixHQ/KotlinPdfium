# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.3] - 2026-01-26

### Changed
- **License Change**: Relicensed from MIT to **GNU Affero General Public License v3.0 (AGPL-3.0)** for enhanced protection and community involvement.

## [1.0.2] - 2026-01-24

### Added
- New web link detection API for matching and extracting URLs from text-based PDFs.

## [1.0.1] - 2026-01-18

### Changed
- Improved build transparency with internal Gradle wrapper and `settings.gradle.kts`.
- Standardized JitPack build configuration.
- Cleaned up redundant type conversions in `PdfiumCore.kt`.

## [1.0.0] - 2026-01-17

### Added
- Initial release
- Core document operations (open, close, save, create)
- Page rendering to Android Bitmap
- Text extraction and search
- Bookmark/Table of Contents support
- Annotation reading
- Link detection and navigation
- Form filling support (read/write)
- Digital signature information
- Embedded file attachments
- Structure tree for accessibility
- Page import/export between documents
- Coordinate mapping (device ↔ page)

### Technical
- Pure Kotlin API with JNI bindings
- Pre-built PDFium v145.0.7616.0
- arm64-v8a architecture support
- Min SDK 26, Target SDK 36
