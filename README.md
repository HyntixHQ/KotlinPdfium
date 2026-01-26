# KotlinPdfium

[![License: AGPL-3.0](https://img.shields.io/badge/License-AGPL--3.0-blue.svg)](LICENSE)
[![Android](https://img.shields.io/badge/Platform-Android-green.svg)](https://developer.android.com)
[![API](https://img.shields.io/badge/API-26%2B-brightgreen.svg)](https://android-arsenal.com/api?level=26)

A pure Kotlin/JNI wrapper for [PDFium](https://pdfium.googlesource.com/pdfium/) on Android. Provides a clean, idiomatic Kotlin API for PDF operations including rendering, text extraction, annotations, form filling, and more.

## Features

- 📄 **Document Operations** - Open, create, save, and merge PDFs
- 🎨 **Rendering** - High-quality page rendering to Android Bitmaps
- 📝 **Text Extraction** - Extract text with position information
- 🔍 **Search** - Full-text search with match highlighting
- 📑 **Bookmarks** - Navigate table of contents
- ✏️ **Annotations** - Read annotation data
- 📋 **Forms** - Interactive form filling support
- 🔐 **Signatures** - Read digital signature information
- 📎 **Attachments** - Access embedded files
- 🔗 **Web Links** - Auto-detect and extract URLs from text
- ♿ **Accessibility** - Structure tree for tagged PDFs

## Installation

### Via JitPack (Recommended)

1. Add the JitPack repository to your root `settings.gradle.kts`:

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

2. Add the dependency to your app level `build.gradle.kts`:

```kotlin
dependencies {
    implementation("com.github.HyntixHQ:KotlinPdfium:1.0.3")
}
```

### Manual Installation

Add the library module to your project:

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
    
    // Render a page
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
            println(textPage.text)
        }
    }
}

// Clean up when done (typically in Application.onTerminate)
PdfiumCore.destroyLibrary()
```

## API Overview

### Core Classes

| Class | Description |
|-------|-------------|
| `PdfiumCore` | Library initialization and document loading |
| `PdfDocument` | Represents an open PDF document |
| `PdfPage` | Single page with rendering and coordinate mapping |
| `PdfTextPage` | Text extraction and search |
| `PdfBookmark` | Table of contents entry |
| `PdfAnnotation` | Annotation data |
| `PdfLink` | Hyperlink information |
| `PdfWebLinks` | Detected web links (URLs) from text |
| `PdfAttachment` | Embedded file attachment |
| `PdfSignature` | Digital signature information |

### Opening Documents

```kotlin
// From file path
val doc = PdfiumCore.openDocument("/path/to/file.pdf")

// From file descriptor
val doc = PdfiumCore.openDocument(fd)

// From byte array
val doc = PdfiumCore.openDocument(byteArray)

// With password
val doc = PdfiumCore.openDocument(path, password = "secret")

// Create new document
val doc = PdfiumCore.newDocument()
```

### Rendering Pages

```kotlin
doc.openPage(pageIndex).use { page ->
    // Full page render
    val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
    page.render(bitmap)
    
    // Partial render with annotations
    page.render(bitmap, startX = 0, startY = 0, drawWidth = 500, drawHeight = 700, renderAnnot = true)
}
```

### Text Operations

```kotlin
page.openTextPage().use { textPage ->
    // Get all text
    val fullText = textPage.text
    
    // Get character count
    val count = textPage.charCount
    
    // Extract range
    val partial = textPage.extractText(startIndex = 0, count = 100)
    
    // Search
    val matches = textPage.search("keyword", matchCase = false)
    matches.forEach { match ->
        val rects = textPage.getTextRects(match.startIndex, match.count)
    }
    
    // Web Link Detection
    textPage.loadWebLinks().use { webLinks ->
        for (i in 0 until webLinks.count) {
            val url = webLinks.getURL(i)
            val rects = webLinks.getRects(i, textPage)
        }
    }
}
```

### Bookmarks

```kotlin
val bookmarks = doc.getTableOfContents()
bookmarks.forEach { bookmark ->
    println("${bookmark.title} -> Page ${bookmark.pageIndex}")
    // Recursively process bookmark.children
}
```

## Architecture

```
┌─────────────────────────────────────┐
│         Kotlin API Layer            │
│  PdfDocument, PdfPage, PdfTextPage  │
├─────────────────────────────────────┤
│           PdfiumCore                │
│      (JNI method declarations)      │
├─────────────────────────────────────┤
│         pdfium_jni.cpp              │
│     (C++ JNI implementations)       │
├─────────────────────────────────────┤
│         libpdfium.so                │
│    (Pre-built PDFium binary)        │
└─────────────────────────────────────┘
```

## Requirements

- **Min SDK**: 26 (Android 8.0)
- **Target SDK**: 36
- **Architecture**: arm64-v8a only
- **16KB Page Size**: Supported (Android 15+ compatible)
- **NDK**: 29.0.14206865
- **JVM**: 21

## Building

```bash
./gradlew :KotlinPdfium:assembleRelease
```

The library uses a pre-built PDFium binary (v145.0.7616.0) located in `pdfium-android-arm64/lib/`.

## License

This project is licensed under the GNU Affero General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

### Third-Party Licenses

PDFium and its dependencies are licensed under various open-source licenses. See the [pdfium-android-arm64/licenses/](pdfium-android-arm64/licenses/) directory for details.

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Acknowledgments

- [PDFium](https://pdfium.googlesource.com/pdfium/) - The PDF rendering engine by Google/Foxit
- [pdfium-binaries](https://github.com/bblanchon/pdfium-binaries) - Pre-built PDFium binaries by Benoit Blanchon
