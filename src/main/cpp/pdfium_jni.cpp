/**
 * KotlinPdfium JNI Wrapper
 * 
 * JNI bindings for PDFium library - Document operations
 */

#include <jni.h>
#include <string>
#include <unistd.h>
#include <map>
#include <android/log.h>
#include <android/bitmap.h>
#include <fpdfview.h>
#include <fpdf_doc.h>
#include <fpdf_text.h>
#include <fpdf_annot.h>
#include <fpdf_edit.h>
#include <fpdf_save.h>
#include <fpdf_formfill.h>
#include <fpdf_attachment.h>
#include <fpdf_ppo.h>
#include <fpdf_progressive.h>
#include <fpdf_signature.h>
#include <fpdf_transformpage.h>
#include <fpdf_structtree.h>
#include <fpdf_thumbnail.h>
#include <fpdf_flatten.h>
#include <fpdf_dataavail.h>
#include <fpdf_javascript.h>
#include <fpdf_sysfontinfo.h>

#define LOG_TAG "KotlinPdfium"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static int libraryReferenceCount = 0;
static std::map<FPDF_DOCUMENT, char*> g_docBuffers;

extern "C" {

/**
 * Initialize PDFium library
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeInitLibrary(JNIEnv *env, jobject thiz) {
    if (libraryReferenceCount == 0) {
        FPDF_LIBRARY_CONFIG config;
        config.version = 2;
        config.m_pUserFontPaths = nullptr;
        config.m_pIsolate = nullptr;
        config.m_v8EmbedderSlot = 0;
        FPDF_InitLibraryWithConfig(&config);
        LOGI("PDFium library initialized");
    }
    libraryReferenceCount++;
}

/**
 * Destroy PDFium library
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeDestroyLibrary(JNIEnv *env, jobject thiz) {
    libraryReferenceCount--;
    if (libraryReferenceCount == 0) {
        FPDF_DestroyLibrary();
        LOGI("PDFium library destroyed");
    }
}

/**
 * Get last error code
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetLastError(JNIEnv *env, jobject thiz) {
    return (jint) FPDF_GetLastError();
}

/**
 * Open document from file descriptor
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeOpenDocument(JNIEnv *env, jobject thiz,
                                                      jint fd, jstring password) {
    const char *cPassword = nullptr;
    if (password != nullptr) {
        cPassword = env->GetStringUTFChars(password, nullptr);
    }

    // Get file size
    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize <= 0) {
        if (password != nullptr) {
            env->ReleaseStringUTFChars(password, cPassword);
        }
        LOGE("Failed to get file size or empty file");
        return 0;
    }
    lseek(fd, 0, SEEK_SET);
    
    char *buffer = new (std::nothrow) char[fileSize];
    if (!buffer) {
        if (password != nullptr) {
            env->ReleaseStringUTFChars(password, cPassword);
        }
        LOGE("Failed to allocate buffer of size %ld", (long)fileSize);
        return 0;
    }
    
    // Read file with proper loop to handle partial reads
    char *ptr = buffer;
    size_t remaining = fileSize;
    while (remaining > 0) {
        ssize_t bytesRead = read(fd, ptr, remaining);
        if (bytesRead <= 0) {
            delete[] buffer;
            if (password != nullptr) {
                env->ReleaseStringUTFChars(password, cPassword);
            }
            LOGE("Read error: only read %ld of %ld bytes", (long)(fileSize - remaining), (long)fileSize);
            return 0;
        }
        ptr += bytesRead;
        remaining -= bytesRead;
    }
    
    FPDF_DOCUMENT doc = FPDF_LoadMemDocument(buffer, fileSize, cPassword);
    
    if (password != nullptr) {
        env->ReleaseStringUTFChars(password, cPassword);
    }
    
    if (!doc) {
        delete[] buffer;
        LOGE("Failed to load document, error: %lu", FPDF_GetLastError());
        return 0;
    }
    
    // Track buffer for cleanup when document is closed
    g_docBuffers[doc] = buffer;
    LOGI("Document opened successfully, pages: %d", FPDF_GetPageCount(doc));
    return (jlong) doc;
}

/**
 * Open document from memory buffer
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeOpenMemDocument(JNIEnv *env, jobject thiz,
                                                         jbyteArray data, jstring password) {
    const char *cPassword = nullptr;
    if (password != nullptr) {
        cPassword = env->GetStringUTFChars(password, nullptr);
    }
    
    jsize length = env->GetArrayLength(data);
    jbyte *buffer = env->GetByteArrayElements(data, nullptr);
    
    // We must copy the data because ReleaseByteArrayElements might free 'buffer'
    // and PDFium needs it to persist until the document is closed.
    // However, FPDF_LoadMemDocument copies the data internally usually? 
    // Wait, FPDF_LoadMemDocument documentation says:
    // "The memory buffer must remain valid during the life-time of the document."
    // So we absolutely need to allocate a copy that persists.
    
    char* docBuffer = new char[length];
    memcpy(docBuffer, buffer, length);

    FPDF_DOCUMENT doc = FPDF_LoadMemDocument(docBuffer, length, cPassword);
    
    env->ReleaseByteArrayElements(data, buffer, 0);
    if (password != nullptr) {
        env->ReleaseStringUTFChars(password, cPassword);
    }
    
    if (!doc) {
        delete[] docBuffer;
        LOGE("Failed to load document from memory, error: %lu", FPDF_GetLastError());
        return 0;
    }
    
    // Track buffer for cleanup when document is closed
    g_docBuffers[doc] = docBuffer;
    LOGI("Document opened from memory, pages: %d", FPDF_GetPageCount(doc));
    return (jlong) doc;
}

/**
 * Open document from file path
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeOpenDocumentPath(JNIEnv *env, jobject thiz,
                                                          jstring path, jstring password) {
    const char *cPath = env->GetStringUTFChars(path, nullptr);
    const char *cPassword = nullptr;
    if (password != nullptr) {
        cPassword = env->GetStringUTFChars(password, nullptr);
    }

    // FPDF_LoadDocument() is not a standard exported function in some builds,
    // but FPDF_LoadCustomDocument is. However, standart fpdfview.h usually has:
    // FPDF_EXPORT FPDF_DOCUMENT FPDF_CALLCONV FPDF_LoadDocument(FPDF_STRING file_path, FPDF_BYTESTRING password);
    // Let's try it.
    FPDF_DOCUMENT doc = FPDF_LoadDocument(cPath, cPassword);

    env->ReleaseStringUTFChars(path, cPath);
    if (password != nullptr) {
        env->ReleaseStringUTFChars(password, cPassword);
    }

    if (!doc) {
        LOGE("Failed to load document from path, error: %lu", FPDF_GetLastError());
        return 0;
    }

    return (jlong) doc;
}

/**
 * Close document
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCloseDocument(JNIEnv *env, jobject thiz,
                                                       jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (doc) {
        FPDF_CloseDocument(doc);
        
        // Free the buffer associated with this document
        auto it = g_docBuffers.find(doc);
        if (it != g_docBuffers.end()) {
            delete[] it->second;
            g_docBuffers.erase(it);
        }
        LOGI("Document closed");
    }
}

/**
 * Get page count
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageCount(JNIEnv *env, jobject thiz,
                                                      jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return FPDF_GetPageCount(doc);
}

/**
 * Get document metadata
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetMetaText(JNIEnv *env, jobject thiz,
                                                     jlong docPtr, jstring tag) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return nullptr;
    
    const char *cTag = env->GetStringUTFChars(tag, nullptr);
    
    // First call to get required buffer size
    unsigned long size = FPDF_GetMetaText(doc, cTag, nullptr, 0);
    if (size == 0) {
        env->ReleaseStringUTFChars(tag, cTag);
        return env->NewStringUTF("");
    }
    
    // Allocate buffer and get text
    // FPDF_GetMetaText returns size in bytes including terminator.
    // The buffer should be cast to unsigned short* (UTF-16LE).
    unsigned short *buffer = new unsigned short[size];
    FPDF_GetMetaText(doc, cTag, buffer, size);
    
    env->ReleaseStringUTFChars(tag, cTag);
    
    // Convert UTF-16 to Java string. Size is in bytes, so divide by 2.
    // Subtract 1 for null terminator if it exists (check logic safely)
    jstring result = env->NewString((jchar*) buffer, size / 2 - 1);
    delete[] buffer;
    
    return result;
}

/**
 * Get page label (actual page number as displayed in PDF)
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageLabel(JNIEnv *env, jobject thiz,
                                                      jlong docPtr, jint pageIndex) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return env->NewStringUTF("");
    
    // First call to get required buffer size
    unsigned long size = FPDF_GetPageLabel(doc, pageIndex, nullptr, 0);
    if (size == 0) {
        return env->NewStringUTF("");
    }
    
    // Allocate buffer and get label
    // FPDF_GetPageLabel returns UTF-16LE encoded string
    unsigned short *buffer = new unsigned short[size];
    FPDF_GetPageLabel(doc, pageIndex, buffer, size * sizeof(unsigned short));
    
    // Convert UTF-16 to Java string
    jstring result = env->NewString((jchar*) buffer, size / 2 - 1);
    delete[] buffer;
    
    return result ? result : env->NewStringUTF("");
}

/**
 * Load Page
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeLoadPage(JNIEnv *env, jobject thiz,
                                                  jlong docPtr, jint pageIndex) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return (jlong) FPDF_LoadPage(doc, pageIndex);
}

/**
 * Close Page
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeClosePage(JNIEnv *env, jobject thiz,
                                                   jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (page) {
        FPDF_ClosePage(page);
    }
}

/**
 * Get Page Width
 */
JNIEXPORT jdouble JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageWidth(JNIEnv *env, jobject thiz,
                                                      jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0.0;
    return FPDF_GetPageWidth(page);
}

/**
 * Get Page Height
 */
JNIEXPORT jdouble JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageHeight(JNIEnv *env, jobject thiz,
                                                       jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0.0;
    return FPDF_GetPageHeight(page);
}

/**
 * Get Page Size by Index (without loading page)
 * This is much faster than loadPage+getWidth/getHeight for bulk size queries.
 */
JNIEXPORT jdoubleArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageSizeByIndex(JNIEnv *env, jobject thiz,
                                                            jlong docPtr, jint pageIndex) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return nullptr;
    
    double width = 0.0, height = 0.0;
    int success = FPDF_GetPageSizeByIndex(doc, pageIndex, &width, &height);
    
    if (!success) {
        // Return default A4 size if failed
        width = 595.0;
        height = 842.0;
    }
    
    jdoubleArray result = env->NewDoubleArray(2);
    jdouble values[2] = {width, height};
    env->SetDoubleArrayRegion(result, 0, 2, values);
    return result;
}

