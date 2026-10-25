# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.8] - 2026-10-25

### Changed
- **PDFium 158 Upgrade**: Updated PDFium engine from 150.0.7869.0 to **158.0.8111.0** (chromium/8111).
- **Gradle 9.8.1** — upgraded from 9.4.1 (compatible with AGP 9.4.1+).
- **AGP 9.4.1** — upgraded from 9.1.0.
- **AndroidX core-ktx 1.19.1** — upgraded from 1.18.0.
- **NDK 30.0.16248370** — upgraded from 30.0.14904198 (latest LTS).
- **compileSdk 37** — upgraded from 36 (API 37 support).

### Added
- **Attachment APIs**: `attachmentSetDescription()`, `attachmentGetDescription()`, `attachmentGetAFRelationship()`
- **Bookmark APIs**: `bookmarkGetColor()`, `bookmarkGetStyle()`
- **Image Object Pattern APIs**: `imageObjGetRenderedStrokePattern()`, `imageObjGetRenderedFillPattern()`
- **Path Bezier APIs**: `pathGetBezierControlPoints()`
- **Form Text Direction**: `formGetTextDirection()`, `formSetTextDirection()` with constants `FPDF_TEXTDIR_UNKNOWN`, `FPDF_TEXTDIR_AUTO`, `FPDF_TEXTDIR_LTR`, `FPDF_TEXTDIR_RTL`
- **Structure Element**: `structElementGetRawType()`

### Technical
- 410 JNI bindings, 410 external funs, 408 public funs
- AAR: 3.3 MB (debug)
- Binary: 6.6 MB (libpdfium.so)
- arm64-v8a only, minSdk 26

## [1.0.7] - 2026-06-04

### Added
- **Full fpdf_dataavail.h bindings**: `createAvail`, `destroyAvail`, `availGetDocument`, `availGetFirstPageNum`, `availIsDocAvail`, `availIsPageAvail`, `availIsFormAvail` — all with buffer-based `FPDF_FILEAVAIL`/`FPDF_FILEACCESS` callback infrastructure.
- **fpdf_ext.h**: `setUnSpObjProcessHandler` with `JNI_OnLoad` + `UNSUPPORT_INFO` callback dispatching to Kotlin static method.
- **Full fpdf_sysfontinfo.h bindings**: `getDefaultTTFMapCount`, `getDefaultTTFMapEntry`, `getDefaultTTFMap`, `addInstalledFont`, `getDefaultSystemFontInfo`, `freeDefaultSystemFontInfo`, `setSystemFontInfo`.
- **fpdfview.h**: `openDocument64` (`FPDF_LoadMemDocument64`), `loadCustomDocument` (`FPDF_LoadCustomDocument` with `FPDF_FILEACCESS` callback).
- **fpdf_edit.h**: Upgraded `LoadJpegFile`, `LoadJpegFileInline`, `GetImageMetadata` from stubs to real implementations with `FPDF_FILEACCESS`/`FPDF_IMAGEOBJ_METADATA` struct support.

### Fixed
- **Duplicate JNI symbol**: Removed dead-code `nativeImageObjSetBitmap(imageObjPtr, bitmap: Any)` overload that caused linker conflicts with the real implementation.
- **`nativeDestGetView` return type**: JNI return type corrected from `jboolean` to `jlong` to match Kotlin `Long` declaration.
- **`nativeAnnotGetFocusableSubtypes` buffer bug**: JNI was calling `NewIntArray` instead of writing to the caller's passed-in `subtypes` array.
- **Missing public wrapper**: Added `annotGetFocusableSubtypes()` returning `IntArray?`.
- **Phase 8+ PDFium 150 API mismatches**: Fixed 27 compilation errors across 20 object/edit functions:
  - `FPDFAnnot_GetVertices`/`FPDFAnnot_GetInkListPath`: buffer type fixed to `FS_POINTF*`.
  - `FPDFPageObj_CreateNew`/`CreateNewRect`/`CreateTextObj`: corrected signatures (removed non-existent function, added rect coords, added font_size).
  - `FPDFPageObj_TransformF`: scalar args packed into `FS_MATRIX*` struct.

