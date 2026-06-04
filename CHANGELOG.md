# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.6] - 2026-06-04

### Fixed
- **JitPack build**: Added `jitpack.yml` to install NDK 30.0.14904198 and accept licenses on the build server.

## [1.0.5] - 2026-06-04

### Added
- **PDFium 150 Upgrade**: Updated PDFium engine from 145.0.7616.0 to **150.0.7869.0** (chromium/7869).
- **Catalog Language API**: New `getCatalogLanguage()` / `setCatalogLanguage()` for reading and writing document language metadata (`FPDFCatalog_GetLanguage` / `FPDFCatalog_SetLanguage`).
- **Content Mark API**: `addExistingMark()` to attach marks to page objects (`FPDFPageObj_AddExistingMark`).
- **Text Object Font Size**: `setTextObjFontSize()` for precise text object sizing (`FPDFTextObj_SetFontSize`).
- **Text Positions**: `setTextPositions()` for character-level position adjustment (`FPDFText_SetPositions`).
- **Structure Element Expansion**: `getStructElementExpansion()` for accessibility expansion text (`FPDF_StructElement_GetExpansion`).
- **Save Flags**: Added `FPDF_REMOVE_SECURITY` (value 4, bitmask-safe), `FPDF_REMOVE_SECURITY_DEPRECATED` (value 3, legacy), and `FPDF_SUBSET_NEW_FONTS` (value 8) save flag constants.

### Changed
- **NDK 30**: Upgraded build toolchain from NDK 29.0.14206865 to **NDK 30.0.14904198** to match binary requirements.
- **`FPDFPage_InsertObject` return type**: Now returns `Boolean` (was `Unit`) — wrapping the upstream change from `void` to `FPDF_BOOL`.
- **`FPDF_LIBRARY_CONFIG`**: Updated to v5 struct with zero-initialization (`={}`).
- **Binary size**: `libpdfium.so` increased from 4.8 MB to **6.1 MB** (standard build, no V8/XFA).

### Technical
- 159 JNI bindings (was ~150)
- 158 external fun declarations, 104 public functions
- AAR size: 3.2 MB (debug)

## [1.0.4] - 2026-03-30

### Changed
- **Dependency Optimization**: Stripped all redundant Android app-specific dependencies from the library, significantly reducing transitive footprint.
- **Pure Library Focus**: Refined project configuration to align with a pure JNI/Kotlin library architecture.

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