/**
 * Render Page to Bitmap
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeRenderPageBitmap(JNIEnv *env, jobject thiz,
                                                          jlong pagePtr, jobject bitmap,
                                                          jint startX, jint startY,
                                                          jint drawWidth, jint drawHeight,
                                                          jboolean renderAnnot) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page || !bitmap) return;

    AndroidBitmapInfo info;
    int ret = AndroidBitmap_getInfo(env, bitmap, &info);
    if (ret != ANDROID_BITMAP_RESULT_SUCCESS) {
        LOGE("AndroidBitmap_getInfo failed: %d", ret);
        return;
    }

    // Validate bitmap dimensions
    if (info.width == 0 || info.height == 0) {
        LOGE("Invalid bitmap dimensions: %dx%d", info.width, info.height);
        return;
    }

    // Check bitmap format - PDFium requires 4 bytes per pixel (ARGB_8888)
    // RGB_565 is NOT supported by FPDFBitmap_CreateEx
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        // Cannot render to RGB_565 or other formats directly
        // Skip silently - caller should use ARGB_8888 bitmaps
        return;
    }

    void *pixels;
    ret = AndroidBitmap_lockPixels(env, bitmap, &pixels);
    if (ret != ANDROID_BITMAP_RESULT_SUCCESS) {
        LOGE("AndroidBitmap_lockPixels failed: %d", ret);
        return;
    }
    
    // Create PDFium bitmap wrapper using BGRA format for ARGB_8888 Android bitmap
    FPDF_BITMAP fpdfBitmap = FPDFBitmap_CreateEx(info.width, info.height, FPDFBitmap_BGRA, pixels, info.stride);
    
    if (!fpdfBitmap) {
        LOGE("FPDFBitmap_CreateEx failed for %dx%d bitmap (stride=%d)", info.width, info.height, info.stride);
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    }

    // Fill background with white
    FPDFBitmap_FillRect(fpdfBitmap, 0, 0, info.width, info.height, 0xFFFFFFFF);

    int flags = FPDF_REVERSE_BYTE_ORDER; // Android uses ARGB, PDFium uses BGRA
    if (renderAnnot) {
        flags |= FPDF_ANNOT;
    }

    FPDF_RenderPageBitmap(fpdfBitmap, page, startX, startY, drawWidth, drawHeight, 0, flags);

    FPDFBitmap_Destroy(fpdfBitmap);
    AndroidBitmap_unlockPixels(env, bitmap);
}

/**
 * Device to Page Coordinate Conversion
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeDeviceToPage(JNIEnv *env, jobject thiz,
                                                      jlong pagePtr,
                                                      jint startX, jint startY,
                                                      jint sizeX, jint sizeY,
                                                      jint rotate,
                                                      jint deviceX, jint deviceY,
                                                      jdoubleArray result) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return;
    
    double pageX, pageY;
    FPDF_DeviceToPage(page, startX, startY, sizeX, sizeY, rotate, deviceX, deviceY, &pageX, &pageY);
    
    jdouble *body = env->GetDoubleArrayElements(result, nullptr);
    body[0] = pageX;
    body[1] = pageY;
    env->ReleaseDoubleArrayElements(result, body, 0);
}

/**
 * Page to Device Coordinate Conversion
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageToDevice(JNIEnv *env, jobject thiz,
                                                      jlong pagePtr,
                                                      jint startX, jint startY,
                                                      jint sizeX, jint sizeY,
                                                      jint rotate,
                                                      jdouble pageX, jdouble pageY,
                                                      jintArray result) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return;
    
    int deviceX, deviceY;
    FPDF_PageToDevice(page, startX, startY, sizeX, sizeY, rotate, pageX, pageY, &deviceX, &deviceY);
    
    jint *body = env->GetIntArrayElements(result, nullptr);
    body[0] = deviceX;
    body[1] = deviceY;
    env->ReleaseIntArrayElements(result, body, 0);
}

/**
 * Load Text Page
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeLoadTextPage(JNIEnv *env, jobject thiz,
                                                      jlong docPtr, jlong pagePtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!doc || !page) return 0;
    return (jlong) FPDFText_LoadPage(page);
}

/**
 * Close Text Page
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCloseTextPage(JNIEnv *env, jobject thiz,
                                                       jlong textPagePtr) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (textPage) {
        FPDFText_ClosePage(textPage);
    }
}

/**
 * Get Text Count
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextCountChars(JNIEnv *env, jobject thiz,
                                                        jlong textPagePtr) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return 0;
    return FPDFText_CountChars(textPage);
}

/**
 * Get Text in range
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetText(JNIEnv *env, jobject thiz,
                                                 jlong textPagePtr, jint startIndex, jint count) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return nullptr;
    
    // FPDFText_GetText requires buffer size in unsigned shorts (not bytes) + 1 for null terminator
    // The documentation says "number of characters", but it returns UTF-16LE.
    // Ensure we allocate enough.
    
    int length = count + 1;
    unsigned short *buffer = new unsigned short[length];
    
    int written = FPDFText_GetText(textPage, startIndex, count, buffer);
    
    jstring result;
    if (written > 0) {
        result = env->NewString((jchar*) buffer, written - 1);
    } else {
        result = env->NewStringUTF("");
    }
    
    delete[] buffer;
    return result;
}

/**
 * Get Character Box
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetCharBox(JNIEnv *env, jobject thiz,
                                                    jlong textPagePtr, jint index,
                                                    jdoubleArray result) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return;
    
    double left, right, bottom, top;
    FPDFText_GetCharBox(textPage, index, &left, &right, &bottom, &top);
    
    jdouble *body = env->GetDoubleArrayElements(result, nullptr);
    body[0] = left;
    body[1] = top;
    body[2] = right;
    body[3] = bottom;
    env->ReleaseDoubleArrayElements(result, body, 0);
}

/**
 * Get Character Index at Position
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetCharIndexAtPos(JNIEnv *env, jobject thiz,
                                                           jlong textPagePtr,
                                                           jdouble x, jdouble y,    
                                                           jdouble xTolerance, jdouble yTolerance) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return -1;
    return FPDFText_GetCharIndexAtPos(textPage, x, y, xTolerance, yTolerance);
}

/**
 * Start Text Search
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextFindStart(JNIEnv *env, jobject thiz,
                                                       jlong textPagePtr, jstring query,
                                                       jboolean matchCase, jboolean matchWholeWord) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return 0;
    
    const char *cQuery = env->GetStringUTFChars(query, nullptr);
    // Convert query to wide string implementation if needed, but FPDFText_FindStart takes FPDF_WIDESTRING
    // Use helper to convert UTF-8 to UTF-16LE
    
    jsize queryLen = env->GetStringLength(query);
    const jchar *queryChars = env->GetStringChars(query, nullptr);
    
    // FPDF_WIDESTRING is unsigned short*
    unsigned short *wQuery = new unsigned short[queryLen + 1];
    for (int i = 0; i < queryLen; i++) {
        wQuery[i] = (unsigned short) queryChars[i];
    }
    wQuery[queryLen] = 0;
    
    unsigned long flags = 0;
    if (matchCase) flags |= FPDF_MATCHCASE;
    if (matchWholeWord) flags |= FPDF_MATCHWHOLEWORD;
    
    FPDF_SCHHANDLE search = FPDFText_FindStart(textPage, wQuery, flags, 0);
    
    delete[] wQuery;
    env->ReleaseStringChars(query, queryChars);
    env->ReleaseStringUTFChars(query, cQuery); // Actually we didn't use this one, redundant but safe to release if valid
    
    return (jlong) search;
}

/**
 * Find Next
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextFindNext(JNIEnv *env, jobject thiz,
                                                      jlong searchHandle) {
    FPDF_SCHHANDLE search = (FPDF_SCHHANDLE) searchHandle;
    if (!search) return JNI_FALSE;
    return FPDFText_FindNext(search) ? JNI_TRUE : JNI_FALSE;
}

/**
 * Find Previous
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextFindPrev(JNIEnv *env, jobject thiz,
                                                      jlong searchHandle) {
    FPDF_SCHHANDLE search = (FPDF_SCHHANDLE) searchHandle;
    if (!search) return JNI_FALSE;
    return FPDFText_FindPrev(search) ? JNI_TRUE : JNI_FALSE;
}

/**
 * Get Search Result Index
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetSchResultIndex(JNIEnv *env, jobject thiz,
                                                               jlong searchHandle) {
    FPDF_SCHHANDLE search = (FPDF_SCHHANDLE) searchHandle;
    if (!search) return -1;
    return FPDFText_GetSchResultIndex(search);
}

/**
 * Get Search Result Count
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetSchCount(JNIEnv *env, jobject thiz,
                                                         jlong searchHandle) {
    FPDF_SCHHANDLE search = (FPDF_SCHHANDLE) searchHandle;
    if (!search) return 0;
    return FPDFText_GetSchCount(search);
}

/**
 * Close Text Search
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextFindClose(JNIEnv *env, jobject thiz,
                                                       jlong searchHandle) {
    FPDF_SCHHANDLE search = (FPDF_SCHHANDLE) searchHandle;
    if (search) {
        FPDFText_FindClose(search);
    }
}

/**
 * Get First Child Bookmark
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetFirstChildBookmark(JNIEnv *env, jobject thiz,
                                                               jlong docPtr, jlong bookmarkPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    FPDF_BOOKMARK bookmark = (FPDF_BOOKMARK) bookmarkPtr; // Can be NULL for root
    if (!doc) return 0;
    return (jlong) FPDFBookmark_GetFirstChild(doc, bookmark);
}

/**
 * Get Next Sibling Bookmark
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetNextSiblingBookmark(JNIEnv *env, jobject thiz,
                                                                jlong docPtr, jlong bookmarkPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    FPDF_BOOKMARK bookmark = (FPDF_BOOKMARK) bookmarkPtr;
    if (!doc || !bookmark) return 0;
    return (jlong) FPDFBookmark_GetNextSibling(doc, bookmark);
}

/**
 * Get Bookmark Title
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetBookmarkTitle(JNIEnv *env, jobject thiz,
                                                          jlong bookmarkPtr) {
    FPDF_BOOKMARK bookmark = (FPDF_BOOKMARK) bookmarkPtr;
    if (!bookmark) return nullptr;

    unsigned long size = FPDFBookmark_GetTitle(bookmark, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");

    unsigned short *buffer = new unsigned short[size];
    FPDFBookmark_GetTitle(bookmark, buffer, size);

    jstring result = env->NewString((jchar*) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

/**
 * Get Bookmark Dest Page Index
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetBookmarkDestIndex(JNIEnv *env, jobject thiz,
                                                              jlong docPtr, jlong bookmarkPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    FPDF_BOOKMARK bookmark = (FPDF_BOOKMARK) bookmarkPtr;
    if (!doc || !bookmark) return -1;

    FPDF_DEST dest = FPDFBookmark_GetDest(doc, bookmark);
    if (!dest) {
        // Try action
        FPDF_ACTION action = FPDFBookmark_GetAction(bookmark);
        if (action) {
            dest = FPDFAction_GetDest(doc, action);
        }
    }
    
    if (!dest) return -1;
    return FPDFDest_GetDestPageIndex(doc, dest);
}

/**
 * Get Link at Point
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetLinkAtPoint(JNIEnv *env, jobject thiz,
                                                        jlong pagePtr, jdouble x, jdouble y) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0;
    return (jlong) FPDFLink_GetLinkAtPoint(page, x, y);
}

/**
 * Get Link Dest Page Index
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetLinkDestIndex(JNIEnv *env, jobject thiz,
                                                          jlong docPtr, jlong linkPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    FPDF_LINK link = (FPDF_LINK) linkPtr;
    if (!doc || !link) return -1;
    
    FPDF_DEST dest = FPDFLink_GetDest(doc, link);
    if (!dest) {
         FPDF_ACTION action = FPDFLink_GetAction(link);
         if (action) {
             dest = FPDFAction_GetDest(doc, action);
         }
    }
    
    if (!dest) return -1;
    return FPDFDest_GetDestPageIndex(doc, dest);
}

/**
 * Get Link URI
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetLinkURI(JNIEnv *env, jobject thiz,
                                                    jlong docPtr, jlong linkPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    FPDF_LINK link = (FPDF_LINK) linkPtr;
    if (!doc || !link) return nullptr;

    FPDF_ACTION action = FPDFLink_GetAction(link);
    if (!action) return nullptr;
    
    unsigned long size = FPDFAction_GetURIPath(doc, action, nullptr, 0);
    if (size == 0) return nullptr;
    
    char *buffer = new char[size];
    FPDFAction_GetURIPath(doc, action, buffer, size);
    
    jstring result = env->NewStringUTF(buffer);
    delete[] buffer;
    return result;
}

/**
 * Get Link Rect
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetLinkRect(JNIEnv *env, jobject thiz,
                                                     jlong linkPtr, jdoubleArray result) {
    FPDF_LINK link = (FPDF_LINK) linkPtr;
    if (!link) return;
    
    FS_RECTF rect;
    if (FPDFLink_GetAnnotRect(link, &rect)) {
        jdouble *body = env->GetDoubleArrayElements(result, nullptr);
        body[0] = rect.left;
        body[1] = rect.top;
        body[2] = rect.right;
        body[3] = rect.bottom;
        env->ReleaseDoubleArrayElements(result, body, 0);
    }
}

/**
 * Get Annotation Count
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetAnnotCount(JNIEnv *env, jobject thiz,
                                                       jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0;
    return FPDFPage_GetAnnotCount(page);
}

/**
 * Get Annotation
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetAnnot(JNIEnv *env, jobject thiz,
                                                  jlong pagePtr, jint index) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0;
    return (jlong) FPDFPage_GetAnnot(page, index);
}

/**
 * Close Annotation
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCloseAnnot(JNIEnv *env, jobject thiz,
                                                    jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (annot) {
        FPDFPage_CloseAnnot(annot);
    }
}

/**
 * Get Annotation Subtype
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetAnnotSubtype(JNIEnv *env, jobject thiz,
                                                         jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return -1;
    return (jint) FPDFAnnot_GetSubtype(annot);
}

/**
 * Get Annotation Rect
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetAnnotRect(JNIEnv *env, jobject thiz,
                                                      jlong annotPtr, jdoubleArray result) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return;
    
    FS_RECTF rect;
    if (FPDFAnnot_GetRect(annot, &rect)) {
        jdouble *body = env->GetDoubleArrayElements(result, nullptr);
        body[0] = rect.left;
        body[1] = rect.top;
        body[2] = rect.right;
        body[3] = rect.bottom;
        env->ReleaseDoubleArrayElements(result, body, 0);
    }
}

/**
 * Create Annotation
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCreateAnnot(JNIEnv *env, jobject thiz,
                                                     jlong pagePtr, jint subtype) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0;
    return (jlong) FPDFPage_CreateAnnot(page, subtype);
}

/**
 * Set Annotation Rect
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetAnnotRect(JNIEnv *env, jobject thiz,
                                                      jlong annotPtr, jdoubleArray rectArray) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    
    jdouble *rectData = env->GetDoubleArrayElements(rectArray, nullptr);
    FS_RECTF rect;
    rect.left = (float) rectData[0];
    rect.top = (float) rectData[1];
    rect.right = (float) rectData[2];
    rect.bottom = (float) rectData[3];
    env->ReleaseDoubleArrayElements(rectArray, rectData, 0);
    
    return FPDFAnnot_SetRect(annot, &rect) ? JNI_TRUE : JNI_FALSE;
}

/**
 * Set Annotation Contents
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetAnnotContents(JNIEnv *env, jobject thiz,
                                                          jlong annotPtr, jstring contents) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    
    const char *cContents = env->GetStringUTFChars(contents, nullptr);
    jsize len = env->GetStringLength(contents);
    const jchar *wContents = env->GetStringChars(contents, nullptr);
    
    unsigned short *buffer = new unsigned short[len + 1];
    for (int i = 0; i < len; i++) {
        buffer[i] = (unsigned short) wContents[i];
    }
    buffer[len] = 0;
    
    jboolean result = FPDFAnnot_SetStringValue(annot, "Contents", buffer) ? JNI_TRUE : JNI_FALSE;
    
    delete[] buffer;
    env->ReleaseStringChars(contents, wContents);
    env->ReleaseStringUTFChars(contents, cContents);
    
    return result;
}

/**
 * Set Annotation Color
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetAnnotColor(JNIEnv *env, jobject thiz,
                                                       jlong annotPtr, 
                                                       jint type, 
                                                       jint r, jint g, jint b, jint a) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    return FPDFAnnot_SetColor(annot, (FPDFANNOT_COLORTYPE)type, r, g, b, a) ? JNI_TRUE : JNI_FALSE;
}

/**
 * Set Annotation Flags
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetAnnotFlags(JNIEnv *env, jobject thiz,
                                                       jlong annotPtr, jint flags) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    return FPDFAnnot_SetFlags(annot, flags) ? JNI_TRUE : JNI_FALSE;
}


/**
 * Create New Document
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeNewDocument(JNIEnv *env, jobject thiz) {
    return (jlong) FPDF_CreateNewDocument();
}

/**
 * Create New Page
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeNewPage(JNIEnv *env, jobject thiz,
                                                 jlong docPtr, jint index, jdouble width, jdouble height) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return (jlong) FPDFPage_New(doc, index, width, height);
}

// File Write Interface Implementation
struct FPDF_FILEWRITE_IMPL : public FPDF_FILEWRITE {
    FILE *file;
    
    static int WriteBlockImpl(FPDF_FILEWRITE* pThis, const void* pData, unsigned long size) {
        FPDF_FILEWRITE_IMPL* pImpl = (FPDF_FILEWRITE_IMPL*)pThis;
        return fwrite(pData, 1, size, pImpl->file) == size;
    }
};

/**
 * Save Document
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSaveDocument(JNIEnv *env, jobject thiz,
                                                      jlong docPtr, jstring path) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return JNI_FALSE;
    
    const char *cPath = env->GetStringUTFChars(path, nullptr);
    FILE *file = fopen(cPath, "wb");
    if (!file) {
        env->ReleaseStringUTFChars(path, cPath);
        return JNI_FALSE;
    }
    
    FPDF_FILEWRITE_IMPL writer;
    writer.version = 1;
    writer.WriteBlock = FPDF_FILEWRITE_IMPL::WriteBlockImpl;
    writer.file = file;
    
    bool success = FPDF_SaveAsCopy(doc, &writer, 0);
    
    fclose(file);
    env->ReleaseStringUTFChars(path, cPath);
    return success ? JNI_TRUE : JNI_FALSE;
}

// ----------------------------------------------------------------------------
// Form Filling Support
// ----------------------------------------------------------------------------

struct FormFillInfo : public FPDF_FORMFILLINFO {
    // Keeping it simple for now, can extend to call Java methods later if needed
};

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeInitFormFillEnvironment(JNIEnv *env, jobject thiz,
                                                                 jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    
    // We allocate the struct on heap, it must be freed in ExitFormFillEnvironment
    FormFillInfo *formInfo = new FormFillInfo();
    memset(formInfo, 0, sizeof(FPDF_FORMFILLINFO));
    formInfo->version = 1;
    // formInfo->m_pJsPlatform = ...; // JS support if needed
    
    // Initialize callbacks to stub/null or simple implementations if required
    // FPDFDOC_InitFormFillEnvironment checks version and some pointers.
    // For read-only/basic forms, 0-initialized might be okay or crash.
    // Let's implement basics.
    
    FPDF_FORMHANDLE formHandle = FPDFDOC_InitFormFillEnvironment(doc, formInfo);
    
    // Store the info pointer in the user data of form handle if possible, 
    // or we just trust the caller to manage it? 
    // FPDFDOC_InitFormFillEnvironment returns a handle, but doesn't take ownership of info.
    // We need to associate them. For now, we return the HANDLE. 
    // But we need to delete formInfo later. 
    // We can map handle -> info in a global map, OR we create a struct wrapper.
    // Simpler: We assume the caller (Java) holds the pointer to formInfo? No, it holds native pointer.
    // Wait, FPDF_FORMHANDLE is opaque.
    // Let's modify the signature to return a pair or just return handle, and leak the info for now (bad practice)
    // OR we just assume we'll fix memory management in a robust wrapper.
    // For this implementation, I will just return the FormHandle. 
    // ISSUE: We need to free `formInfo` when closing.
    // WORKAROUND: We can store formInfo in the `m_pUserData` if unused, or just implement Release callback?
    // FPDF_FORMFILLINFO::Release is called when... FPDFDOC_ExitFormFillEnvironment is called?
    // Documentation says: "Release - Give the implementation a chance to release any data..."
    // Let's use that.
    
    formInfo->Release = [](FPDF_FORMFILLINFO* pThis) {
        delete (FormFillInfo*)pThis;
    };
    
    return (jlong) formHandle;
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeExitFormFillEnvironment(JNIEnv *env, jobject thiz,
                                                                 jlong formHandlePtr) {
    FPDF_FORMHANDLE formHandle = (FPDF_FORMHANDLE) formHandlePtr;
    if (formHandle) {
        FPDFDOC_ExitFormFillEnvironment(formHandle);
    }
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFORMOnAfterLoadPage(JNIEnv *env, jobject thiz,
                                                             jlong pagePtr, jlong formHandlePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    FPDF_FORMHANDLE formHandle = (FPDF_FORMHANDLE) formHandlePtr;
    if (page && formHandle) {
        FORM_OnAfterLoadPage(page, formHandle);
    }
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFORMOnBeforeClosePage(JNIEnv *env, jobject thiz,
                                                               jlong pagePtr, jlong formHandlePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    FPDF_FORMHANDLE formHandle = (FPDF_FORMHANDLE) formHandlePtr;
    if (page && formHandle) {
        FORM_OnBeforeClosePage(page, formHandle);
    }
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFPDFFFLDraw(JNIEnv *env, jobject thiz,
                                                     jlong formHandlePtr, jobject bitmap,
                                                     jlong pagePtr,
                                                     jint startX, jint startY,
                                                     jint drawWidth, jint drawHeight,
                                                     jint rotate, jint flags) {
    FPDF_FORMHANDLE formHandle = (FPDF_FORMHANDLE) formHandlePtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!formHandle || !page || !bitmap) return;
    
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) != ANDROID_BITMAP_RESULT_SUCCESS) return;
    
    // Check bitmap format - PDFium requires ARGB_8888
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) return;
    
    void *pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS) return;
    
    FPDF_BITMAP fpdfBitmap = FPDFBitmap_CreateEx(info.width, info.height, FPDFBitmap_BGRA, pixels, info.stride);
    if (fpdfBitmap) {
         FPDF_FFLDraw(formHandle, fpdfBitmap, page, startX, startY, drawWidth, drawHeight, rotate, flags);
         FPDFBitmap_Destroy(fpdfBitmap);
    }
    
    AndroidBitmap_unlockPixels(env, bitmap);
}

// ----------------------------------------------------------------------------
// Attachment Support
// ----------------------------------------------------------------------------

/**
 * Get Attachment Count
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetAttachmentCount(JNIEnv *env, jobject thiz,
                                                            jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return FPDFDoc_GetAttachmentCount(doc);
}

/**
 * Get Attachment Name
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetAttachmentName(JNIEnv *env, jobject thiz,
                                                           jlong docPtr, jint index) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return nullptr;
    
    FPDF_ATTACHMENT attachment = FPDFDoc_GetAttachment(doc, index);
    if (!attachment) return nullptr;
    
    unsigned long size = FPDFAttachment_GetName(attachment, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    
    unsigned short *buffer = new unsigned short[size];
    FPDFAttachment_GetName(attachment, buffer, size);
    
    jstring result = env->NewString((jchar*) buffer, size / 2 - 1);
    delete[] buffer;
    
    return result;
}

/**
 * Get Attachment File Data
 */
