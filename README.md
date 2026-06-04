# KotlinPdfium

[![License: AGPL-3.0](https://img.shields.io/badge/License-AGPL--3.0-blue.svg)](LICENSE)
[![Android](https://img.shields.io/badge/Platform-Android-green.svg)](https://developer.android.com)
[![API](https://img.shields.io/badge/API-26%2B-brightgreen.svg)](https://android-arsenal.com/api?level=26)
[![PDFium](https://img.shields.io/badge/PDFium-150.0.7869.0-orange.svg)](https://pdfium.googlesource.com/pdfium/)

A pure Kotlin/JNI wrapper for [PDFium](https://pdfium.googlesource.com/pdfium/) on Android (arm64-v8a). Provides a clean, idiomatic Kotlin API covering 190+ PDFium functions for rendering, text extraction, annotations, form filling, editing, and more.

## Features

- **Document Operations** — Open, create, save, merge, and import pages across documents
- **Page Rendering** — High-quality bitmap rendering with configurable DPI and clipping
- **Text Extraction** — Full-text extraction with character positioning, bounding boxes, and font info
- **Text Search** — Find matches with per-result rects and count
- **Web Links** — Auto-detect and extract URLs from rendered text
- **Annotations** — Read, create, and modify annotations (highlight, underline, note, etc.)
- **Interactive Forms** — Full form-filling event model with undo/redo support
- **Page Editing** — Create and manipulate text, image, and path objects
- **Bookmarks** — Navigate the document outline tree
- **Signatures** — Read digital signature metadata and contents
- **Attachments** — Access embedded files with metadata
- **Accessibility** — Structure tree (tagged PDF) with alt text and expansion
- **Thumbnails** — Extract decoded and raw thumbnail data
- **Catalog** — Get/set document language metadata
- **Progressive Rendering** — Render pages incrementally with status callbacks

## Installation

### Via JitPack

**Step 1.** Add the JitPack repository to your root `settings.gradle.kts`:

```kotlin
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
        maven { url = uri("https://jitpack.io") }
    }
}
```

**Step 2.** Add the dependency:

```kotlin
dependencies {
    implementation("com.github.HyntixHQ:KotlinPdfium:1.0.5")
}
```

### Manual (Module)

```kotlin
// settings.gradle.kts
include(":KotlinPdfium")
project(":KotlinPdfium").projectDir = file("path/to/KotlinPdfium")

// app/build.gradle.kts
dependencies {
    implementation(project(":KotlinPdfium"))
}
```

## Quick Start

```kotlin
import com.hyntix.pdfium.PdfiumCore

// Initialize once (typically in Application.onCreate)
PdfiumCore.initLibrary()

// Open a document
val document = PdfiumCore.openDocument("/path/to/file.pdf")
document?.use { doc ->
    println("Pages: ${doc.pageCount}")

    // Render a page to a bitmap
    doc.openPage(0).use { page ->
        val bitmap = Bitmap.createBitmap(
            page.width.toInt(),
            page.height.toInt(),
            Bitmap.Config.ARGB_8888
        )
        page.render(bitmap)
    }

    // Extract text
    doc.openPage(0).use { page ->
        page.openTextPage().use { textPage ->
            println("Text: ${textPage.text}")
        }
    }
}

// Clean up when done (typically in Application.onTerminate)
PdfiumCore.destroyLibrary()
```

## API Overview

### Core Classes

| Class | Description |
|---|---|
| `PdfiumCore` | Library lifecycle, document loading, low-level JNI bridge |
| `PdfDocument` | Open, create, save, and manage PDF documents |
| `PdfPage` | Page rendering, coordinate mapping, page object access |
| `PdfTextPage` | Text extraction, character info, search, web links |
| `PdfBookmark` | Table-of-contents tree navigation |
| `PdfAnnotation` | Annotation metadata and properties |
| `PdfLink` | Hyperlink destinations and actions |
| `PdfWebLinks` | Auto-detected URL list with bounding rects |
| `PdfAttachment` | Embedded file access |
| `PdfSignature` | Digital signature metadata |

### Opening Documents

```kotlin
val doc = PdfiumCore.openDocument("/path/to/file.pdf")
val doc = PdfiumCore.openDocument(fd)          // from file descriptor
val doc = PdfiumCore.openDocument(byteArray)   // from bytes
val doc = PdfiumCore.openDocument(path, password = "secret")
val doc = PdfiumCore.newDocument()             // create blank
```

### Rendering Pages

```kotlin
doc.openPage(pageIndex).use { page ->
    // Full-page render at native resolution
    val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
    page.render(bitmap)

    // Partial render with annotations
    page.render(bitmap, startX = 0, startY = 0,
                drawWidth = 500, drawHeight = 700, renderAnnot = true)
}
```

### Text Operations

```kotlin
page.openTextPage().use { textPage ->
    val count = textPage.charCount
    val text = textPage.extractText(startIndex = 0, count = 100)

    val matches = textPage.search("keyword", matchCase = false)
    matches.forEach { match ->
        val rects = textPage.getTextRects(match.startIndex, match.count)
    }

    textPage.loadWebLinks().use { webLinks ->
        for (i in 0 until webLinks.count) {
            val url = webLinks.getURL(i)
        }
    }
}
```

### Page Editing

```kotlin
// Create a text object
val textObj = doc.newTextObject("Helvetica", 12f)
textObj.setText("Hello PDF")
doc.openPage(0).use { page ->
    page.insertObject(textObj.handle)
    page.generateContent()
}

// Create and draw a path
val path = doc.createNewPath(100f, 100f)
path.lineTo(200f, 100f)
path.lineTo(150f, 200f)
path.close()
```

### Bookmarks

```kotlin
doc.getTableOfContents().forEach { bookmark ->
    println("${bookmark.title} → Page ${bookmark.pageIndex}")
    bookmark.children.forEach { child -> /* recurse */ }
}
```

## Requirements

| Requirement | Version |
|---|---|
| **Min SDK** | 26 (Android 8.0) |
| **Target SDK** | 36 |
| **Architecture** | arm64-v8a |
| **NDK** | 30.0.14904198 |
| **JVM** | 21 |
| **PDFium** | 150.0.7869.0 (chromium/7869) |

## Architecture

```
┌──────────────────────────────────┐
│        Kotlin API Layer          │
│  PdfDocument, PdfPage, ...       │
├──────────────────────────────────┤
│          PdfiumCore              │
│     (JNI external methods)       │
├──────────────────────────────────┤
│        pdfium_jni.cpp            │
│    (159 C++ JNI bindings)        │
├──────────────────────────────────┤
│        libpdfium.so              │
│   (Pre-built, NDK r30, no V8)   │
└──────────────────────────────────┘
```

## Building

```bash
./gradlew assembleRelease
```

The pre-built PDFium binary lives at `pdfium-android-arm64/lib/libpdfium.so` (6.1 MB, built from [pdfium-binaries](https://github.com/bblanchon/pdfium-binaries)).

## License

This project is licensed under the **GNU Affero General Public License v3.0** — see [LICENSE](LICENSE).

PDFium and its dependencies are distributed under their respective open-source licenses — see [pdfium-android-arm64/licenses/](pdfium-android-arm64/licenses/).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Acknowledgments

- [PDFium](https://pdfium.googlesource.com/pdfium/) — The PDF rendering engine by Google / Foxit
- [pdfium-binaries](https://github.com/bblanchon/pdfium-binaries) — Pre-built PDFium binaries by Benoit Blanchon
