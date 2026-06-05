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
#include <fpdf_catalog.h>
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
#include <fpdf_ext.h>
#include <fpdf_searchex.h>

#define LOG_TAG "KotlinPdfium"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static JavaVM* g_vm = nullptr;
static int libraryReferenceCount = 0;
static std::map<FPDF_DOCUMENT, char*> g_docBuffers;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

// --- fpdf_ext.h callback infrastructure ---
static void UnSpObjHandlerCallback(struct _UNSUPPORT_INFO* pThis, int nType) {
    JNIEnv* env;
    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) return;
    jclass clazz = env->FindClass("com/hyntix/pdfium/PdfiumCore");
    if (!clazz) return;
    jmethodID method = env->GetStaticMethodID(clazz, "onUnsupportedObject", "(I)V");
    if (method) env->CallStaticVoidMethod(clazz, method, nType);
    env->DeleteLocalRef(clazz);
}

static UNSUPPORT_INFO g_unsupportInfo = {1, UnSpObjHandlerCallback};
// ---

// --- fpdf_dataavail.h callback infrastructure ---
// Forward declarations of JNI callbacks for data avail
static FPDF_BOOL IsDataAvailCallback(struct _FX_FILEAVAIL* pThis, size_t offset, size_t size);
static int GetBlockCallback(void* param, unsigned long position, unsigned char* pBuf, unsigned long size);
static void AddSegmentCallback(struct _FX_DOWNLOADHINTS* pThis, size_t offset, size_t size);

// --- AvailData: wraps FX_FILEAVAIL + FPDF_FILEACCESS for FPDFAvail_Create ---
struct AvailData {
    unsigned char* fileData;
    unsigned long fileLen;
    FX_FILEAVAIL fileAvailStruct;
    FPDF_FILEACCESS fileAccessStruct;
};

static std::map<FPDF_AVAIL, AvailData*> g_availDataMap;

static FPDF_BOOL IsDataAvailCallback(struct _FX_FILEAVAIL* pThis, size_t offset, size_t size) {
    return 1;
}

static int GetBlockCallback(void* param, unsigned long position, unsigned char* pBuf, unsigned long size) {
    AvailData* data = (AvailData*) param;
    if (!data || position + size > data->fileLen) return 0;
    memcpy(pBuf, data->fileData + position, size);
    return 1;
}

static void AddSegmentCallback(struct _FX_DOWNLOADHINTS* pThis, size_t offset, size_t size) {
}

// Helper for synchronous FPDF_FILEACCESS callbacks (LoadJpegFile, etc.)
struct SyncFileReadCtx {
    unsigned char* data;
    unsigned long len;
};

static int SyncFileReadBlock(void* param, unsigned long position, unsigned char* pBuf, unsigned long size) {
    SyncFileReadCtx* ctx = (SyncFileReadCtx*) param;
    if (!ctx || position + size > ctx->len) return 0;
    memcpy(pBuf, ctx->data + position, size);
    return 1;
}
// ---