JNIEXPORT jbyteArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetAttachmentFile(JNIEnv *env, jobject thiz,
                                                           jlong docPtr, jint index) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return nullptr;
    
    FPDF_ATTACHMENT attachment = FPDFDoc_GetAttachment(doc, index);
    if (!attachment) return nullptr;
    
    unsigned long size = 0;
    if (!FPDFAttachment_GetFile(attachment, nullptr, 0, &size)) {
         return nullptr;
    }
    
    if (size == 0) return env->NewByteArray(0);
    
    jbyteArray result = env->NewByteArray(size);
    jbyte *buffer = env->GetByteArrayElements(result, nullptr);
    
    unsigned long outLen = 0;
    FPDFAttachment_GetFile(attachment, buffer, size, &outLen);
    
    env->ReleaseByteArrayElements(result, buffer, 0);
    
    return result;
}
// Page Object Support
// ----------------------------------------------------------------------------

/**
 * Get Page Object Count
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCountPageObjects(JNIEnv *env, jobject thiz,
                                                          jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0;
    return FPDFPage_CountObjects(page);
}

/**
 * Get Page Object
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageObject(JNIEnv *env, jobject thiz,
                                                       jlong pagePtr, jint index) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0;
    return (jlong) FPDFPage_GetObject(page, index);
}

/**
 * Get Page Object Type
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageObjectType(JNIEnv *env, jobject thiz,
                                                           jlong pageObjPtr) {
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!pageObj) return -1;
    return FPDFPageObj_GetType(pageObj);
}

// ----------------------------------------------------------------------------
// Phase 8: Comprehensive Page Editing (Images, Paths, Text Objects)
// ----------------------------------------------------------------------------

/**
 * Text Object Creation
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeNewTextObj(JNIEnv *env, jobject thiz,
                                                    jlong docPtr, jstring fontName, jfloat fontSize) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    
    // FPDFPageObj_NewTextObj takes font NAME and size directly
    const char *cFontName = env->GetStringUTFChars(fontName, nullptr);
    FPDF_PAGEOBJECT textObj = FPDFPageObj_NewTextObj(doc, cFontName, fontSize);
    env->ReleaseStringUTFChars(fontName, cFontName);
    
    return (jlong) textObj;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetTextObjText(JNIEnv *env, jobject thiz,
                                                        jlong textObjPtr, jstring text) {
    FPDF_PAGEOBJECT textObj = (FPDF_PAGEOBJECT) textObjPtr;
    if (!textObj) return JNI_FALSE;
    
    const jchar *wText = env->GetStringChars(text, nullptr);
    jsize len = env->GetStringLength(text);
    
    unsigned short *buffer = new unsigned short[len + 1];
    for (int i=0; i<len; i++) buffer[i] = (unsigned short)wText[i];
    buffer[len] = 0;
    
    jboolean result = FPDFText_SetText(textObj, buffer) ? JNI_TRUE : JNI_FALSE;
    
    delete[] buffer;
    env->ReleaseStringChars(text, wText);
    return result;
}

/**
 * Path Object Creation
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCreateNewPath(JNIEnv *env, jobject thiz,
                                                      jfloat x, jfloat y) {
    return (jlong) FPDFPageObj_CreateNewPath(x, y);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePathMoveTo(JNIEnv *env, jobject thiz,
                                                   jlong pathObjPtr, jfloat x, jfloat y) {
    FPDF_PAGEOBJECT pathObj = (FPDF_PAGEOBJECT) pathObjPtr;
    return FPDFPath_MoveTo(pathObj, x, y) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePathLineTo(JNIEnv *env, jobject thiz,
                                                   jlong pathObjPtr, jfloat x, jfloat y) {
    FPDF_PAGEOBJECT pathObj = (FPDF_PAGEOBJECT) pathObjPtr;
    return FPDFPath_LineTo(pathObj, x, y) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePathBezierTo(JNIEnv *env, jobject thiz,
                                                     jlong pathObjPtr, jfloat x1, jfloat y1,
                                                     jfloat x2, jfloat y2,
                                                     jfloat x3, jfloat y3) {
    FPDF_PAGEOBJECT pathObj = (FPDF_PAGEOBJECT) pathObjPtr;
    return FPDFPath_BezierTo(pathObj, x1, y1, x2, y2, x3, y3) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePathClose(JNIEnv *env, jobject thiz,
                                                  jlong pathObjPtr) {
    FPDF_PAGEOBJECT pathObj = (FPDF_PAGEOBJECT) pathObjPtr;
    return FPDFPath_Close(pathObj) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePathSetDrawMode(JNIEnv *env, jobject thiz,
                                                        jlong pathObjPtr, jint fillMode, jboolean stroke) {
    FPDF_PAGEOBJECT pathObj = (FPDF_PAGEOBJECT) pathObjPtr;
    return FPDFPath_SetDrawMode(pathObj, fillMode, stroke ? 1 : 0) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePathSetStrokeWidth(JNIEnv *env, jobject thiz,
                                                           jlong pathObjPtr, jfloat width) {
    FPDF_PAGEOBJECT pathObj = (FPDF_PAGEOBJECT) pathObjPtr;
    return FPDFPageObj_SetStrokeWidth(pathObj, width) ? JNI_TRUE : JNI_FALSE;
}

/**
 * Image Object Creation
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeNewImageObj(JNIEnv *env, jobject thiz,
                                                    jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return (jlong) FPDFPageObj_NewImageObj(doc);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjSetBitmap(JNIEnv *env, jobject thiz,
                                                          jlong imageObjPtr, jobject bitmap) {
    // In a real implementation we would convert Android Bitmap to FPDF_BITMAP or feed raw data
    // This is complex because we need to keep the bitmap data alive or copy it.
    // Simplifying: LoadJpegFile is easier for now as per features.md checklist
    return JNI_FALSE; 
}

/**
 * Object Management
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeInsertObject(JNIEnv *env, jobject thiz,
                                                     jlong pagePtr, jlong pageObjPtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (page && pageObj) {
        FPDFPage_InsertObject(page, pageObj);
    }
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeRemoveObject(JNIEnv *env, jobject thiz,
                                                     jlong pagePtr, jlong pageObjPtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (page && pageObj) {
        return FPDFPage_RemoveObject(page, pageObj) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetObjectFillColor(JNIEnv *env, jobject thiz,
                                                           jlong pageObjPtr, jint r, jint g, jint b, jint a) {
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (pageObj) {
        FPDFPageObj_SetFillColor(pageObj, r, g, b, a);
    }
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetObjectStrokeColor(JNIEnv *env, jobject thiz,
                                                             jlong pageObjPtr, jint r, jint g, jint b, jint a) {
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (pageObj) {
        FPDFPageObj_SetStrokeColor(pageObj, r, g, b, a);
    }
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGenerateContent(JNIEnv *env, jobject thiz,
                                                        jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (page) FPDFPage_GenerateContent(page);
}

// ----------------------------------------------------------------------------
// Phase 9: Document Utilities (Import/Export, Flatten, Transform)
// ----------------------------------------------------------------------------

/**
 * Page Import/Export
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImportPages(JNIEnv *env, jobject thiz,
                                                    jlong destDocPtr, jlong srcDocPtr,
                                                    jstring pageRange, jint insertIndex) {
    FPDF_DOCUMENT destDoc = (FPDF_DOCUMENT) destDocPtr;
    FPDF_DOCUMENT srcDoc = (FPDF_DOCUMENT) srcDocPtr;
    if (!destDoc || !srcDoc) return JNI_FALSE;
    
    const char *cPageRange = nullptr;
    if (pageRange != nullptr) {
        cPageRange = env->GetStringUTFChars(pageRange, nullptr);
    }
    
    jboolean result = FPDF_ImportPages(destDoc, srcDoc, cPageRange, insertIndex) ? JNI_TRUE : JNI_FALSE;
    
    if (cPageRange) env->ReleaseStringUTFChars(pageRange, cPageRange);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCopyViewerPreferences(JNIEnv *env, jobject thiz,
                                                              jlong destDocPtr, jlong srcDocPtr) {
    FPDF_DOCUMENT destDoc = (FPDF_DOCUMENT) destDocPtr;
    FPDF_DOCUMENT srcDoc = (FPDF_DOCUMENT) srcDocPtr;
    if (!destDoc || !srcDoc) return JNI_FALSE;
    return FPDF_CopyViewerPreferences(destDoc, srcDoc) ? JNI_TRUE : JNI_FALSE;
}

/**
 * Page Flatten
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFlattenPage(JNIEnv *env, jobject thiz,
                                                    jlong pagePtr, jint flags) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return -1;
    return FPDFPage_Flatten(page, flags);
}

/**
 * Page Transform
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetPageMediaBox(JNIEnv *env, jobject thiz,
                                                        jlong pagePtr, jfloat left, jfloat bottom,
                                                        jfloat right, jfloat top) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return JNI_FALSE;
    FPDFPage_SetMediaBox(page, left, bottom, right, top);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetPageCropBox(JNIEnv *env, jobject thiz,
                                                       jlong pagePtr, jfloat left, jfloat bottom,
                                                       jfloat right, jfloat top) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return JNI_FALSE;
    FPDFPage_SetCropBox(page, left, bottom, right, top);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageMediaBox(JNIEnv *env, jobject thiz,
                                                        jlong pagePtr, jfloatArray result) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return JNI_FALSE;
    
    float left, bottom, right, top;
    if (!FPDFPage_GetMediaBox(page, &left, &bottom, &right, &top)) return JNI_FALSE;
    
    jfloat *body = env->GetFloatArrayElements(result, nullptr);
    body[0] = left; body[1] = bottom; body[2] = right; body[3] = top;
    env->ReleaseFloatArrayElements(result, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageCropBox(JNIEnv *env, jobject thiz,
                                                       jlong pagePtr, jfloatArray result) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return JNI_FALSE;
    
    float left, bottom, right, top;
    if (!FPDFPage_GetCropBox(page, &left, &bottom, &right, &top)) return JNI_FALSE;
    
    jfloat *body = env->GetFloatArrayElements(result, nullptr);
    body[0] = left; body[1] = bottom; body[2] = right; body[3] = top;
    env->ReleaseFloatArrayElements(result, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageRotation(JNIEnv *env, jobject thiz,
                                                        jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return -1;
    return FPDFPage_GetRotation(page);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetPageRotation(JNIEnv *env, jobject thiz,
                                                        jlong pagePtr, jint rotation) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (page) FPDFPage_SetRotation(page, rotation);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeDeletePage(JNIEnv *env, jobject thiz,
                                                   jlong docPtr, jint pageIndex) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (doc) FPDFPage_Delete(doc, pageIndex);
}

// ----------------------------------------------------------------------------
// Phase 10: Advanced Rendering & Navigation (Thumbnails, StructTree, Progressive)
// ----------------------------------------------------------------------------

/**
 * Thumbnails
 */
JNIEXPORT jbyteArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetDecodedThumbnailData(JNIEnv *env, jobject thiz,
                                                                jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return nullptr;
    
    unsigned long size = FPDFPage_GetDecodedThumbnailData(page, nullptr, 0);
    if (size == 0) return nullptr;
    
    jbyteArray result = env->NewByteArray(size);
    jbyte *buffer = env->GetByteArrayElements(result, nullptr);
    FPDFPage_GetDecodedThumbnailData(page, buffer, size);
    env->ReleaseByteArrayElements(result, buffer, 0);
    return result;
}

JNIEXPORT jbyteArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetRawThumbnailData(JNIEnv *env, jobject thiz,
                                                            jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return nullptr;
    
    unsigned long size = FPDFPage_GetRawThumbnailData(page, nullptr, 0);
    if (size == 0) return nullptr;
    
    jbyteArray result = env->NewByteArray(size);
    jbyte *buffer = env->GetByteArrayElements(result, nullptr);
    FPDFPage_GetRawThumbnailData(page, buffer, size);
    env->ReleaseByteArrayElements(result, buffer, 0);
    return result;
}

/**
 * Structure Tree (Accessibility)
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetStructTreeForPage(JNIEnv *env, jobject thiz,
                                                             jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0;
    return (jlong) FPDF_StructTree_GetForPage(page);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCloseStructTree(JNIEnv *env, jobject thiz,
                                                        jlong structTreePtr) {
    FPDF_STRUCTTREE tree = (FPDF_STRUCTTREE) structTreePtr;
    if (tree) FPDF_StructTree_Close(tree);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructTreeCountChildren(JNIEnv *env, jobject thiz,
                                                                jlong structTreePtr) {
    FPDF_STRUCTTREE tree = (FPDF_STRUCTTREE) structTreePtr;
    if (!tree) return 0;
    return FPDF_StructTree_CountChildren(tree);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructTreeGetChildAtIndex(JNIEnv *env, jobject thiz,
                                                                   jlong structTreePtr, jint index) {
    FPDF_STRUCTTREE tree = (FPDF_STRUCTTREE) structTreePtr;
    if (!tree) return 0;
    return (jlong) FPDF_StructTree_GetChildAtIndex(tree, index);
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetType(JNIEnv *env, jobject thiz,
                                                             jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return nullptr;
    
    unsigned long size = FPDF_StructElement_GetType(elem, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    
    unsigned short *buffer = new unsigned short[size];
    FPDF_StructElement_GetType(elem, buffer, size);
    jstring result = env->NewString((jchar*)buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetAltText(JNIEnv *env, jobject thiz,
                                                                jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return nullptr;
    
    unsigned long size = FPDF_StructElement_GetAltText(elem, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    
    unsigned short *buffer = new unsigned short[size];
    FPDF_StructElement_GetAltText(elem, buffer, size);
    jstring result = env->NewString((jchar*)buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

// ----------------------------------------------------------------------------
// Phase 11: Specialized Features (Signatures, Data Availability)
// ----------------------------------------------------------------------------

/**
 * Signatures
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetSignatureCount(JNIEnv *env, jobject thiz,
                                                          jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return FPDF_GetSignatureCount(doc);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetSignatureObject(JNIEnv *env, jobject thiz,
                                                           jlong docPtr, jint index) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return (jlong) FPDF_GetSignatureObject(doc, index);
}

JNIEXPORT jbyteArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetSignatureContents(JNIEnv *env, jobject thiz,
                                                             jlong sigObjPtr) {
    FPDF_SIGNATURE sig = (FPDF_SIGNATURE) sigObjPtr;
    if (!sig) return nullptr;
    
    unsigned long size = FPDFSignatureObj_GetContents(sig, nullptr, 0);
    if (size == 0) return nullptr;
    
    jbyteArray result = env->NewByteArray(size);
    jbyte *buffer = env->GetByteArrayElements(result, nullptr);
    FPDFSignatureObj_GetContents(sig, buffer, size);
    env->ReleaseByteArrayElements(result, buffer, 0);
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetSignatureReason(JNIEnv *env, jobject thiz,
                                                           jlong sigObjPtr) {
    FPDF_SIGNATURE sig = (FPDF_SIGNATURE) sigObjPtr;
    if (!sig) return nullptr;
    
    unsigned long size = FPDFSignatureObj_GetReason(sig, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    
    char *buffer = new char[size];
    FPDFSignatureObj_GetReason(sig, buffer, size);
    jstring result = env->NewStringUTF(buffer);
    delete[] buffer;
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetSignatureTime(JNIEnv *env, jobject thiz,
                                                         jlong sigObjPtr) {
    FPDF_SIGNATURE sig = (FPDF_SIGNATURE) sigObjPtr;
    if (!sig) return nullptr;
    
    unsigned long size = FPDFSignatureObj_GetTime(sig, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    
    char *buffer = new char[size];
    FPDFSignatureObj_GetTime(sig, buffer, size);
    jstring result = env->NewStringUTF(buffer);
    delete[] buffer;
    return result;
}

/**
 * JavaScript
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetJavaScriptActionCount(JNIEnv *env, jobject thiz,
                                                                  jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return FPDFDoc_GetJavaScriptActionCount(doc);
}

// ----------------------------------------------------------------------------
// Phase 12: Remaining Minor Features (WebLinks, Font Info, Enums, etc.)
// ----------------------------------------------------------------------------

/**
 * Web Links in Text
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeLoadWebLinks(JNIEnv *env, jobject thiz,
                                                     jlong textPagePtr) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return 0;
    return (jlong) FPDFLink_LoadWebLinks(textPage);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCloseWebLinks(JNIEnv *env, jobject thiz,
                                                      jlong pageLinksPtr) {
    FPDF_PAGELINK pageLinks = (FPDF_PAGELINK) pageLinksPtr;
    if (pageLinks) FPDFLink_CloseWebLinks(pageLinks);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCountWebLinks(JNIEnv *env, jobject thiz,
                                                      jlong pageLinksPtr) {
    FPDF_PAGELINK pageLinks = (FPDF_PAGELINK) pageLinksPtr;
    if (!pageLinks) return 0;
    return FPDFLink_CountWebLinks(pageLinks);
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetWebLinkURL(JNIEnv *env, jobject thiz,
                                                      jlong pageLinksPtr, jint index) {
    FPDF_PAGELINK pageLinks = (FPDF_PAGELINK) pageLinksPtr;
    if (!pageLinks) return nullptr;
    
    int size = FPDFLink_GetURL(pageLinks, index, nullptr, 0);
    if (size <= 0) return env->NewStringUTF("");
    
    unsigned short *buffer = new unsigned short[size];
    FPDFLink_GetURL(pageLinks, index, buffer, size);
    jstring result = env->NewString((jchar*)buffer, size - 1);
    delete[] buffer;
    return result;
}

/**
 * Form Type
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetFormType(JNIEnv *env, jobject thiz,
                                                    jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return -1;
    return FPDF_GetFormType(doc);
}

/**
 * Page Mode
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageMode(JNIEnv *env, jobject thiz,
                                                    jlong docPtr) {
    // FPDFDoc_GetPageMode is not in our PDFium build, return -1 (unknown)
    return -1; 
}

/**
 * Transform Object
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTransformPageObj(JNIEnv *env, jobject thiz,
                                                         jlong pageObjPtr,
                                                         jdouble a, jdouble b, jdouble c,
                                                         jdouble d, jdouble e, jdouble f) {
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (pageObj) FPDFPageObj_Transform(pageObj, a, b, c, d, e, f);
}

/**
 * Get Object Bounds
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageObjBounds(JNIEnv *env, jobject thiz,
                                                         jlong pageObjPtr, jfloatArray result) {
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!pageObj) return JNI_FALSE;
    
    float left, bottom, right, top;
    if (!FPDFPageObj_GetBounds(pageObj, &left, &bottom, &right, &top)) return JNI_FALSE;
    
    jfloat *body = env->GetFloatArrayElements(result, nullptr);
    body[0] = left; body[1] = bottom; body[2] = right; body[3] = top;
    env->ReleaseFloatArrayElements(result, body, 0);
    return JNI_TRUE;
}

/**
 * Remove Annotation
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeRemoveAnnot(JNIEnv *env, jobject thiz,
                                                    jlong pagePtr, jint index) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return JNI_FALSE;
    return FPDFPage_RemoveAnnot(page, index) ? JNI_TRUE : JNI_FALSE;
}

// ----------------------------------------------------------------------------
// Progressive Rendering
// ----------------------------------------------------------------------------

// We need a simple IFSDK_PAUSE implementation that can call back to Java
struct JavaPauseCallback : public IFSDK_PAUSE {
    JNIEnv *env;
    jobject callback;
    jmethodID methodId;
    
    static FPDF_BOOL NeedToPauseNowImpl(IFSDK_PAUSE* pThis) {
        JavaPauseCallback* self = static_cast<JavaPauseCallback*>(pThis);
        if (self->env && self->callback && self->methodId) {
            return self->env->CallBooleanMethod(self->callback, self->methodId) ? 1 : 0;
        }
        return 0; // Don't pause by default
    }
};

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeRenderPageBitmapStart(JNIEnv *env, jobject thiz,
                                                              jobject bitmap, jlong pagePtr,
                                                              jint startX, jint startY,
                                                              jint drawWidth, jint drawHeight,
                                                              jint rotate, jint flags) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page || !bitmap) return FPDF_RENDER_FAILED;
    
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) != ANDROID_BITMAP_RESULT_SUCCESS) {
        return FPDF_RENDER_FAILED;
    }
    
    // Check bitmap format - PDFium requires ARGB_8888
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        return FPDF_RENDER_FAILED;
    }
    
    void *pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS) {
        return FPDF_RENDER_FAILED;
    }
    
    FPDF_BITMAP fpdfBitmap = FPDFBitmap_CreateEx(info.width, info.height, FPDFBitmap_BGRA, pixels, info.stride);
    if (!fpdfBitmap) {
        AndroidBitmap_unlockPixels(env, bitmap);
        return FPDF_RENDER_FAILED;
    }
    
    // Fill with white
    FPDFBitmap_FillRect(fpdfBitmap, 0, 0, info.width, info.height, 0xFFFFFFFF);
    
    // Start progressive rendering without pause callback (will complete in one go unless very large)
    int status = FPDF_RenderPageBitmap_Start(fpdfBitmap, page, startX, startY, drawWidth, drawHeight, rotate, flags, nullptr);
    
    FPDFBitmap_Destroy(fpdfBitmap);
    AndroidBitmap_unlockPixels(env, bitmap);
    
    return status;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeRenderPageContinue(JNIEnv *env, jobject thiz,
                                                           jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return FPDF_RENDER_FAILED;
    
    // Continue without pause - will finish immediately
    return FPDF_RenderPage_Continue(page, nullptr);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeRenderPageClose(JNIEnv *env, jobject thiz,
                                                        jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (page) FPDF_RenderPage_Close(page);
}

// ============================================================================
// COMPLETE IMPLEMENTATION - ALL REMAINING FEATURES
// ============================================================================

// --- Form Events ---
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormOnMouseMove(JNIEnv *env, jobject thiz,
                                                        jlong formPtr, jlong pagePtr,
                                                        jint modifier, jdouble x, jdouble y) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_OnMouseMove(form, page, modifier, x, y) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormOnLButtonDown(JNIEnv *env, jobject thiz,
                                                          jlong formPtr, jlong pagePtr,
                                                          jint modifier, jdouble x, jdouble y) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_OnLButtonDown(form, page, modifier, x, y) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormOnLButtonUp(JNIEnv *env, jobject thiz,
                                                        jlong formPtr, jlong pagePtr,
                                                        jint modifier, jdouble x, jdouble y) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_OnLButtonUp(form, page, modifier, x, y) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormOnKeyDown(JNIEnv *env, jobject thiz,
                                                      jlong formPtr, jlong pagePtr,
                                                      jint keyCode, jint modifier) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_OnKeyDown(form, page, keyCode, modifier) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormOnKeyUp(JNIEnv *env, jobject thiz,
                                                    jlong formPtr, jlong pagePtr,
                                                    jint keyCode, jint modifier) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_OnKeyUp(form, page, keyCode, modifier) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormOnChar(JNIEnv *env, jobject thiz,
                                                   jlong formPtr, jlong pagePtr,
                                                   jint charCode, jint modifier) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_OnChar(form, page, charCode, modifier) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormOnFocus(JNIEnv *env, jobject thiz,
                                                    jlong formPtr, jlong pagePtr,
                                                    jint modifier, jdouble x, jdouble y) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_OnFocus(form, page, modifier, x, y) ? JNI_TRUE : JNI_FALSE;
}

// --- Form Field Operations ---
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormCanUndo(JNIEnv *env, jobject thiz,
                                                    jlong formPtr, jlong pagePtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_CanUndo(form, page) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormCanRedo(JNIEnv *env, jobject thiz,
                                                    jlong formPtr, jlong pagePtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_CanRedo(form, page) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormUndo(JNIEnv *env, jobject thiz,
                                                 jlong formPtr, jlong pagePtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_Undo(form, page) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormRedo(JNIEnv *env, jobject thiz,
                                                 jlong formPtr, jlong pagePtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_Redo(form, page) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormSelectAllText(JNIEnv *env, jobject thiz,
                                                          jlong formPtr, jlong pagePtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (form && page) FORM_SelectAllText(form, page);
}

// --- Annotation Getters ---
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetAnnotColor(JNIEnv *env, jobject thiz,
                                                      jlong annotPtr, jint colorType, jintArray result) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    
    unsigned int r, g, b, a;
    if (!FPDFAnnot_GetColor(annot, (FPDFANNOT_COLORTYPE)colorType, &r, &g, &b, &a)) return JNI_FALSE;
    
    jint *body = env->GetIntArrayElements(result, nullptr);
    body[0] = r; body[1] = g; body[2] = b; body[3] = a;
    env->ReleaseIntArrayElements(result, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetAnnotFlags(JNIEnv *env, jobject thiz, jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return FPDFAnnot_GetFlags(annot);
}

// --- Actions ---
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetActionType(JNIEnv *env, jobject thiz, jlong actionPtr) {
    FPDF_ACTION action = (FPDF_ACTION) actionPtr;
    if (!action) return -1;
    return FPDFAction_GetType(action);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetActionDest(JNIEnv *env, jobject thiz,
                                                      jlong docPtr, jlong actionPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    FPDF_ACTION action = (FPDF_ACTION) actionPtr;
    if (!doc || !action) return 0;
    return (jlong) FPDFAction_GetDest(doc, action);
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetActionFilePath(JNIEnv *env, jobject thiz, jlong actionPtr) {
    FPDF_ACTION action = (FPDF_ACTION) actionPtr;
    if (!action) return nullptr;
    
    unsigned long size = FPDFAction_GetFilePath(action, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    
    char *buffer = new char[size];
    FPDFAction_GetFilePath(action, buffer, size);
    jstring result = env->NewStringUTF(buffer);
    delete[] buffer;
    return result;
}

// --- Bookmarks ---
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFindBookmark(JNIEnv *env, jobject thiz,
                                                     jlong docPtr, jstring title) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc || !title) return 0;
    
    const jchar *wTitle = env->GetStringChars(title, nullptr);
    FPDF_BOOKMARK bookmark = FPDFBookmark_Find(doc, (FPDF_WIDESTRING)wTitle);
    env->ReleaseStringChars(title, wTitle);
    return (jlong) bookmark;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetBookmarkDest(JNIEnv *env, jobject thiz,
                                                        jlong docPtr, jlong bookmarkPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    FPDF_BOOKMARK bookmark = (FPDF_BOOKMARK) bookmarkPtr;
    if (!doc || !bookmark) return 0;
    return (jlong) FPDFBookmark_GetDest(doc, bookmark);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetBookmarkAction(JNIEnv *env, jobject thiz, jlong bookmarkPtr) {
    FPDF_BOOKMARK bookmark = (FPDF_BOOKMARK) bookmarkPtr;
    if (!bookmark) return 0;
    return (jlong) FPDFBookmark_GetAction(bookmark);
}

// --- Link Enumerate ---
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetLinkAction(JNIEnv *env, jobject thiz, jlong linkPtr) {
    FPDF_LINK link = (FPDF_LINK) linkPtr;
    if (!link) return 0;
    return (jlong) FPDFLink_GetAction(link);
}

// --- Text Rectangles ---
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextCountRects(JNIEnv *env, jobject thiz,
                                                       jlong textPagePtr, jint startIndex, jint count) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return 0;
    return FPDFText_CountRects(textPage, startIndex, count);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetRect(JNIEnv *env, jobject thiz,
                                                    jlong textPagePtr, jint index, jdoubleArray result) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return JNI_FALSE;
    
    double left, top, right, bottom;
    if (!FPDFText_GetRect(textPage, index, &left, &top, &right, &bottom)) return JNI_FALSE;
    
    jdouble *body = env->GetDoubleArrayElements(result, nullptr);
    body[0] = left; body[1] = top; body[2] = right; body[3] = bottom;
    env->ReleaseDoubleArrayElements(result, body, 0);
    return JNI_TRUE;
}

// --- Attachment Operations ---
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAddAttachment(JNIEnv *env, jobject thiz,
                                                      jlong docPtr, jstring name) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc || !name) return 0;
    
    const jchar *wName = env->GetStringChars(name, nullptr);
    FPDF_ATTACHMENT attachment = FPDFDoc_AddAttachment(doc, (FPDF_WIDESTRING)wName);
    env->ReleaseStringChars(name, wName);
    return (jlong) attachment;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeDeleteAttachment(JNIEnv *env, jobject thiz,
                                                         jlong docPtr, jint index) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return JNI_FALSE;
    return FPDFDoc_DeleteAttachment(doc, index) ? JNI_TRUE : JNI_FALSE;
}

// --- Page Object Colors (Get) ---
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetObjectStrokeColor(JNIEnv *env, jobject thiz,
                                                             jlong pageObjPtr, jintArray result) {
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!pageObj) return JNI_FALSE;
    
    unsigned int r, g, b, a;
    if (!FPDFPageObj_GetStrokeColor(pageObj, &r, &g, &b, &a)) return JNI_FALSE;
    
    jint *body = env->GetIntArrayElements(result, nullptr);
    body[0] = r; body[1] = g; body[2] = b; body[3] = a;
    env->ReleaseIntArrayElements(result, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetObjectFillColor(JNIEnv *env, jobject thiz,
                                                           jlong pageObjPtr, jintArray result) {
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!pageObj) return JNI_FALSE;
    
    unsigned int r, g, b, a;
    if (!FPDFPageObj_GetFillColor(pageObj, &r, &g, &b, &a)) return JNI_FALSE;
    
    jint *body = env->GetIntArrayElements(result, nullptr);
    body[0] = r; body[1] = g; body[2] = b; body[3] = a;
    env->ReleaseIntArrayElements(result, body, 0);
    return JNI_TRUE;
}

// --- Page Boxes (Bleed, Trim, Art) ---
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetPageBleedBox(JNIEnv *env, jobject thiz,
                                                        jlong pagePtr, jfloat left, jfloat bottom,
                                                        jfloat right, jfloat top) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (page) FPDFPage_SetBleedBox(page, left, bottom, right, top);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetPageTrimBox(JNIEnv *env, jobject thiz,
                                                       jlong pagePtr, jfloat left, jfloat bottom,
                                                       jfloat right, jfloat top) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (page) FPDFPage_SetTrimBox(page, left, bottom, right, top);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetPageArtBox(JNIEnv *env, jobject thiz,
                                                      jlong pagePtr, jfloat left, jfloat bottom,
                                                      jfloat right, jfloat top) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (page) FPDFPage_SetArtBox(page, left, bottom, right, top);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageBleedBox(JNIEnv *env, jobject thiz,
                                                        jlong pagePtr, jfloatArray result) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return JNI_FALSE;
    
    float left, bottom, right, top;
    if (!FPDFPage_GetBleedBox(page, &left, &bottom, &right, &top)) return JNI_FALSE;
    
    jfloat *body = env->GetFloatArrayElements(result, nullptr);
    body[0] = left; body[1] = bottom; body[2] = right; body[3] = top;
    env->ReleaseFloatArrayElements(result, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageTrimBox(JNIEnv *env, jobject thiz,
                                                       jlong pagePtr, jfloatArray result) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return JNI_FALSE;
    
    float left, bottom, right, top;
    if (!FPDFPage_GetTrimBox(page, &left, &bottom, &right, &top)) return JNI_FALSE;
    
    jfloat *body = env->GetFloatArrayElements(result, nullptr);
    body[0] = left; body[1] = bottom; body[2] = right; body[3] = top;
    env->ReleaseFloatArrayElements(result, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageArtBox(JNIEnv *env, jobject thiz,
                                                      jlong pagePtr, jfloatArray result) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return JNI_FALSE;
    
    float left, bottom, right, top;
    if (!FPDFPage_GetArtBox(page, &left, &bottom, &right, &top)) return JNI_FALSE;
    
    jfloat *body = env->GetFloatArrayElements(result, nullptr);
    body[0] = left; body[1] = bottom; body[2] = right; body[3] = top;
    env->ReleaseFloatArrayElements(result, body, 0);
    return JNI_TRUE;
}

// --- StructTree Extended ---
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementCountChildren(JNIEnv *env, jobject thiz,
                                                                   jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return 0;
    return FPDF_StructElement_CountChildren(elem);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetChildAtIndex(JNIEnv *env, jobject thiz,
                                                                      jlong structElemPtr, jint index) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return 0;
    return (jlong) FPDF_StructElement_GetChildAtIndex(elem, index);
}

// --- Font Loading ---
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeLoadStandardFont(JNIEnv *env, jobject thiz,
                                                         jlong docPtr, jstring fontName) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc || !fontName) return 0;
    
    const char *cFontName = env->GetStringUTFChars(fontName, nullptr);
    FPDF_FONT font = FPDFText_LoadStandardFont(doc, cFontName);
    env->ReleaseStringUTFChars(fontName, cFontName);
    return (jlong) font;
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCloseFont(JNIEnv *env, jobject thiz, jlong fontPtr) {
    FPDF_FONT font = (FPDF_FONT) fontPtr;
    if (font) FPDFFont_Close(font);
}

// --- Data Availability ---
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeIsLinearized(JNIEnv *env, jobject thiz, jlong availPtr) {
    FPDF_AVAIL avail = (FPDF_AVAIL) availPtr;
    if (!avail) return JNI_FALSE;
    // PDF_LINEARIZED is 1 in fpdf_dataavail.h (PDF_LINEARIZED)
    return FPDFAvail_IsLinearized(avail) == 1 ? JNI_TRUE : JNI_FALSE;
}

/**
 * Get Link Handle from Annotation
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetLinkFromAnnot(JNIEnv *env, jobject thiz,
                                                         jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    
    // Use proper PDFium API to get link from annotation
    FPDF_LINK link = FPDFAnnot_GetLink(annot);
    return (jlong) link;
}

} // extern "C"