extern "C" {

/**
 * Initialize PDFium library
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeInitLibrary(JNIEnv *env, jobject thiz) {
    if (libraryReferenceCount == 0) {
        FPDF_LIBRARY_CONFIG config = {};
        config.version = 2;
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

// --- fpdf_ext.h ---
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetUnSpObjProcessHandler(JNIEnv *env, jobject thiz) {
    return FSDK_SetUnSpObjProcessHandler(&g_unsupportInfo) ? JNI_TRUE : JNI_FALSE;
}

// --- fpdfview.h Additional Bindings ---

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetSandBoxPolicy(JNIEnv *env, jobject thiz,
                                                        jint policy, jboolean enabled) {
    FPDF_SetSandBoxPolicy((FPDF_DWORD) policy, enabled ? 1 : 0);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetPrintMode(JNIEnv *env, jobject thiz, jint mode) {
    return JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetFileVersion(JNIEnv *env, jobject thiz,
                                                      jlong docPtr, jintArray version) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return JNI_FALSE;
    int fileVersion = 0;
    if (!FPDF_GetFileVersion(doc, &fileVersion)) return JNI_FALSE;
    jint *body = env->GetIntArrayElements(version, nullptr);
    body[0] = fileVersion;
    env->ReleaseIntArrayElements(version, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageWidthFFloat(JNIEnv *env, jobject thiz,
                                                          jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0;
    return (int)(FPDF_GetPageWidthF(page) * 1000000);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageHeightFFloat(JNIEnv *env, jobject thiz,
                                                           jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0;
    return (int)(FPDF_GetPageHeightF(page) * 1000000);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageBoundingBoxFFloat(JNIEnv *env, jobject thiz,
                                                                jlong pagePtr, jfloatArray rect) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return JNI_FALSE;
    FS_RECTF fRect;
    if (!FPDF_GetPageBoundingBox(page, &fRect)) return JNI_FALSE;
    jfloat *body = env->GetFloatArrayElements(rect, nullptr);
    body[0] = fRect.left; body[1] = fRect.top;
    body[2] = fRect.right; body[3] = fRect.bottom;
    env->ReleaseFloatArrayElements(rect, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeBitmapGetFormat(JNIEnv *env, jobject thiz,
                                                       jlong bitmapPtr) {
    FPDF_BITMAP bitmap = (FPDF_BITMAP) bitmapPtr;
    if (!bitmap) return -1;
    return FPDFBitmap_GetFormat(bitmap);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetDefaultPrinterMode(JNIEnv *env, jobject thiz,
                                                              jint mode) {
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetDefaultPrinterMode(JNIEnv *env, jobject thiz) {
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetDuplexOperation(JNIEnv *env, jobject thiz,
                                                           jlong docPtr) {
    return 0;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetRecommendedV8Flags(JNIEnv *env, jobject thiz) {
    return nullptr;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetArrayBufferAllocatorSharedInstance(JNIEnv *env,
                                                                              jobject thiz) {
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetXFAPacketCount(JNIEnv *env, jobject thiz,
                                                         jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return FPDF_GetXFAPacketCount(doc);
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetXFAPacketName(JNIEnv *env, jobject thiz,
                                                        jlong docPtr, jint index) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return nullptr;
    unsigned long size = FPDF_GetXFAPacketName(doc, index, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    char *buffer = new char[size];
    FPDF_GetXFAPacketName(doc, index, buffer, size);
    jstring result = env->NewStringUTF(buffer);
    delete[] buffer;
    return result;
}

JNIEXPORT jbyteArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetXFAPacketContent(JNIEnv *env, jobject thiz,
                                                           jlong docPtr, jint index) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return nullptr;
    unsigned long outLen = 0;
    FPDF_GetXFAPacketContent(doc, index, nullptr, 0, &outLen);
    if (outLen == 0) return nullptr;
    unsigned char *buffer = new unsigned char[outLen];
    if (!FPDF_GetXFAPacketContent(doc, index, buffer, outLen, &outLen)) {
        delete[] buffer;
        return nullptr;
    }
    jbyteArray result = env->NewByteArray(outLen);
    env->SetByteArrayRegion(result, 0, outLen, (jbyte*) buffer);
    delete[] buffer;
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeBStrInit(JNIEnv *env, jobject thiz, jlong bstrPtr) {
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeBStrSet(JNIEnv *env, jobject thiz,
                                                jlong bstrPtr, jstring str) {
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeBStrClear(JNIEnv *env, jobject thiz, jlong bstrPtr) {
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

// --- FPDF_LoadMemDocument64 (64-bit size) ---
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeOpenMemDocument64(JNIEnv *env, jobject thiz,
                                                          jbyteArray data, jstring password) {
    const char *cPassword = nullptr;
    if (password != nullptr) {
        cPassword = env->GetStringUTFChars(password, nullptr);
    }

    jsize length = env->GetArrayLength(data);
    jbyte *buffer = env->GetByteArrayElements(data, nullptr);

    char* docBuffer = new char[(size_t)length];
    memcpy(docBuffer, buffer, (size_t)length);

    FPDF_DOCUMENT doc = FPDF_LoadMemDocument64(docBuffer, (size_t)length, cPassword);

    env->ReleaseByteArrayElements(data, buffer, 0);
    if (password != nullptr) {
        env->ReleaseStringUTFChars(password, cPassword);
    }

    if (!doc) {
        delete[] docBuffer;
        LOGE("Failed to load document from memory (64), error: %lu", FPDF_GetLastError());
        return 0;
    }

    g_docBuffers[doc] = docBuffer;
    LOGI("Document opened from memory (64-bit), pages: %d", FPDF_GetPageCount(doc));
    return (jlong) doc;
}
// ---

// --- FPDF_LoadCustomDocument (FPDF_FILEACCESS callback) ---
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeLoadCustomDocument(JNIEnv *env, jobject thiz,
                                                           jbyteArray data, jstring password) {
    const char *cPassword = nullptr;
    if (password != nullptr) {
        cPassword = env->GetStringUTFChars(password, nullptr);
    }

    jsize length = env->GetArrayLength(data);
    jbyte *elements = env->GetByteArrayElements(data, nullptr);

    SyncFileReadCtx* ctx = new SyncFileReadCtx();
    ctx->data = new unsigned char[(size_t)length];
    ctx->len = (unsigned long)length;
    memcpy(ctx->data, elements, (size_t)length);

    FPDF_FILEACCESS fileAccess;
    fileAccess.m_FileLen = (unsigned long)length;
    fileAccess.m_GetBlock = SyncFileReadBlock;
    fileAccess.m_Param = ctx;

    FPDF_DOCUMENT doc = FPDF_LoadCustomDocument(&fileAccess, cPassword);

    env->ReleaseByteArrayElements(data, elements, JNI_ABORT);
    if (password != nullptr) {
        env->ReleaseStringUTFChars(password, cPassword);
    }

    if (!doc) {
        delete[] ctx->data;
        delete ctx;
        LOGE("Failed to load custom document, error: %lu", FPDF_GetLastError());
        return 0;
    }

    g_docBuffers[doc] = (char*)ctx->data;
    LOGI("Custom document loaded from memory, pages: %d", FPDF_GetPageCount(doc));
    return (jlong) doc;
}
// ---

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
 * Get Character Index from Text Index
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetCharIndexFromTextIndex(JNIEnv *env, jobject thiz,
                                                                  jlong textPagePtr,
                                                                  jint nTextIndex) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return -1;
    return FPDFText_GetCharIndexFromTextIndex(textPage, nTextIndex);
}

/**
 * Get Text Index from Character Index
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetTextIndexFromCharIndex(JNIEnv *env, jobject thiz,
                                                                  jlong textPagePtr,
                                                                  jint nCharIndex) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return -1;
    return FPDFText_GetTextIndexFromCharIndex(textPage, nCharIndex);
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

/**
 * Save Document With Version
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSaveDocumentWithVersion(JNIEnv *env, jobject thiz,
                                                                jlong docPtr, jstring path,
                                                                jint fileVersion) {
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

    bool success = FPDF_SaveWithVersion(doc, &writer, 0, fileVersion);

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

/**
 * Object Management
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeInsertObject(JNIEnv *env, jobject thiz,
                                                     jlong pagePtr, jlong pageObjPtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (page && pageObj) {
        return FPDFPage_InsertObject(page, pageObj) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
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
 * Import Pages By Index
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImportPagesByIndex(JNIEnv *env, jobject thiz,
                                                           jlong destDocPtr, jlong srcDocPtr,
                                                           jintArray pageIndices, jint insertIndex) {
    FPDF_DOCUMENT destDoc = (FPDF_DOCUMENT) destDocPtr;
    FPDF_DOCUMENT srcDoc = (FPDF_DOCUMENT) srcDocPtr;
    if (!destDoc || !srcDoc || !pageIndices) return JNI_FALSE;

    jsize len = env->GetArrayLength(pageIndices);
    jint *indices = env->GetIntArrayElements(pageIndices, nullptr);

    jboolean result = FPDF_ImportPagesByIndex(destDoc, srcDoc, indices, len, insertIndex) ? JNI_TRUE : JNI_FALSE;

    env->ReleaseIntArrayElements(pageIndices, indices, JNI_ABORT);
    return result;
}

/**
 * Import N Pages To One
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImportNPagesToOne(JNIEnv *env, jobject thiz,
                                                          jlong srcDocPtr,
                                                          jfloat outputWidth, jfloat outputHeight,
                                                          jint numPagesX, jint numPagesY) {
    FPDF_DOCUMENT srcDoc = (FPDF_DOCUMENT) srcDocPtr;
    if (!srcDoc) return 0;
    return (jlong) FPDF_ImportNPagesToOne(srcDoc, outputWidth, outputHeight, numPagesX, numPagesY);
}

/**
 * New XObject From Page
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeNewXObjectFromPage(JNIEnv *env, jobject thiz,
                                                           jlong destDocPtr, jlong srcDocPtr,
                                                           jint srcPageIndex) {
    FPDF_DOCUMENT destDoc = (FPDF_DOCUMENT) destDocPtr;
    FPDF_DOCUMENT srcDoc = (FPDF_DOCUMENT) srcDocPtr;
    if (!destDoc || !srcDoc) return 0;
    return (jlong) FPDF_NewXObjectFromPage(destDoc, srcDoc, srcPageIndex);
}

/**
 * Close XObject
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCloseXObject(JNIEnv *env, jobject thiz,
                                                     jlong xobjectPtr) {
    FPDF_XOBJECT xobj = (FPDF_XOBJECT) xobjectPtr;
    if (!xobj) return;
    FPDF_CloseXObject(xobj);
}

/**
 * New Form Object From XObject
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeNewFormObjectFromXObject(JNIEnv *env, jobject thiz,
                                                                 jlong xobjectPtr) {
    FPDF_XOBJECT xobj = (FPDF_XOBJECT) xobjectPtr;
    if (!xobj) return 0;
    return (jlong) FPDF_NewFormObjectFromXObject(xobj);
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
 * Get Thumbnail As Bitmap
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetThumbnailAsBitmap(JNIEnv *env, jobject thiz,
                                                             jlong pagePtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return 0;
    return (jlong) FPDFPage_GetThumbnailAsBitmap(page);
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
 * Get Signature Byte Range
 */
JNIEXPORT jintArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetSignatureByteRange(JNIEnv *env, jobject thiz,
                                                              jlong sigObjPtr) {
    FPDF_SIGNATURE sig = (FPDF_SIGNATURE) sigObjPtr;
    if (!sig) return nullptr;

    unsigned long size = FPDFSignatureObj_GetByteRange(sig, nullptr, 0);
    if (size == 0) return nullptr;

    int *buffer = new int[size];
    FPDFSignatureObj_GetByteRange(sig, buffer, size);

    jintArray result = env->NewIntArray(size);
    env->SetIntArrayRegion(result, 0, size, buffer);
    delete[] buffer;
    return result;
}

/**
 * Get Signature Sub Filter
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetSignatureSubFilter(JNIEnv *env, jobject thiz,
                                                              jlong sigObjPtr) {
    FPDF_SIGNATURE sig = (FPDF_SIGNATURE) sigObjPtr;
    if (!sig) return nullptr;

    unsigned long size = FPDFSignatureObj_GetSubFilter(sig, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");

    char *buffer = new char[size];
    FPDFSignatureObj_GetSubFilter(sig, buffer, size);
    jstring result = env->NewStringUTF(buffer);
    delete[] buffer;
    return result;
}

/**
 * Get Signature DocMDP Permission
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetSignatureDocMDPPermission(JNIEnv *env, jobject thiz,
                                                                     jlong sigObjPtr) {
    FPDF_SIGNATURE sig = (FPDF_SIGNATURE) sigObjPtr;
    if (!sig) return 0;
    return (jint) FPDFSignatureObj_GetDocMDPPermission(sig);
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

/**
 * Get JavaScript Action
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetJavaScriptAction(JNIEnv *env, jobject thiz,
                                                            jlong docPtr, jint index) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return (jlong) FPDFDoc_GetJavaScriptAction(doc, index);
}

/**
 * Close JavaScript Action
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCloseJavaScriptAction(JNIEnv *env, jobject thiz,
                                                              jlong jsActionPtr) {
    FPDF_JAVASCRIPT_ACTION jsAction = (FPDF_JAVASCRIPT_ACTION) jsActionPtr;
    if (jsAction) FPDFDoc_CloseJavaScriptAction(jsAction);
}

/**
 * Get JavaScript Action Name
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetJavaScriptActionName(JNIEnv *env, jobject thiz,
                                                                jlong jsActionPtr) {
    FPDF_JAVASCRIPT_ACTION jsAction = (FPDF_JAVASCRIPT_ACTION) jsActionPtr;
    if (!jsAction) return nullptr;
    unsigned long size = FPDFJavaScriptAction_GetName(jsAction, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDFJavaScriptAction_GetName(jsAction, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

/**
 * Get JavaScript Action Script
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetJavaScriptActionScript(JNIEnv *env, jobject thiz,
                                                                  jlong jsActionPtr) {
    FPDF_JAVASCRIPT_ACTION jsAction = (FPDF_JAVASCRIPT_ACTION) jsActionPtr;
    if (!jsAction) return nullptr;
    unsigned long size = FPDFJavaScriptAction_GetScript(jsAction, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDFJavaScriptAction_GetScript(jsAction, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
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

/**
 * Render Page Bitmap With Color Scheme Start
 */
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeRenderPageBitmapWithColorSchemeStart(
    JNIEnv *env, jobject thiz,
    jlong bitmapPtr, jlong pagePtr,
    jint startX, jint startY, jint sizeX, jint sizeY,
    jint rotate, jint flags, jintArray colorScheme) {
    FPDF_BITMAP bitmap = (FPDF_BITMAP) bitmapPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!bitmap || !page || !colorScheme) return FPDF_RENDER_FAILED;

    jint *colors = env->GetIntArrayElements(colorScheme, nullptr);
    FPDF_COLORSCHEME scheme;
    scheme.path_fill_color = (FPDF_DWORD) colors[0];
    scheme.path_stroke_color = (FPDF_DWORD) colors[1];
    scheme.text_fill_color = (FPDF_DWORD) colors[2];
    scheme.text_stroke_color = (FPDF_DWORD) colors[3];
    env->ReleaseIntArrayElements(colorScheme, colors, JNI_ABORT);

    return FPDF_RenderPageBitmapWithColorScheme_Start(
        bitmap, page, startX, startY, sizeX, sizeY, rotate, flags, &scheme, nullptr);
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

// --- Additional Form Fill Functions ---

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormDoDocumentJSAction(JNIEnv *env, jobject thiz,
                                                               jlong formPtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    if (form) FORM_DoDocumentJSAction(form);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormDoDocumentOpenAction(JNIEnv *env, jobject thiz,
                                                                  jlong formPtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    if (form) FORM_DoDocumentOpenAction(form);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormDoDocumentAAction(JNIEnv *env, jobject thiz,
                                                              jlong formPtr, jint aaType) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    if (form) FORM_DoDocumentAAction(form, aaType);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormDoPageAAction(JNIEnv *env, jobject thiz,
                                                          jlong formPtr, jlong pagePtr, jint aaType) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (form && page) FORM_DoPageAAction(page, form, aaType);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormOnMouseWheel(JNIEnv *env, jobject thiz,
                                                         jlong formPtr, jlong pagePtr,
                                                         jint modifier, jdouble x, jdouble y,
                                                         jint deltaX, jint deltaY) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    FS_POINTF pt = {(float)x, (float)y};
    return FORM_OnMouseWheel(form, page, modifier, &pt, deltaX, deltaY) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormOnRButtonDown(JNIEnv *env, jobject thiz,
                                                          jlong formPtr, jlong pagePtr,
                                                          jint modifier, jdouble x, jdouble y) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_OnRButtonDown(form, page, modifier, x, y) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormOnRButtonUp(JNIEnv *env, jobject thiz,
                                                        jlong formPtr, jlong pagePtr,
                                                        jint modifier, jdouble x, jdouble y) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_OnRButtonUp(form, page, modifier, x, y) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormOnLButtonDoubleClick(JNIEnv *env, jobject thiz,
                                                                 jlong formPtr, jlong pagePtr,
                                                                 jint modifier, jdouble x, jdouble y) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_OnLButtonDoubleClick(form, page, modifier, x, y) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormGetFocusedText(JNIEnv *env, jobject thiz,
                                                           jlong formPtr, jlong pagePtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return nullptr;
    unsigned long size = FORM_GetFocusedText(form, page, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FORM_GetFocusedText(form, page, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormGetSelectedText(JNIEnv *env, jobject thiz,
                                                            jlong formPtr, jlong pagePtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return nullptr;
    unsigned long size = FORM_GetSelectedText(form, page, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FORM_GetSelectedText(form, page, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormReplaceAndKeepSelection(JNIEnv *env, jobject thiz,
                                                                    jlong formPtr, jlong pagePtr,
                                                                    jstring text) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return;
    jsize len = env->GetStringLength(text);
    const jchar *chars = env->GetStringChars(text, nullptr);
    FORM_ReplaceAndKeepSelection(form, page, (FPDF_WIDESTRING) chars);
    env->ReleaseStringChars(text, chars);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormReplaceSelection(JNIEnv *env, jobject thiz,
                                                              jlong formPtr, jlong pagePtr,
                                                              jstring text) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return;
    jsize len = env->GetStringLength(text);
    const jchar *chars = env->GetStringChars(text, nullptr);
    FORM_ReplaceSelection(form, page, (FPDF_WIDESTRING) chars);
    env->ReleaseStringChars(text, chars);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormGetFocusedAnnot(JNIEnv *env, jobject thiz,
                                                            jlong formPtr, jintArray pageIndex, jlongArray annotPtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    if (!form) return JNI_FALSE;
    int pageIdx = 0;
    FPDF_ANNOTATION annot = nullptr;
    FPDF_BOOL result = FORM_GetFocusedAnnot(form, &pageIdx, &annot);
    if (result) {
        jint *idxBody = env->GetIntArrayElements(pageIndex, nullptr);
        idxBody[0] = pageIdx;
        env->ReleaseIntArrayElements(pageIndex, idxBody, 0);
        jlong *annBody = env->GetLongArrayElements(annotPtr, nullptr);
        annBody[0] = (jlong) annot;
        env->ReleaseLongArrayElements(annotPtr, annBody, 0);
    }
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormSetFocusedAnnot(JNIEnv *env, jobject thiz,
                                                            jlong formPtr, jlong annotPtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!form || !annot) return JNI_FALSE;
    return FORM_SetFocusedAnnot(form, annot) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageHasFormFieldAtPoint(JNIEnv *env, jobject thiz,
                                                                jlong formPtr, jlong pagePtr,
                                                                jdouble x, jdouble y) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return -1;
    return FPDFPage_HasFormFieldAtPoint(form, page, x, y);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageFormFieldZOrderAtPoint(JNIEnv *env, jobject thiz,
                                                                    jlong formPtr, jlong pagePtr,
                                                                    jdouble x, jdouble y) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return -1;
    return FPDFPage_FormFieldZOrderAtPoint(form, page, x, y);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetFormFieldHighlightColor(JNIEnv *env, jobject thiz,
                                                                   jlong formPtr, jint fieldType,
                                                                   jlong color) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    if (form) FPDF_SetFormFieldHighlightColor(form, fieldType, (unsigned long) color);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetFormFieldHighlightAlpha(JNIEnv *env, jobject thiz,
                                                                   jlong formPtr, jint alpha) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    if (form) FPDF_SetFormFieldHighlightAlpha(form, (unsigned char) alpha);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeRemoveFormFieldHighlight(JNIEnv *env, jobject thiz,
                                                                 jlong formPtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    if (form) FPDF_RemoveFormFieldHighlight(form);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormSetIndexSelected(JNIEnv *env, jobject thiz,
                                                             jlong formPtr, jlong pagePtr,
                                                             jint index, jboolean selected) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_SetIndexSelected(form, page, index, selected ? 1 : 0) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormIsIndexSelected(JNIEnv *env, jobject thiz,
                                                            jlong formPtr, jlong pagePtr,
                                                            jint index) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!form || !page) return JNI_FALSE;
    return FORM_IsIndexSelected(form, page, index) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeLoadXFA(JNIEnv *env, jobject thiz,
                                                jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return JNI_FALSE;
    return FPDF_LoadXFA(doc) ? JNI_TRUE : JNI_FALSE;
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

// --- fpdf_doc.h Additional Bindings ---

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeBookmarkGetCount(JNIEnv *env, jobject thiz,
                                                        jlong bookmarkPtr) {
    FPDF_BOOKMARK bookmark = (FPDF_BOOKMARK) bookmarkPtr;
    if (!bookmark) return 0;
    return FPDFBookmark_GetCount(bookmark);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeLinkGetLinkZOrderAtPoint(JNIEnv *env, jobject thiz,
                                                                 jlong pagePtr, jdouble x, jdouble y) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return -1;
    return FPDFLink_GetLinkZOrderAtPoint(page, x, y);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeLinkEnumerate(JNIEnv *env, jobject thiz,
                                                     jlong pagePtr, jintArray startIndex,
                                                     jlongArray linkPtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return JNI_FALSE;
    jint *body = env->GetIntArrayElements(startIndex, nullptr);
    int startIdx = body[0];
    FPDF_LINK link = nullptr;
    if (!FPDFLink_Enumerate(page, &startIdx, &link)) {
        env->ReleaseIntArrayElements(startIndex, body, 0);
        return JNI_FALSE;
    }
    body[0] = startIdx;
    env->ReleaseIntArrayElements(startIndex, body, 0);
    jlong *linkBody = env->GetLongArrayElements(linkPtr, nullptr);
    linkBody[0] = (jlong) link;
    env->ReleaseLongArrayElements(linkPtr, linkBody, 0);
    return JNI_TRUE;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeLinkGetAnnot(JNIEnv *env, jobject thiz,
                                                     jlong pagePtr, jlong linkPtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    FPDF_LINK link = (FPDF_LINK) linkPtr;
    if (!page || !link) return 0;
    return (jlong) FPDFLink_GetAnnot(page, link);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeLinkCountQuadPoints(JNIEnv *env, jobject thiz,
                                                           jlong linkPtr) {
    FPDF_LINK link = (FPDF_LINK) linkPtr;
    if (!link) return 0;
    return FPDFLink_CountQuadPoints(link);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeLinkGetQuadPoints(JNIEnv *env, jobject thiz,
                                                          jlong linkPtr, jint quadIndex,
                                                          jfloatArray result) {
    FPDF_LINK link = (FPDF_LINK) linkPtr;
    if (!link) return JNI_FALSE;

    FS_QUADPOINTSF quadPoints;
    if (!FPDFLink_GetQuadPoints(link, quadIndex, &quadPoints)) return JNI_FALSE;

    jfloat *body = env->GetFloatArrayElements(result, nullptr);
    body[0] = quadPoints.x1;
    body[1] = quadPoints.y1;
    body[2] = quadPoints.x2;
    body[3] = quadPoints.y2;
    body[4] = quadPoints.x3;
    body[5] = quadPoints.y3;
    body[6] = quadPoints.x4;
    body[7] = quadPoints.y4;
    env->ReleaseFloatArrayElements(result, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeDestGetView(JNIEnv *env, jobject thiz,
                                                     jlong destPtr, jlongArray numParams,
                                                     jfloatArray params) {
    FPDF_DEST dest = (FPDF_DEST) destPtr;
    if (!dest) return 0;
    unsigned long numParamsVal = 0;
    float paramsBuf[4] = {0};
    unsigned long result = FPDFDest_GetView(dest, &numParamsVal, paramsBuf);
    jlong *numBody = env->GetLongArrayElements(numParams, nullptr);
    numBody[0] = numParamsVal;
    env->ReleaseLongArrayElements(numParams, numBody, 0);
    jfloat *paramBody = env->GetFloatArrayElements(params, nullptr);
    for (int i = 0; i < 4 && i < numParamsVal; i++) paramBody[i] = paramsBuf[i];
    env->ReleaseFloatArrayElements(params, paramBody, 0);
    return (jlong) result;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeDestGetLocationInPage(JNIEnv *env, jobject thiz,
                                                             jlong destPtr,
                                                             jbooleanArray hasXYZ,
                                                             jfloatArray location) {
    FPDF_DEST dest = (FPDF_DEST) destPtr;
    if (!dest) return JNI_FALSE;
    FPDF_BOOL hasX = 0, hasY = 0, hasZoom = 0;
    float x = 0, y = 0, zoom = 0;
    if (!FPDFDest_GetLocationInPage(dest, &hasX, &hasY, &hasZoom, &x, &y, &zoom)) return JNI_FALSE;
    jboolean *hasBody = env->GetBooleanArrayElements(hasXYZ, nullptr);
    hasBody[0] = hasX ? JNI_TRUE : JNI_FALSE;
    hasBody[1] = hasY ? JNI_TRUE : JNI_FALSE;
    hasBody[2] = hasZoom ? JNI_TRUE : JNI_FALSE;
    env->ReleaseBooleanArrayElements(hasXYZ, hasBody, 0);
    jfloat *locBody = env->GetFloatArrayElements(location, nullptr);
    locBody[0] = x; locBody[1] = y; locBody[2] = zoom;
    env->ReleaseFloatArrayElements(location, locBody, 0);
    return JNI_TRUE;
}

JNIEXPORT jbyteArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetFileIdentifier(JNIEnv *env, jobject thiz,
                                                         jlong docPtr, jint idType) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return nullptr;
    unsigned long size = FPDF_GetFileIdentifier(doc, (FPDF_FILEIDTYPE) idType, nullptr, 0);
    if (size == 0) return nullptr;
    unsigned char *buffer = new unsigned char[size];
    FPDF_GetFileIdentifier(doc, (FPDF_FILEIDTYPE) idType, buffer, size);
    jbyteArray result = env->NewByteArray(size);
    env->SetByteArrayRegion(result, 0, size, (jbyte*) buffer);
    delete[] buffer;
    return result;
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

// --- Attachment Extended Bindings ---

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAttachmentHasKey(JNIEnv *env, jobject thiz,
                                                         jlong attachmentPtr, jstring key) {
    FPDF_ATTACHMENT attachment = (FPDF_ATTACHMENT) attachmentPtr;
    if (!attachment || !key) return JNI_FALSE;
    const char *cKey = env->GetStringUTFChars(key, nullptr);
    FPDF_BOOL result = FPDFAttachment_HasKey(attachment, cKey);
    env->ReleaseStringUTFChars(key, cKey);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAttachmentGetValueType(JNIEnv *env, jobject thiz,
                                                               jlong attachmentPtr, jstring key) {
    FPDF_ATTACHMENT attachment = (FPDF_ATTACHMENT) attachmentPtr;
    if (!attachment || !key) return 0;
    const char *cKey = env->GetStringUTFChars(key, nullptr);
    jint result = (jint) FPDFAttachment_GetValueType(attachment, cKey);
    env->ReleaseStringUTFChars(key, cKey);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAttachmentSetStringValue(JNIEnv *env, jobject thiz,
                                                                 jlong attachmentPtr, jstring key,
                                                                 jstring value) {
    FPDF_ATTACHMENT attachment = (FPDF_ATTACHMENT) attachmentPtr;
    if (!attachment || !key || !value) return JNI_FALSE;
    const char *cKey = env->GetStringUTFChars(key, nullptr);
    const jchar *wValue = env->GetStringChars(value, nullptr);
    FPDF_BOOL result = FPDFAttachment_SetStringValue(attachment, cKey, (FPDF_WIDESTRING) wValue);
    env->ReleaseStringChars(value, wValue);
    env->ReleaseStringUTFChars(key, cKey);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAttachmentGetStringValue(JNIEnv *env, jobject thiz,
                                                                 jlong attachmentPtr, jstring key) {
    FPDF_ATTACHMENT attachment = (FPDF_ATTACHMENT) attachmentPtr;
    if (!attachment || !key) return nullptr;
    const char *cKey = env->GetStringUTFChars(key, nullptr);
    unsigned long size = FPDFAttachment_GetStringValue(attachment, cKey, nullptr, 0);
    if (size == 0) { env->ReleaseStringUTFChars(key, cKey); return env->NewStringUTF(""); }
    unsigned short *buffer = new unsigned short[size];
    FPDFAttachment_GetStringValue(attachment, cKey, buffer, size);
    env->ReleaseStringUTFChars(key, cKey);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAttachmentSetFile(JNIEnv *env, jobject thiz,
                                                          jlong attachmentPtr, jlong docPtr,
                                                          jbyteArray contents) {
    FPDF_ATTACHMENT attachment = (FPDF_ATTACHMENT) attachmentPtr;
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!attachment || !doc || !contents) return JNI_FALSE;
    jsize len = env->GetArrayLength(contents);
    jbyte *data = env->GetByteArrayElements(contents, nullptr);
    FPDF_BOOL result = FPDFAttachment_SetFile(attachment, doc, data, len);
    env->ReleaseByteArrayElements(contents, data, JNI_ABORT);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAttachmentGetSubtype(JNIEnv *env, jobject thiz,
                                                             jlong attachmentPtr) {
    FPDF_ATTACHMENT attachment = (FPDF_ATTACHMENT) attachmentPtr;
    if (!attachment) return nullptr;
    unsigned long size = FPDFAttachment_GetSubtype(attachment, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDFAttachment_GetSubtype(attachment, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
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

// --- Clip Path Operations ---

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTransformPageWithClip(JNIEnv *env, jobject thiz,
                                                             jlong pagePtr,
                                                             jfloat a, jfloat b, jfloat c,
                                                             jfloat d, jfloat e, jfloat f,
                                                             jfloat clipLeft, jfloat clipBottom,
                                                             jfloat clipRight, jfloat clipTop) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page) return JNI_FALSE;
    FS_MATRIX matrix = {a, b, c, d, e, f};
    FS_RECTF clipRect = {clipLeft, clipBottom, clipRight, clipTop};
    return FPDFPage_TransFormWithClip(page, &matrix, &clipRect) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTransformClipPath(JNIEnv *env, jobject thiz,
                                                         jlong pageObjPtr,
                                                         jdouble a, jdouble b, jdouble c,
                                                         jdouble d, jdouble e, jdouble f) {
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (pageObj) FPDFPageObj_TransformClipPath(pageObj, a, b, c, d, e, f);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageObjectClipPath(JNIEnv *env, jobject thiz,
                                                             jlong pageObjPtr) {
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!pageObj) return 0;
    return (jlong) FPDFPageObj_GetClipPath(pageObj);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeClipPathCountPaths(JNIEnv *env, jobject thiz,
                                                          jlong clipPathPtr) {
    FPDF_CLIPPATH clipPath = (FPDF_CLIPPATH) clipPathPtr;
    if (!clipPath) return -1;
    return FPDFClipPath_CountPaths(clipPath);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeClipPathCountPathSegments(JNIEnv *env, jobject thiz,
                                                                 jlong clipPathPtr, jint pathIndex) {
    FPDF_CLIPPATH clipPath = (FPDF_CLIPPATH) clipPathPtr;
    if (!clipPath) return -1;
    return FPDFClipPath_CountPathSegments(clipPath, pathIndex);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeClipPathGetPathSegment(JNIEnv *env, jobject thiz,
                                                              jlong clipPathPtr, jint pathIndex,
                                                              jint segmentIndex) {
    FPDF_CLIPPATH clipPath = (FPDF_CLIPPATH) clipPathPtr;
    if (!clipPath) return 0;
    return (jlong) FPDFClipPath_GetPathSegment(clipPath, pathIndex, segmentIndex);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCreateClipPath(JNIEnv *env, jobject thiz,
                                                      jfloat left, jfloat bottom,
                                                      jfloat right, jfloat top) {
    return (jlong) FPDF_CreateClipPath(left, bottom, right, top);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeDestroyClipPath(JNIEnv *env, jobject thiz,
                                                       jlong clipPathPtr) {
    FPDF_CLIPPATH clipPath = (FPDF_CLIPPATH) clipPathPtr;
    if (clipPath) FPDF_DestroyClipPath(clipPath);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeInsertClipPath(JNIEnv *env, jobject thiz,
                                                      jlong pagePtr, jlong clipPathPtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    FPDF_CLIPPATH clipPath = (FPDF_CLIPPATH) clipPathPtr;
    if (page && clipPath) FPDFPage_InsertClipPath(page, clipPath);
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

// --- StructElement Additional Bindings ---

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetActualText(JNIEnv *env, jobject thiz,
                                                                   jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return nullptr;
    unsigned long size = FPDF_StructElement_GetActualText(elem, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDF_StructElement_GetActualText(elem, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetID(JNIEnv *env, jobject thiz,
                                                           jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return nullptr;
    unsigned long size = FPDF_StructElement_GetID(elem, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDF_StructElement_GetID(elem, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetLang(JNIEnv *env, jobject thiz,
                                                             jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return nullptr;
    unsigned long size = FPDF_StructElement_GetLang(elem, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDF_StructElement_GetLang(elem, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetStringAttribute(JNIEnv *env, jobject thiz,
                                                                       jlong structElemPtr,
                                                                       jstring attrName) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem || !attrName) return nullptr;
    const char *cAttrName = env->GetStringUTFChars(attrName, nullptr);
    unsigned long size = FPDF_StructElement_GetStringAttribute(elem, cAttrName, nullptr, 0);
    if (size == 0) { env->ReleaseStringUTFChars(attrName, cAttrName); return env->NewStringUTF(""); }
    unsigned short *buffer = new unsigned short[size];
    FPDF_StructElement_GetStringAttribute(elem, cAttrName, buffer, size);
    env->ReleaseStringUTFChars(attrName, cAttrName);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetMarkedContentID(JNIEnv *env, jobject thiz,
                                                                       jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return -1;
    return FPDF_StructElement_GetMarkedContentID(elem);
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetObjType(JNIEnv *env, jobject thiz,
                                                                jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return nullptr;
    unsigned long size = FPDF_StructElement_GetObjType(elem, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDF_StructElement_GetObjType(elem, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetTitle(JNIEnv *env, jobject thiz,
                                                              jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return nullptr;
    unsigned long size = FPDF_StructElement_GetTitle(elem, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDF_StructElement_GetTitle(elem, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetChildMarkedContentID(JNIEnv *env, jobject thiz,
                                                                            jlong structElemPtr,
                                                                            jint index) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return -1;
    return FPDF_StructElement_GetChildMarkedContentID(elem, index);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetParent(JNIEnv *env, jobject thiz,
                                                               jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return 0;
    return (jlong) FPDF_StructElement_GetParent(elem);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetAttributeCount(JNIEnv *env, jobject thiz,
                                                                      jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return -1;
    return FPDF_StructElement_GetAttributeCount(elem);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetAttributeAtIndex(JNIEnv *env, jobject thiz,
                                                                        jlong structElemPtr,
                                                                        jint index) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return 0;
    return (jlong) FPDF_StructElement_GetAttributeAtIndex(elem, index);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementAttrGetCount(JNIEnv *env, jobject thiz,
                                                                  jlong attrPtr) {
    FPDF_STRUCTELEMENT_ATTR attr = (FPDF_STRUCTELEMENT_ATTR) attrPtr;
    if (!attr) return -1;
    return FPDF_StructElement_Attr_GetCount(attr);
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementAttrGetName(JNIEnv *env, jobject thiz,
                                                                 jlong attrPtr, jint index) {
    FPDF_STRUCTELEMENT_ATTR attr = (FPDF_STRUCTELEMENT_ATTR) attrPtr;
    if (!attr) return nullptr;
    unsigned long outLen = 0;
    FPDF_StructElement_Attr_GetName(attr, index, nullptr, 0, &outLen);
    if (outLen == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[outLen];
    FPDF_StructElement_Attr_GetName(attr, index, buffer, outLen, &outLen);
    jstring result = env->NewString((jchar *) buffer, outLen / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementAttrGetValue(JNIEnv *env, jobject thiz,
                                                                  jlong attrPtr, jstring name) {
    FPDF_STRUCTELEMENT_ATTR attr = (FPDF_STRUCTELEMENT_ATTR) attrPtr;
    if (!attr || !name) return 0;
    const char *cName = env->GetStringUTFChars(name, nullptr);
    jlong result = (jlong) FPDF_StructElement_Attr_GetValue(attr, cName);
    env->ReleaseStringUTFChars(name, cName);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementAttrGetType(JNIEnv *env, jobject thiz,
                                                                 jlong valuePtr) {
    FPDF_STRUCTELEMENT_ATTR_VALUE value = (FPDF_STRUCTELEMENT_ATTR_VALUE) valuePtr;
    if (!value) return 0;
    return (jint) FPDF_StructElement_Attr_GetType(value);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementAttrGetBooleanValue(JNIEnv *env, jobject thiz,
                                                                        jlong valuePtr,
                                                                        jbooleanArray outValue) {
    FPDF_STRUCTELEMENT_ATTR_VALUE value = (FPDF_STRUCTELEMENT_ATTR_VALUE) valuePtr;
    if (!value || !outValue) return JNI_FALSE;
    FPDF_BOOL boolVal = static_cast<FPDF_BOOL>(0);
    FPDF_BOOL ok = FPDF_StructElement_Attr_GetBooleanValue(value, &boolVal);
    if (ok) {
        jboolean bVal = boolVal ? JNI_TRUE : JNI_FALSE;
        env->SetBooleanArrayRegion(outValue, 0, 1, &bVal);
    }
    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementAttrGetNumberValue(JNIEnv *env, jobject thiz,
                                                                       jlong valuePtr,
                                                                        jfloatArray outValue) {
    FPDF_STRUCTELEMENT_ATTR_VALUE value = (FPDF_STRUCTELEMENT_ATTR_VALUE) valuePtr;
    if (!value || !outValue) return JNI_FALSE;
    float numVal = 0.0f;
    FPDF_BOOL ok = FPDF_StructElement_Attr_GetNumberValue(value, &numVal);
    if (ok) {
        env->SetFloatArrayRegion(outValue, 0, 1, &numVal);
    }
    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementAttrGetStringValue(JNIEnv *env, jobject thiz,
                                                                       jlong valuePtr) {
    FPDF_STRUCTELEMENT_ATTR_VALUE value = (FPDF_STRUCTELEMENT_ATTR_VALUE) valuePtr;
    if (!value) return JNI_FALSE;
    unsigned long outLen = 0;
    FPDF_BOOL ok = FPDF_StructElement_Attr_GetStringValue(value, nullptr, 0, &outLen);
    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jbyteArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementAttrGetBlobValue(JNIEnv *env, jobject thiz,
                                                                      jlong valuePtr) {
    FPDF_STRUCTELEMENT_ATTR_VALUE value = (FPDF_STRUCTELEMENT_ATTR_VALUE) valuePtr;
    if (!value) return nullptr;
    unsigned long outLen = 0;
    FPDF_StructElement_Attr_GetBlobValue(value, nullptr, 0, &outLen);
    if (outLen == 0) return nullptr;
    unsigned char *buffer = new unsigned char[outLen];
    FPDF_StructElement_Attr_GetBlobValue(value, buffer, outLen, &outLen);
    jbyteArray result = env->NewByteArray(outLen);
    env->SetByteArrayRegion(result, 0, outLen, (jbyte *) buffer);
    delete[] buffer;
    return result;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementAttrCountChildren(JNIEnv *env, jobject thiz,
                                                                      jlong valuePtr) {
    FPDF_STRUCTELEMENT_ATTR_VALUE value = (FPDF_STRUCTELEMENT_ATTR_VALUE) valuePtr;
    if (!value) return -1;
    return FPDF_StructElement_Attr_CountChildren(value);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementAttrGetChildAtIndex(JNIEnv *env, jobject thiz,
                                                                        jlong valuePtr,
                                                                        jint index) {
    FPDF_STRUCTELEMENT_ATTR_VALUE value = (FPDF_STRUCTELEMENT_ATTR_VALUE) valuePtr;
    if (!value) return 0;
    return (jlong) FPDF_StructElement_Attr_GetChildAtIndex(value, index);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetMarkedContentIdCount(JNIEnv *env, jobject thiz,
                                                                            jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return -1;
    return FPDF_StructElement_GetMarkedContentIdCount(elem);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeStructElementGetMarkedContentIdAtIndex(JNIEnv *env, jobject thiz,
                                                                              jlong structElemPtr,
                                                                              jint index) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return -1;
    return FPDF_StructElement_GetMarkedContentIdAtIndex(elem, index);
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

// --- Data Availability (fpdf_dataavail.h) ---
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeIsLinearized(JNIEnv *env, jobject thiz, jlong availPtr) {
    FPDF_AVAIL avail = (FPDF_AVAIL) availPtr;
    if (!avail) return JNI_FALSE;
    return FPDFAvail_IsLinearized(avail) == 1 ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCreateAvail(JNIEnv *env, jobject thiz, jbyteArray fileData) {
    if (!fileData) return 0;
    jsize len = env->GetArrayLength(fileData);
    if (len == 0) return 0;
    jbyte* elements = env->GetByteArrayElements(fileData, nullptr);

    AvailData* data = new AvailData();
    data->fileLen = (unsigned long) len;
    data->fileData = new unsigned char[len];
    memcpy(data->fileData, elements, len);

    data->fileAvailStruct.version = 1;
    data->fileAvailStruct.IsDataAvail = IsDataAvailCallback;

    data->fileAccessStruct.m_FileLen = data->fileLen;
    data->fileAccessStruct.m_GetBlock = GetBlockCallback;
    data->fileAccessStruct.m_Param = data;

    env->ReleaseByteArrayElements(fileData, elements, JNI_ABORT);

    FPDF_AVAIL avail = FPDFAvail_Create(&data->fileAvailStruct, &data->fileAccessStruct);
    if (!avail) {
        delete[] data->fileData;
        delete data;
        return 0;
    }
    g_availDataMap[avail] = data;
    return (jlong) avail;
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeDestroyAvail(JNIEnv *env, jobject thiz, jlong availPtr) {
    FPDF_AVAIL avail = (FPDF_AVAIL) availPtr;
    if (!avail) return;
    auto it = g_availDataMap.find(avail);
    if (it != g_availDataMap.end()) {
        delete[] it->second->fileData;
        delete it->second;
        g_availDataMap.erase(it);
    }
    FPDFAvail_Destroy(avail);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAvailGetDocument(JNIEnv *env, jobject thiz,
                                                         jlong availPtr, jstring password) {
    FPDF_AVAIL avail = (FPDF_AVAIL) availPtr;
    if (!avail) return 0;
    const char* pwd = password ? env->GetStringUTFChars(password, nullptr) : nullptr;
    FPDF_DOCUMENT doc = FPDFAvail_GetDocument(avail, (FPDF_BYTESTRING) pwd);
    if (pwd) env->ReleaseStringUTFChars(password, pwd);
    return (jlong) doc;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAvailGetFirstPageNum(JNIEnv *env, jobject thiz,
                                                             jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return (jint) FPDFAvail_GetFirstPageNum(doc);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAvailIsDocAvail(JNIEnv *env, jobject thiz,
                                                        jlong availPtr) {
    FPDF_AVAIL avail = (FPDF_AVAIL) availPtr;
    if (!avail) return -1;
    return (jint) FPDFAvail_IsDocAvail(avail, nullptr);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAvailIsPageAvail(JNIEnv *env, jobject thiz,
                                                         jlong availPtr, jint pageIndex) {
    FPDF_AVAIL avail = (FPDF_AVAIL) availPtr;
    if (!avail) return -1;
    return (jint) FPDFAvail_IsPageAvail(avail, pageIndex, nullptr);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAvailIsFormAvail(JNIEnv *env, jobject thiz,
                                                         jlong availPtr) {
    FPDF_AVAIL avail = (FPDF_AVAIL) availPtr;
    if (!avail) return -1;
    return (jint) FPDFAvail_IsFormAvail(avail, nullptr);
}

// --- fpdf_sysfontinfo.h ---
JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetDefaultTTFMapCount(JNIEnv *env, jobject thiz) {
    return (jint) FPDF_GetDefaultTTFMapCount();
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetDefaultTTFMapEntry(JNIEnv *env, jobject thiz,
                                                               jint index, jintArray outCharset) {
    const FPDF_CharsetFontMap* entry = FPDF_GetDefaultTTFMapEntry((size_t) index);
    if (!entry || entry->charset == -1) return nullptr;
    if (outCharset) {
        jint charset = entry->charset;
        env->SetIntArrayRegion(outCharset, 0, 1, &charset);
    }
    return entry->fontname ? env->NewStringUTF(entry->fontname) : nullptr;
}

// --- fpdf_sysfontinfo.h remaining functions ---
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAddInstalledFont(JNIEnv *env, jobject thiz,
                                                         jlong mapperPtr, jstring face,
                                                         jint charset) {
    void* mapper = (void*) mapperPtr;
    const char* cFace = env->GetStringUTFChars(face, nullptr);
    FPDF_AddInstalledFont(mapper, cFace, charset);
    env->ReleaseStringUTFChars(face, cFace);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetDefaultSystemFontInfo(JNIEnv *env, jobject thiz) {
    return (jlong) FPDF_GetDefaultSystemFontInfo();
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFreeDefaultSystemFontInfo(JNIEnv *env, jobject thiz,
                                                                   jlong fontInfoPtr) {
    FPDF_FreeDefaultSystemFontInfo((FPDF_SYSFONTINFO*) fontInfoPtr);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetSystemFontInfo(JNIEnv *env, jobject thiz,
                                                           jlong fontInfoPtr) {
    FPDF_SetSystemFontInfo((FPDF_SYSFONTINFO*) fontInfoPtr);
}
// ---

/**
 * Get Link Handle from Annotation
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetLinkFromAnnot(JNIEnv *env, jobject thiz,
                                                         jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    
    FPDF_LINK link = FPDFAnnot_GetLink(annot);
    return (jlong) link;
}

/**
 * Get Catalog Language
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetCatalogLanguage(JNIEnv *env, jobject thiz,
                                                           jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return nullptr;
    
    unsigned long size = FPDFCatalog_GetLanguage(doc, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    
    unsigned short *buffer = new unsigned short[size];
    FPDFCatalog_GetLanguage(doc, buffer, size);
    
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

/**
 * Set Catalog Language
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetCatalogLanguage(JNIEnv *env, jobject thiz,
                                                           jlong docPtr, jstring language) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc || !language) return JNI_FALSE;
    
    const jchar *wideChars = env->GetStringChars(language, nullptr);
    jsize len = env->GetStringLength(language);
    
    FPDF_WCHAR *wideStr = new FPDF_WCHAR[len + 1];
    for (jsize i = 0; i < len; i++) {
        wideStr[i] = (FPDF_WCHAR) wideChars[i];
    }
    wideStr[len] = 0;
    
    env->ReleaseStringChars(language, wideChars);
    
    FPDF_BOOL result = FPDFCatalog_SetLanguage(doc, wideStr);
    delete[] wideStr;
    return result ? JNI_TRUE : JNI_FALSE;
}

/**
 * Check if document is a tagged PDF
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeCatalogIsTagged(JNIEnv *env, jobject thiz,
                                                        jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return JNI_FALSE;
    return FPDFCatalog_IsTagged(doc) ? JNI_TRUE : JNI_FALSE;
}

/**
 * Get Struct Element Expansion
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetStructElementExpansion(JNIEnv *env, jobject thiz,
                                                                  jlong structElemPtr) {
    FPDF_STRUCTELEMENT elem = (FPDF_STRUCTELEMENT) structElemPtr;
    if (!elem) return nullptr;
    
    unsigned long size = FPDF_StructElement_GetExpansion(elem, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    
    unsigned short *buffer = new unsigned short[size];
    FPDF_StructElement_GetExpansion(elem, buffer, size);
    
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

/**
 * Add Existing Mark to Page Object
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAddExistingMark(JNIEnv *env, jobject thiz,
                                                        jlong pageObjPtr, jlong markPtr) {
    FPDF_PAGEOBJECT pageObj = (FPDF_PAGEOBJECT) pageObjPtr;
    FPDF_PAGEOBJECTMARK mark = (FPDF_PAGEOBJECTMARK) markPtr;
    if (!pageObj || !mark) return JNI_FALSE;
    
    return FPDFPageObj_AddExistingMark(pageObj, mark) ? JNI_TRUE : JNI_FALSE;
}

/**
 * Set Font Size on Text Object
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetTextObjFontSize(JNIEnv *env, jobject thiz,
                                                           jlong textObjPtr, jfloat size) {
    FPDF_PAGEOBJECT text = (FPDF_PAGEOBJECT) textObjPtr;
    if (!text) return JNI_FALSE;
    
    return FPDFTextObj_SetFontSize(text, (float) size) ? JNI_TRUE : JNI_FALSE;
}

/**
 * Set Positions on Text Object
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeSetTextPositions(JNIEnv *env, jobject thiz,
                                                         jlong textObjPtr,
                                                         jfloatArray positions) {
    FPDF_PAGEOBJECT text = (FPDF_PAGEOBJECT) textObjPtr;
    if (!text || !positions) return JNI_FALSE;
    
    jsize count = env->GetArrayLength(positions);
    if (count == 0) return JNI_FALSE;
    
    jfloat *pos = env->GetFloatArrayElements(positions, nullptr);
    FPDF_BOOL result = FPDFText_SetPositions(text, (const float *) pos, (size_t) count);
    env->ReleaseFloatArrayElements(positions, pos, JNI_ABORT);
    
    return result ? JNI_TRUE : JNI_FALSE;
}

// =========================================================================
// NEW BINDINGS
// =========================================================================

/**
 * Get Document Permissions
 */
JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetDocPermissions(JNIEnv *env, jobject thiz,
                                                          jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return (jlong) FPDF_GetDocPermissions(doc);
}

/**
 * Get Annotation String Value
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetAnnotStringValue(JNIEnv *env, jobject thiz,
                                                            jlong annotPtr, jstring key) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot || !key) return nullptr;

    const char *keyChars = env->GetStringUTFChars(key, nullptr);
    unsigned long size = FPDFAnnot_GetStringValue(annot, keyChars, nullptr, 0);
    env->ReleaseStringUTFChars(key, keyChars);

    if (size == 0) return env->NewStringUTF("");

    unsigned short *buffer = new unsigned short[size];
    keyChars = env->GetStringUTFChars(key, nullptr);
    FPDFAnnot_GetStringValue(annot, keyChars, buffer, size);
    env->ReleaseStringUTFChars(key, keyChars);

    jstring result = env->NewString((jchar *) buffer, size - 1);
    delete[] buffer;
    return result;
}

/**
 * Get Bounded Text - extract text from a region
 */
JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetBoundedText(JNIEnv *env, jobject thiz,
                                                       jlong textPagePtr,
                                                       jdouble left, jdouble top,
                                                       jdouble right, jdouble bottom) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return nullptr;

    int count = FPDFText_GetBoundedText(textPage, left, top, right, bottom, nullptr, 0);
    if (count == 0) return env->NewStringUTF("");

    unsigned short *buffer = new unsigned short[count];
    FPDFText_GetBoundedText(textPage, left, top, right, bottom, buffer, count);

    jstring result = env->NewString((jchar *) buffer, count);
    delete[] buffer;
    return result;
}

/**
 * Form Force to Kill Focus
 */
JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeFormForceToKillFocus(JNIEnv *env, jobject thiz,
                                                             jlong formPtr) {
    FPDF_FORMHANDLE form = (FPDF_FORMHANDLE) formPtr;
    if (!form) return;
    FORM_ForceToKillFocus(form);
}

/**
 * Get Page Bounding Box
 */
JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeGetPageBoundingBox(JNIEnv *env, jobject thiz,
                                                           jlong pagePtr, jfloatArray result) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!page || !result) return JNI_FALSE;

    FS_RECTF rect;
    FPDF_BOOL success = FPDF_GetPageBoundingBox(page, &rect);
    if (!success) return JNI_FALSE;

    jfloat *out = env->GetFloatArrayElements(result, nullptr);
    out[0] = rect.left;
    out[1] = rect.top;
    out[2] = rect.right;
    out[3] = rect.bottom;
    env->ReleaseFloatArrayElements(result, out, 0);

    return JNI_TRUE;
}

// --- fpdf_text.h Additional Bindings ---

JNIEXPORT jfloat JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetCharAngle(JNIEnv *env, jobject thiz,
                                                        jlong textPagePtr, jint index) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return 0;
    return (jfloat) FPDFText_GetCharAngle(textPage, index);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetCharOrigin(JNIEnv *env, jobject thiz,
                                                         jlong textPagePtr, jint index,
                                                         jdoubleArray origin) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return JNI_FALSE;
    double x = 0, y = 0;
    if (!FPDFText_GetCharOrigin(textPage, index, &x, &y)) return JNI_FALSE;
    jdouble *body = env->GetDoubleArrayElements(origin, nullptr);
    body[0] = x; body[1] = y;
    env->ReleaseDoubleArrayElements(origin, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetFillColor(JNIEnv *env, jobject thiz,
                                                        jlong textPagePtr, jint index,
                                                        jintArray color) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return JNI_FALSE;
    unsigned int r = 0, g = 0, b = 0, a = 0;
    if (!FPDFText_GetFillColor(textPage, index, &r, &g, &b, &a)) return JNI_FALSE;
    jint *body = env->GetIntArrayElements(color, nullptr);
    body[0] = r; body[1] = g; body[2] = b; body[3] = a;
    env->ReleaseIntArrayElements(color, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetFontInfo(JNIEnv *env, jobject thiz,
                                                       jlong textPagePtr, jint index) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return nullptr;
    int flags = 0;
    unsigned long size = FPDFText_GetFontInfo(textPage, index, nullptr, 0, &flags);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDFText_GetFontInfo(textPage, index, buffer, size, &flags);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jdouble JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetFontSize(JNIEnv *env, jobject thiz,
                                                       jlong textPagePtr, jint index) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return 0;
    return FPDFText_GetFontSize(textPage, index);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetFontWeight(JNIEnv *env, jobject thiz,
                                                         jlong textPagePtr, jint index) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return 0;
    return FPDFText_GetFontWeight(textPage, index);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetLooseCharBox(JNIEnv *env, jobject thiz,
                                                           jlong textPagePtr, jint index,
                                                           jdoubleArray rect) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return JNI_FALSE;
    FS_RECTF fRect;
    if (!FPDFText_GetLooseCharBox(textPage, index, &fRect)) return JNI_FALSE;
    jdouble *body = env->GetDoubleArrayElements(rect, nullptr);
    body[0] = fRect.left; body[1] = fRect.top;
    body[2] = fRect.right; body[3] = fRect.bottom;
    env->ReleaseDoubleArrayElements(rect, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetMatrix(JNIEnv *env, jobject thiz,
                                                     jlong textPagePtr, jint index,
                                                     jfloatArray matrix) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return JNI_FALSE;
    FS_MATRIX fsMatrix;
    if (!FPDFText_GetMatrix(textPage, index, &fsMatrix)) return JNI_FALSE;
    jfloat *body = env->GetFloatArrayElements(matrix, nullptr);
    body[0] = fsMatrix.a; body[1] = fsMatrix.b;
    body[2] = fsMatrix.c; body[3] = fsMatrix.d;
    body[4] = fsMatrix.e; body[5] = fsMatrix.f;
    env->ReleaseFloatArrayElements(matrix, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetStrokeColor(JNIEnv *env, jobject thiz,
                                                          jlong textPagePtr, jint index,
                                                          jintArray color) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return JNI_FALSE;
    unsigned int r = 0, g = 0, b = 0, a = 0;
    if (!FPDFText_GetStrokeColor(textPage, index, &r, &g, &b, &a)) return JNI_FALSE;
    jint *body = env->GetIntArrayElements(color, nullptr);
    body[0] = r; body[1] = g; body[2] = b; body[3] = a;
    env->ReleaseIntArrayElements(color, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetTextObject(JNIEnv *env, jobject thiz,
                                                         jlong textPagePtr, jint index) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return 0;
    return (jlong) FPDFText_GetTextObject(textPage, index);
}

JNIEXPORT jshort JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextGetUnicode(JNIEnv *env, jobject thiz,
                                                      jlong textPagePtr, jint index) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return 0;
    return (jshort) FPDFText_GetUnicode(textPage, index);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextHasUnicodeMapError(JNIEnv *env, jobject thiz,
                                                              jlong textPagePtr, jint index) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return JNI_FALSE;
    return FPDFText_HasUnicodeMapError(textPage, index) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextIsGenerated(JNIEnv *env, jobject thiz,
                                                       jlong textPagePtr, jint index) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return JNI_FALSE;
    return FPDFText_IsGenerated(textPage, index) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextIsHyphen(JNIEnv *env, jobject thiz,
                                                    jlong textPagePtr, jint index) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    if (!textPage) return JNI_FALSE;
    return FPDFText_IsHyphen(textPage, index) ? JNI_TRUE : JNI_FALSE;
}

// --- fpdf_annot.h Additional Bindings ---

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageGetAnnotIndex(JNIEnv *env, jobject thiz,
                                                         jlong pagePtr, jlong annotPtr) {
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!page || !annot) return -1;
    return FPDFPage_GetAnnotIndex(page, annot);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotHasKey(JNIEnv *env, jobject thiz,
                                                   jlong annotPtr, jstring key) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    const char* cKey = env->GetStringUTFChars(key, nullptr);
    FPDF_BOOL result = FPDFAnnot_HasKey(annot, cKey);
    env->ReleaseStringUTFChars(key, cKey);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetValueType(JNIEnv *env, jobject thiz,
                                                         jlong annotPtr, jstring key) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    const char* cKey = env->GetStringUTFChars(key, nullptr);
    int result = FPDFAnnot_GetValueType(annot, cKey);
    env->ReleaseStringUTFChars(key, cKey);
    return result;
}

JNIEXPORT jdouble JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetNumberValue(JNIEnv *env, jobject thiz,
                                                           jlong annotPtr, jstring key) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    const char* cKey = env->GetStringUTFChars(key, nullptr);
    float result = 0.0f;
    FPDF_BOOL ok = FPDFAnnot_GetNumberValue(annot, cKey, &result);
    env->ReleaseStringUTFChars(key, cKey);
    return ok ? (jdouble)result : 0.0;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotSetURI(JNIEnv *env, jobject thiz,
                                                    jlong annotPtr, jstring uri) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    const char* cUri = env->GetStringUTFChars(uri, nullptr);
    FPDF_BOOL result = FPDFAnnot_SetURI(annot, cUri);
    env->ReleaseStringUTFChars(uri, cUri);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetBorder(JNIEnv *env, jobject thiz,
                                                       jlong annotPtr, jfloatArray border) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    float horizontal = 0, vertical = 0, corner = 0;
    if (!FPDFAnnot_GetBorder(annot, &horizontal, &vertical, &corner)) return JNI_FALSE;
    jfloat *body = env->GetFloatArrayElements(border, nullptr);
    body[0] = horizontal; body[1] = vertical; body[2] = corner;
    env->ReleaseFloatArrayElements(border, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotSetBorder(JNIEnv *env, jobject thiz,
                                                       jlong annotPtr, jfloat horizontal, jfloat vertical, jfloat corner) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    return FPDFAnnot_SetBorder(annot, horizontal, vertical, corner) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFontColor(JNIEnv *env, jobject thiz,
                                                          jlong hFormPtr, jlong annotPtr, jintArray color) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    unsigned int r = 0, g = 0, b = 0;
    if (!FPDFAnnot_GetFontColor(hForm, annot, &r, &g, &b)) return JNI_FALSE;
    jint *body = env->GetIntArrayElements(color, nullptr);
    body[0] = (jint)r; body[1] = (jint)g; body[2] = (jint)b;
    env->ReleaseIntArrayElements(color, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotSetFontColor(JNIEnv *env, jobject thiz,
                                                          jlong hFormPtr, jlong annotPtr, jint r, jint g, jint b) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    return FPDFAnnot_SetFontColor(hForm, annot, (unsigned int)r, (unsigned int)g, (unsigned int)b) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jdouble JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFontSize(JNIEnv *env, jobject thiz,
                                                         jlong hFormPtr, jlong annotPtr) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    float value = 0;
    FPDFAnnot_GetFontSize(hForm, annot, &value);
    return (jdouble)value;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFormFieldType(JNIEnv *env, jobject thiz,
                                                              jlong hFormPtr, jlong annotPtr) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return FPDFAnnot_GetFormFieldType(hForm, annot);
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFormFieldName(JNIEnv *env, jobject thiz,
                                                              jlong hFormPtr, jlong annotPtr) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return nullptr;
    unsigned long size = FPDFAnnot_GetFormFieldName(hForm, annot, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDFAnnot_GetFormFieldName(hForm, annot, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFormFieldValue(JNIEnv *env, jobject thiz,
                                                               jlong hFormPtr, jlong annotPtr) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return nullptr;
    unsigned long size = FPDFAnnot_GetFormFieldValue(hForm, annot, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDFAnnot_GetFormFieldValue(hForm, annot, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFormControlCount(JNIEnv *env, jobject thiz,
                                                                 jlong hFormPtr, jlong annotPtr) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return FPDFAnnot_GetFormControlCount(hForm, annot);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFormControlIndex(JNIEnv *env, jobject thiz,
                                                                 jlong hFormPtr, jlong annotPtr) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return FPDFAnnot_GetFormControlIndex(hForm, annot);
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFormFieldAlternateName(JNIEnv *env, jobject thiz,
                                                                       jlong hFormPtr, jlong annotPtr) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return nullptr;
    unsigned long size = FPDFAnnot_GetFormFieldAlternateName(hForm, annot, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDFAnnot_GetFormFieldAlternateName(hForm, annot, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetOptionCount(JNIEnv *env, jobject thiz,
                                                            jlong hFormPtr, jlong annotPtr) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return FPDFAnnot_GetOptionCount(hForm, annot);
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetOptionLabel(JNIEnv *env, jobject thiz,
                                                            jlong hFormPtr, jlong annotPtr, jint index) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return nullptr;
    unsigned long size = FPDFAnnot_GetOptionLabel(hForm, annot, index, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDFAnnot_GetOptionLabel(hForm, annot, index, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotIsOptionSelected(JNIEnv *env, jobject thiz,
                                                              jlong hFormPtr, jlong annotPtr, jint index) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    return FPDFAnnot_IsOptionSelected(hForm, annot, index) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotIsChecked(JNIEnv *env, jobject thiz,
                                                        jlong hFormPtr, jlong annotPtr) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    return FPDFAnnot_IsChecked(hForm, annot) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFocusableSubtypesCount(JNIEnv *env, jobject thiz,
                                                                       jlong hFormPtr) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    if (!hForm) return 0;
    return FPDFAnnot_GetFocusableSubtypesCount(hForm);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFocusableSubtypes(JNIEnv *env, jobject thiz,
                                                                  jlong hFormPtr, jintArray subtypes) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    if (!hForm) return JNI_FALSE;
    int count = FPDFAnnot_GetFocusableSubtypesCount(hForm);
    if (count == 0) return JNI_FALSE;
    int *buffer = new int[count];
    if (!FPDFAnnot_GetFocusableSubtypes(hForm, buffer, count)) {
        delete[] buffer;
        return JNI_FALSE;
    }
    if (subtypes && env->GetArrayLength(subtypes) >= count) {
        env->SetIntArrayRegion(subtypes, 0, count, buffer);
    }
    delete[] buffer;
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotSetFocusableSubtypes(JNIEnv *env, jobject thiz,
                                                                  jlong hFormPtr, jintArray subtypes) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    if (!hForm) return JNI_FALSE;
    jint *body = env->GetIntArrayElements(subtypes, nullptr);
    int count = env->GetArrayLength(subtypes);
    FPDF_BOOL result = FPDFAnnot_SetFocusableSubtypes(hForm, (FPDF_ANNOTATION_SUBTYPE*)body, count);
    env->ReleaseIntArrayElements(subtypes, body, 0);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetLinkedAnnot(JNIEnv *env, jobject thiz,
                                                            jlong annotPtr, jstring key) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    const char* cKey = env->GetStringUTFChars(key, nullptr);
    jlong result = (jlong) FPDFAnnot_GetLinkedAnnot(annot, cKey);
    env->ReleaseStringUTFChars(key, cKey);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetLine(JNIEnv *env, jobject thiz,
                                                     jlong annotPtr, jdoubleArray line) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    FS_POINTF start, end;
    if (!FPDFAnnot_GetLine(annot, &start, &end)) return JNI_FALSE;
    jdouble *body = env->GetDoubleArrayElements(line, nullptr);
    body[0] = start.x; body[1] = start.y;
    body[2] = end.x; body[3] = end.y;
    env->ReleaseDoubleArrayElements(line, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetVerticesCount(JNIEnv *env, jobject thiz,
                                                              jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return (jint) FPDFAnnot_GetVertices(annot, nullptr, 0);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetVertices(JNIEnv *env, jobject thiz,
                                                        jlong annotPtr, jfloatArray vertices) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    int count = FPDFAnnot_GetVertices(annot, nullptr, 0);
    if (count == 0) return JNI_FALSE;
    FS_POINTF *points = new FS_POINTF[count];
    if (!FPDFAnnot_GetVertices(annot, points, count)) {
        delete[] points;
        return JNI_FALSE;
    }
    jfloat *body = env->GetFloatArrayElements(vertices, nullptr);
    for (int i = 0; i < count; i++) {
        body[i*2] = points[i].x; body[i*2+1] = points[i].y;
    }
    env->ReleaseFloatArrayElements(vertices, body, 0);
    delete[] points;
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetInkListCount(JNIEnv *env, jobject thiz,
                                                            jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return FPDFAnnot_GetInkListCount(annot);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetInkListPath(JNIEnv *env, jobject thiz,
                                                           jlong annotPtr, jint index,
                                                           jfloatArray points) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    int count = FPDFAnnot_GetInkListPath(annot, index, nullptr, 0);
    if (count == 0) return JNI_FALSE;
    FS_POINTF *inkPoints = new FS_POINTF[count];
    if (!FPDFAnnot_GetInkListPath(annot, index, inkPoints, count)) {
        delete[] inkPoints;
        return JNI_FALSE;
    }
    jfloat *body = env->GetFloatArrayElements(points, nullptr);
    for (int i = 0; i < count; i++) {
        body[i*2] = inkPoints[i].x; body[i*2+1] = inkPoints[i].y;
    }
    env->ReleaseFloatArrayElements(points, body, 0);
    delete[] inkPoints;
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotRemoveInkList(JNIEnv *env, jobject thiz,
                                                          jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    return FPDFAnnot_RemoveInkList(annot) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotAddInkStroke(JNIEnv *env, jobject thiz,
                                                         jlong annotPtr, jfloatArray points) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    jfloat *body = env->GetFloatArrayElements(points, nullptr);
    int count = env->GetArrayLength(points) / 2;
    FPDF_BOOL result = FPDFAnnot_AddInkStroke(annot, (const FS_POINTF *) body, count);
    env->ReleaseFloatArrayElements(points, body, 0);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotHasAttachmentPoints(JNIEnv *env, jobject thiz,
                                                                jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    return FPDFAnnot_HasAttachmentPoints(annot) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotSetAttachmentPoints(JNIEnv *env, jobject thiz,
                                                                  jlong annotPtr, jfloatArray quadPoints) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    jfloat *body = env->GetFloatArrayElements(quadPoints, nullptr);
    int count = env->GetArrayLength(quadPoints) / 8;
    FPDF_BOOL result = JNI_TRUE;
    FS_QUADPOINTSF pts;
    for (int i = 0; i < count; i++) {
        pts.x1 = body[i*8]; pts.y1 = body[i*8+1];
        pts.x2 = body[i*8+2]; pts.y2 = body[i*8+3];
        pts.x3 = body[i*8+4]; pts.y3 = body[i*8+5];
        pts.x4 = body[i*8+6]; pts.y4 = body[i*8+7];
        if (!FPDFAnnot_SetAttachmentPoints(annot, i, &pts)) {
            result = JNI_FALSE;
            break;
        }
    }
    env->ReleaseFloatArrayElements(quadPoints, body, 0);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotAppendAttachmentPoints(JNIEnv *env, jobject thiz,
                                                                    jlong annotPtr, jfloatArray quadPoints) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    jfloat *body = env->GetFloatArrayElements(quadPoints, nullptr);
    int count = env->GetArrayLength(quadPoints) / 8;
    FPDF_BOOL result = JNI_TRUE;
    FS_QUADPOINTSF pts;
    for (int i = 0; i < count; i++) {
        pts.x1 = body[i*8]; pts.y1 = body[i*8+1];
        pts.x2 = body[i*8+2]; pts.y2 = body[i*8+3];
        pts.x3 = body[i*8+4]; pts.y3 = body[i*8+5];
        pts.x4 = body[i*8+6]; pts.y4 = body[i*8+7];
        if (!FPDFAnnot_AppendAttachmentPoints(annot, &pts)) {
            result = JNI_FALSE;
            break;
        }
    }
    env->ReleaseFloatArrayElements(quadPoints, body, 0);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotCountAttachmentPoints(JNIEnv *env, jobject thiz,
                                                                  jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return FPDFAnnot_CountAttachmentPoints(annot);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetObjectCount(JNIEnv *env, jobject thiz,
                                                           jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return FPDFAnnot_GetObjectCount(annot);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetObject(JNIEnv *env, jobject thiz,
                                                      jlong annotPtr, jint index) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return (jlong) FPDFAnnot_GetObject(annot, index);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotAppendObject(JNIEnv *env, jobject thiz,
                                                         jlong annotPtr, jlong objPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) objPtr;
    if (!annot || !obj) return JNI_FALSE;
    return FPDFAnnot_AppendObject(annot, obj) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotRemoveObject(JNIEnv *env, jobject thiz,
                                                         jlong annotPtr, jint index) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    return FPDFAnnot_RemoveObject(annot, index) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotUpdateObject(JNIEnv *env, jobject thiz,
                                                         jlong annotPtr, jlong objPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) objPtr;
    if (!annot || !obj) return JNI_FALSE;
    return FPDFAnnot_UpdateObject(annot, obj) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetAP(JNIEnv *env, jobject thiz,
                                                  jlong annotPtr, jint mode) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return FPDFAnnot_GetAP(annot, mode, nullptr, 0);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotSetAP(JNIEnv *env, jobject thiz,
                                                   jlong annotPtr, jint mode, jstring value) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    FPDF_BOOL result = JNI_FALSE;
    jboolean isCopy;
    const jchar* wideValue = env->GetStringChars(value, &isCopy);
    result = FPDFAnnot_SetAP(annot, (FPDF_ANNOT_APPEARANCEMODE)mode, (FPDF_WIDESTRING)wideValue);
    env->ReleaseStringChars(value, wideValue);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFileAttachment(JNIEnv *env, jobject thiz,
                                                              jlong annotPtr) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return (jlong) FPDFAnnot_GetFileAttachment(annot);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotAddFileAttachment(JNIEnv *env, jobject thiz,
                                                               jlong annotPtr, jstring name) {
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    jboolean isCopy;
    const jchar* wideName = env->GetStringChars(name, &isCopy);
    FPDF_ATTACHMENT result = FPDFAnnot_AddFileAttachment(annot, (FPDF_WIDESTRING)wideName);
    env->ReleaseStringChars(name, wideName);
    return (jlong) result;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFormFieldAtPoint(JNIEnv *env, jobject thiz,
                                                                 jlong hFormPtr, jlong pagePtr,
                                                                 jdouble x, jdouble y) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!hForm || !page) return 0;
    FS_POINTF point = {(float)x, (float)y};
    return (jlong) FPDFAnnot_GetFormFieldAtPoint(hForm, page, &point);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFormFieldFlags(JNIEnv *env, jobject thiz,
                                                               jlong hFormPtr, jlong annotPtr) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return 0;
    return FPDFAnnot_GetFormFieldFlags(hForm, annot);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotSetFormFieldFlags(JNIEnv *env, jobject thiz,
                                                               jlong hFormPtr, jlong annotPtr, jint flags) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return JNI_FALSE;
    return FPDFAnnot_SetFormFieldFlags(hForm, annot, flags) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotGetFormAdditionalActionJavaScript(JNIEnv *env, jobject thiz,
                                                                               jlong hFormPtr, jlong annotPtr, jint eventType) {
    FPDF_FORMHANDLE hForm = (FPDF_FORMHANDLE) hFormPtr;
    FPDF_ANNOTATION annot = (FPDF_ANNOTATION) annotPtr;
    if (!annot) return nullptr;
    unsigned long size = FPDFAnnot_GetFormAdditionalActionJavaScript(hForm, annot, eventType, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDFAnnot_GetFormAdditionalActionJavaScript(hForm, annot, eventType, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotIsSupportedSubtype(JNIEnv *env, jobject thiz,
                                                               jint subtype) {
    return FPDFAnnot_IsSupportedSubtype(subtype) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeAnnotIsObjectSupportedSubtype(JNIEnv *env, jobject thiz,
                                                                     jint subtype) {
    return FPDFAnnot_IsObjectSupportedSubtype(subtype) ? JNI_TRUE : JNI_FALSE;
}

// --- fpdf_edit.h Additional Bindings ---

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjCreateNew(JNIEnv *env, jobject thiz,
                                                        jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return (jlong) FPDFPageObj_CreateNewPath(0.0f, 0.0f);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjCreateNewRect(JNIEnv *env, jobject thiz,
                                                           jlong docPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    return (jlong) FPDFPageObj_CreateNewRect(0.0f, 0.0f, 0.0f, 0.0f);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjCreateTextObj(JNIEnv *env, jobject thiz,
                                                           jlong docPtr, jstring fontName) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    if (!doc) return 0;
    const char* cName = env->GetStringUTFChars(fontName, nullptr);
    FPDF_FONT font = FPDFText_LoadStandardFont(doc, cName);
    env->ReleaseStringUTFChars(fontName, cName);
    if (!font) return 0;
    return (jlong) FPDFPageObj_CreateTextObj(doc, font, 12.0f);
}

JNIEXPORT void JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjDestroy(JNIEnv *env, jobject thiz,
                                                      jlong pageObjPtr) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (obj) FPDFPageObj_Destroy(obj);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjHasTransparency(JNIEnv *env, jobject thiz,
                                                             jlong pageObjPtr) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    return FPDFPageObj_HasTransparency(obj) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjGetMatrix(JNIEnv *env, jobject thiz,
                                                        jlong pageObjPtr, jfloatArray matrix) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    FS_MATRIX fsMatrix;
    if (!FPDFPageObj_GetMatrix(obj, &fsMatrix)) return JNI_FALSE;
    jfloat *body = env->GetFloatArrayElements(matrix, nullptr);
    body[0] = fsMatrix.a; body[1] = fsMatrix.b;
    body[2] = fsMatrix.c; body[3] = fsMatrix.d;
    body[4] = fsMatrix.e; body[5] = fsMatrix.f;
    env->ReleaseFloatArrayElements(matrix, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjSetMatrix(JNIEnv *env, jobject thiz,
                                                        jlong pageObjPtr, jfloatArray matrix) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    jfloat *body = env->GetFloatArrayElements(matrix, nullptr);
    FS_MATRIX fsMatrix = {body[0], body[1], body[2], body[3], body[4], body[5]};
    FPDF_BOOL result = FPDFPageObj_SetMatrix(obj, &fsMatrix);
    env->ReleaseFloatArrayElements(matrix, body, 0);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjTransformF(JNIEnv *env, jobject thiz,
                                                         jlong pageObjPtr, jfloat a, jfloat b, jfloat c, jfloat d, jfloat e, jfloat f) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    FS_MATRIX matrix = {a, b, c, d, e, f};
    return FPDFPageObj_TransformF(obj, &matrix) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjGetRotatedBounds(JNIEnv *env, jobject thiz,
                                                               jlong pageObjPtr, jfloatArray rect) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    FS_QUADPOINTSF quad;
    if (!FPDFPageObj_GetRotatedBounds(obj, &quad)) return JNI_FALSE;
    jfloat *body = env->GetFloatArrayElements(rect, nullptr);
    body[0] = quad.x1; body[1] = quad.y1;
    body[2] = quad.x2; body[3] = quad.y2;
    body[4] = quad.x3; body[5] = quad.y3;
    body[6] = quad.x4; body[7] = quad.y4;
    env->ReleaseFloatArrayElements(rect, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjGetLineCap(JNIEnv *env, jobject thiz,
                                                         jlong pageObjPtr) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return 0;
    return FPDFPageObj_GetLineCap(obj);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjSetLineCap(JNIEnv *env, jobject thiz,
                                                         jlong pageObjPtr, jint lineCap) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    return FPDFPageObj_SetLineCap(obj, lineCap) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjGetLineJoin(JNIEnv *env, jobject thiz,
                                                          jlong pageObjPtr) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return 0;
    return FPDFPageObj_GetLineJoin(obj);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjSetLineJoin(JNIEnv *env, jobject thiz,
                                                          jlong pageObjPtr, jint lineJoin) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    return FPDFPageObj_SetLineJoin(obj, lineJoin) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jfloat JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjGetStrokeWidth(JNIEnv *env, jobject thiz,
                                                             jlong pageObjPtr) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return 0;
    float strokeWidth = 0;
    return FPDFPageObj_GetStrokeWidth(obj, &strokeWidth) ? (jdouble) strokeWidth : 0;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjGetDashPhase(JNIEnv *env, jobject thiz,
                                                           jlong pageObjPtr, jfloatArray phase) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    float dashPhase = 0;
    if (!FPDFPageObj_GetDashPhase(obj, &dashPhase)) return JNI_FALSE;
    jfloat *body = env->GetFloatArrayElements(phase, nullptr);
    body[0] = dashPhase;
    env->ReleaseFloatArrayElements(phase, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjSetDashPhase(JNIEnv *env, jobject thiz,
                                                           jlong pageObjPtr, jfloat phase) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    return FPDFPageObj_SetDashPhase(obj, phase) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjGetDashCount(JNIEnv *env, jobject thiz,
                                                           jlong pageObjPtr) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return 0;
    return FPDFPageObj_GetDashCount(obj);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjGetDashArray(JNIEnv *env, jobject thiz,
                                                           jlong pageObjPtr, jfloatArray dashes) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    int count = FPDFPageObj_GetDashCount(obj);
    if (count == 0) return JNI_FALSE;
    float *buffer = new float[count];
    if (!FPDFPageObj_GetDashArray(obj, buffer, count)) {
        delete[] buffer;
        return JNI_FALSE;
    }
    env->SetFloatArrayRegion(dashes, 0, count, buffer);
    delete[] buffer;
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjSetDashArray(JNIEnv *env, jobject thiz,
                                                           jlong pageObjPtr, jfloatArray dashes) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    jfloat *body = env->GetFloatArrayElements(dashes, nullptr);
    int count = env->GetArrayLength(dashes);
    FPDF_BOOL result = FPDFPageObj_SetDashArray(obj, body, count, 0);
    env->ReleaseFloatArrayElements(dashes, body, 0);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjGetIsActive(JNIEnv *env, jobject thiz,
                                                          jlong pageObjPtr) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    FPDF_BOOL active = 0;
    return FPDFPageObj_GetIsActive(obj, &active) ? (active ? JNI_TRUE : JNI_FALSE) : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjSetIsActive(JNIEnv *env, jobject thiz,
                                                          jlong pageObjPtr, jboolean active) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    return FPDFPageObj_SetIsActive(obj, active ? 1 : 0) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjCountMarks(JNIEnv *env, jobject thiz,
                                                         jlong pageObjPtr) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return 0;
    return FPDFPageObj_CountMarks(obj);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjGetMark(JNIEnv *env, jobject thiz,
                                                      jlong pageObjPtr, jint index) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return 0;
    return (jlong) FPDFPageObj_GetMark(obj, index);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjAddMark(JNIEnv *env, jobject thiz,
                                                      jlong pageObjPtr, jstring name) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return 0;
    const char* cName = env->GetStringUTFChars(name, nullptr);
    FPDF_PAGEOBJECTMARK mark = FPDFPageObj_AddMark(obj, cName);
    env->ReleaseStringUTFChars(name, cName);
    return (jlong) mark;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjRemoveMark(JNIEnv *env, jobject thiz,
                                                         jlong pageObjPtr, jlong markPtr) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    FPDF_PAGEOBJECTMARK mark = (FPDF_PAGEOBJECTMARK) markPtr;
    if (!obj || !mark) return JNI_FALSE;
    return FPDFPageObj_RemoveMark(obj, mark) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjGetMarkedContentID(JNIEnv *env, jobject thiz,
                                                                 jlong pageObjPtr) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return -1;
    return FPDFPageObj_GetMarkedContentID(obj);
}

static const char* kBlendModeNames[] = {
    "Normal", "Multiply", "Screen", "Overlay", "Darken", "Lighten",
    "ColorDodge", "ColorBurn", "HardLight", "SoftLight", "Difference",
    "Exclusion", "Hue", "Saturation", "Color", "Luminosity"
};

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePageObjSetBlendMode(JNIEnv *env, jobject thiz,
                                                           jlong pageObjPtr, jint blendMode) {
    FPDF_PAGEOBJECT obj = (FPDF_PAGEOBJECT) pageObjPtr;
    if (!obj) return JNI_FALSE;
    if (blendMode < 0 || blendMode >= 16) return JNI_FALSE;
    FPDFPageObj_SetBlendMode(obj, kBlendModeNames[blendMode]);
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePathCountSegments(JNIEnv *env, jobject thiz,
                                                         jlong pathObjPtr) {
    FPDF_PAGEOBJECT pathObj = (FPDF_PAGEOBJECT) pathObjPtr;
    if (!pathObj) return 0;
    return FPDFPath_CountSegments(pathObj);
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePathGetPathSegment(JNIEnv *env, jobject thiz,
                                                          jlong pathObjPtr, jint index) {
    FPDF_PAGEOBJECT pathObj = (FPDF_PAGEOBJECT) pathObjPtr;
    if (!pathObj) return 0;
    return (jlong) FPDFPath_GetPathSegment(pathObj, index);
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativePathGetDrawMode(JNIEnv *env, jobject thiz,
                                                       jlong pathObjPtr) {
    FPDF_PAGEOBJECT pathObj = (FPDF_PAGEOBJECT) pathObjPtr;
    if (!pathObj) return 0;
    int fillmode = 0;
    FPDF_BOOL stroke = 0;
    if (!FPDFPath_GetDrawMode(pathObj, &fillmode, &stroke)) return 0;
    return fillmode;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextObjGetFont(JNIEnv *env, jobject thiz,
                                                      jlong textObjPtr) {
    FPDF_PAGEOBJECT textObj = (FPDF_PAGEOBJECT) textObjPtr;
    if (!textObj) return 0;
    return (jlong) FPDFTextObj_GetFont(textObj);
}

JNIEXPORT jdouble JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextObjGetFontSize(JNIEnv *env, jobject thiz,
                                                          jlong textObjPtr) {
    FPDF_PAGEOBJECT textObj = (FPDF_PAGEOBJECT) textObjPtr;
    if (!textObj) return 0;
    float fontSize = 0;
    return FPDFTextObj_GetFontSize(textObj, &fontSize) ? (jdouble) fontSize : 0;
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextObjGetText(JNIEnv *env, jobject thiz,
                                                      jlong textPagePtr, jlong textObjPtr) {
    FPDF_TEXTPAGE textPage = (FPDF_TEXTPAGE) textPagePtr;
    FPDF_PAGEOBJECT textObj = (FPDF_PAGEOBJECT) textObjPtr;
    if (!textPage || !textObj) return nullptr;
    unsigned long size = FPDFTextObj_GetText(textObj, textPage, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    unsigned short *buffer = new unsigned short[size];
    FPDFTextObj_GetText(textObj, textPage, buffer, size);
    jstring result = env->NewString((jchar *) buffer, size / 2 - 1);
    delete[] buffer;
    return result;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextObjGetTextRenderMode(JNIEnv *env, jobject thiz,
                                                                jlong textObjPtr) {
    FPDF_PAGEOBJECT textObj = (FPDF_PAGEOBJECT) textObjPtr;
    if (!textObj) return 0;
    return FPDFTextObj_GetTextRenderMode(textObj);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeTextObjSetTextRenderMode(JNIEnv *env, jobject thiz,
                                                                jlong textObjPtr, jint renderMode) {
    FPDF_PAGEOBJECT textObj = (FPDF_PAGEOBJECT) textObjPtr;
    if (!textObj) return JNI_FALSE;
    return FPDFTextObj_SetTextRenderMode(textObj, (FPDF_TEXT_RENDERMODE) renderMode) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjSetMatrix(JNIEnv *env, jobject thiz,
                                                         jlong imageObjPtr, jfloat a, jfloat b, jfloat c, jfloat d, jfloat e, jfloat f) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    if (!imageObj) return JNI_FALSE;
    return FPDFImageObj_SetMatrix(imageObj, a, b, c, d, e, f) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjGetImageFilterCount(JNIEnv *env, jobject thiz,
                                                                   jlong imageObjPtr) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    if (!imageObj) return 0;
    return FPDFImageObj_GetImageFilterCount(imageObj);
}

JNIEXPORT jstring JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjGetImageFilter(JNIEnv *env, jobject thiz,
                                                              jlong imageObjPtr, jint index) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    if (!imageObj) return nullptr;
    unsigned long size = FPDFImageObj_GetImageFilter(imageObj, index, nullptr, 0);
    if (size == 0) return env->NewStringUTF("");
    char *buffer = new char[size];
    FPDFImageObj_GetImageFilter(imageObj, index, buffer, size);
    jstring result = env->NewStringUTF(buffer);
    delete[] buffer;
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjGetImagePixelSize(JNIEnv *env, jobject thiz,
                                                                 jlong imageObjPtr, jintArray size) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    if (!imageObj) return JNI_FALSE;
    unsigned int imgWidth = 0, imgHeight = 0;
    if (!FPDFImageObj_GetImagePixelSize(imageObj, &imgWidth, &imgHeight)) return JNI_FALSE;
    jint *body = env->GetIntArrayElements(size, nullptr);
    body[0] = (jint) imgWidth; body[1] = (jint) imgHeight;
    env->ReleaseIntArrayElements(size, body, 0);
    return JNI_TRUE;
}

JNIEXPORT jbyteArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjGetImageDataDecoded(JNIEnv *env, jobject thiz,
                                                                   jlong imageObjPtr) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    if (!imageObj) return nullptr;
    unsigned long size = FPDFImageObj_GetImageDataDecoded(imageObj, nullptr, 0);
    if (size == 0) return nullptr;
    unsigned char *buffer = new unsigned char[size];
    FPDFImageObj_GetImageDataDecoded(imageObj, buffer, size);
    jbyteArray result = env->NewByteArray(size);
    env->SetByteArrayRegion(result, 0, size, (jbyte*) buffer);
    delete[] buffer;
    return result;
}

JNIEXPORT jbyteArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjGetImageDataRaw(JNIEnv *env, jobject thiz,
                                                               jlong imageObjPtr) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    if (!imageObj) return nullptr;
    unsigned long size = FPDFImageObj_GetImageDataRaw(imageObj, nullptr, 0);
    if (size == 0) return nullptr;
    unsigned char *buffer = new unsigned char[size];
    FPDFImageObj_GetImageDataRaw(imageObj, buffer, size);
    jbyteArray result = env->NewByteArray(size);
    env->SetByteArrayRegion(result, 0, size, (jbyte*) buffer);
    delete[] buffer;
    return result;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjGetBitmap(JNIEnv *env, jobject thiz,
                                                         jlong imageObjPtr) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    if (!imageObj) return 0;
    return (jlong) FPDFImageObj_GetBitmap(imageObj);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjSetBitmap(JNIEnv *env, jobject thiz,
                                                         jlong imageObjPtr, jlong pagePtr, jint width, jint height, jint stride, jintArray pixels) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!imageObj) return JNI_FALSE;
    jint *body = env->GetIntArrayElements(pixels, nullptr);
    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(width, height, FPDFBitmap_BGRA, body, stride * 4);
    FPDF_PAGE pages[] = {page};
    FPDF_BOOL result = FPDFImageObj_SetBitmap(pages, 1, imageObj, bitmap);
    FPDFBitmap_Destroy(bitmap);
    env->ReleaseIntArrayElements(pixels, body, 0);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjGetRenderedBitmap(JNIEnv *env, jobject thiz,
                                                                 jlong docPtr, jlong pagePtr, jlong imageObjPtr) {
    FPDF_DOCUMENT doc = (FPDF_DOCUMENT) docPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    if (!doc || !page || !imageObj) return 0;
    return (jlong) FPDFImageObj_GetRenderedBitmap(doc, page, imageObj);
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjLoadJpegFile(JNIEnv *env, jobject thiz,
                                                             jlong imageObjPtr, jlong pagePtr,
                                                             jbyteArray jpegData) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!imageObj || !jpegData) return JNI_FALSE;

    jsize len = env->GetArrayLength(jpegData);
    jbyte* elements = env->GetByteArrayElements(jpegData, nullptr);

    SyncFileReadCtx ctx = { (unsigned char*) elements, (unsigned long) len };
    FPDF_FILEACCESS fileAccess;
    fileAccess.m_FileLen = (unsigned long) len;
    fileAccess.m_GetBlock = SyncFileReadBlock;
    fileAccess.m_Param = &ctx;

    FPDF_PAGE pages[] = {page};
    FPDF_BOOL result = FPDFImageObj_LoadJpegFile(pages, 1, imageObj, &fileAccess);
    env->ReleaseByteArrayElements(jpegData, elements, JNI_ABORT);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjLoadJpegFileInline(JNIEnv *env, jobject thiz,
                                                                    jlong imageObjPtr, jlong pagePtr,
                                                                    jbyteArray jpegData) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!imageObj || !jpegData) return JNI_FALSE;

    jsize len = env->GetArrayLength(jpegData);
    jbyte* elements = env->GetByteArrayElements(jpegData, nullptr);

    SyncFileReadCtx ctx = { (unsigned char*) elements, (unsigned long) len };
    FPDF_FILEACCESS fileAccess;
    fileAccess.m_FileLen = (unsigned long) len;
    fileAccess.m_GetBlock = SyncFileReadBlock;
    fileAccess.m_Param = &ctx;

    FPDF_PAGE pages[] = {page};
    FPDF_BOOL result = FPDFImageObj_LoadJpegFileInline(pages, 1, imageObj, &fileAccess);
    env->ReleaseByteArrayElements(jpegData, elements, JNI_ABORT);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jbyteArray JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjGetIccProfileDataDecoded(JNIEnv *env, jobject thiz,
                                                                        jlong imageObjPtr, jlong pagePtr) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!imageObj || !page) return nullptr;
    size_t size = 0;
    if (!FPDFImageObj_GetIccProfileDataDecoded(imageObj, page, nullptr, 0, &size)) return nullptr;
    if (size == 0) return nullptr;
    unsigned char *buffer = new unsigned char[size];
    if (!FPDFImageObj_GetIccProfileDataDecoded(imageObj, page, buffer, size, &size)) {
        delete[] buffer;
        return nullptr;
    }
    jbyteArray result = env->NewByteArray(size);
    env->SetByteArrayRegion(result, 0, size, (jbyte*) buffer);
    delete[] buffer;
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_hyntix_pdfium_PdfiumCore_nativeImageObjGetImageMetadata(JNIEnv *env, jobject thiz,
                                                                 jlong imageObjPtr, jlong pagePtr,
                                                                 jintArray intValues, jfloatArray floatValues) {
    FPDF_PAGEOBJECT imageObj = (FPDF_PAGEOBJECT) imageObjPtr;
    FPDF_PAGE page = (FPDF_PAGE) pagePtr;
    if (!imageObj) return JNI_FALSE;

    FPDF_IMAGEOBJ_METADATA meta;
    if (!FPDFImageObj_GetImageMetadata(imageObj, page, &meta)) return JNI_FALSE;

    if (intValues && env->GetArrayLength(intValues) >= 5) {
        jint ints[5] = {
            (jint) meta.width,
            (jint) meta.height,
            (jint) meta.bits_per_pixel,
            (jint) meta.colorspace,
            (jint) meta.marked_content_id
        };
        env->SetIntArrayRegion(intValues, 0, 5, ints);
    }
    if (floatValues && env->GetArrayLength(floatValues) >= 2) {
        jfloat floats[2] = { meta.horizontal_dpi, meta.vertical_dpi };
        env->SetFloatArrayRegion(floatValues, 0, 2, floats);
    }
    return JNI_TRUE;
}

} // extern "C"
