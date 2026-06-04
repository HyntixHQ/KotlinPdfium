package com.hyntix.pdfium

/**
 * Core PDFium interface for low-level operations.
 * 
 * This class provides JNI bindings to the native PDFium library.
 * It handles library initialization, document loading, and basic operations.
 */
class PdfiumCore {
    
    companion object {
        init {
            System.loadLibrary("pdfium")
            System.loadLibrary("pdfium_jni")
        }
        
        // Error codes from fpdfview.h
        const val FPDF_ERR_SUCCESS = 0
        const val FPDF_ERR_UNKNOWN = 1
        const val FPDF_ERR_FILE = 2
        const val FPDF_ERR_FORMAT = 3
        const val FPDF_ERR_PASSWORD = 4
        const val FPDF_ERR_SECURITY = 5
        const val FPDF_ERR_PAGE = 6
        
        // Progressive rendering status codes
        const val RENDER_READY = 0
        const val RENDER_TOBECONTINUED = 1
        const val RENDER_DONE = 2
        const val RENDER_FAILED = 3

        // Save flags from fpdf_save.h
        const val FPDF_INCREMENTAL = 1
        const val FPDF_NO_INCREMENTAL = 2
        const val FPDF_REMOVE_SECURITY = 4
        const val FPDF_REMOVE_SECURITY_DEPRECATED = 3
        const val FPDF_SUBSET_NEW_FONTS = 8

        // Data Availability constants from fpdf_dataavail.h
        const val PDF_LINEARIZATION_UNKNOWN = -1
        const val PDF_NOT_LINEARIZED = 0
        const val PDF_LINEARIZED = 1
        const val PDF_DATA_ERROR = -1
        const val PDF_DATA_NOTAVAIL = 0
        const val PDF_DATA_AVAIL = 1
        const val PDF_FORM_ERROR = -1
        const val PDF_FORM_NOTAVAIL = 0
        const val PDF_FORM_AVAIL = 1
        const val PDF_FORM_NOTEXIST = 2

        // fpdf_sysfontinfo.h charset constants
        const val FXFONT_ANSI_CHARSET = 0
        const val FXFONT_DEFAULT_CHARSET = 1
        const val FXFONT_SYMBOL_CHARSET = 2
        const val FXFONT_SHIFTJIS_CHARSET = 128
        const val FXFONT_HANGEUL_CHARSET = 129
        const val FXFONT_GB2312_CHARSET = 134
        const val FXFONT_CHINESEBIG5_CHARSET = 136

        // Unsupported object types from fpdf_ext.h
        const val FPDF_UNSP_DOC_XFAFORM = 1
        const val FPDF_UNSP_DOC_PORTABLECOLLECTION = 2
        const val FPDF_UNSP_DOC_ATTACHMENT = 3
        const val FPDF_UNSP_DOC_SECURITY = 4
        const val FPDF_UNSP_DOC_SHAREDREVIEW = 5
        const val FPDF_UNSP_DOC_SHAREDFORM_ACROBAT = 6
        const val FPDF_UNSP_DOC_SHAREDFORM_FILESYSTEM = 7
        const val FPDF_UNSP_DOC_SHAREDFORM_EMAIL = 8
        const val FPDF_UNSP_ANNOT_3DANNOT = 11
        const val FPDF_UNSP_ANNOT_MOVIE = 12
        const val FPDF_UNSP_ANNOT_SOUND = 13
        const val FPDF_UNSP_ANNOT_SCREEN_MEDIA = 14
        const val FPDF_UNSP_ANNOT_SCREEN_RICHMEDIA = 15
        const val FPDF_UNSP_ANNOT_ATTACHMENT = 16
        const val FPDF_UNSP_ANNOT_SIG = 17

        // Font weight constants
        const val FXFONT_FW_NORMAL = 400
        const val FXFONT_FW_BOLD = 700

        // Colorspace constants from fpdf_edit.h
        const val FPDF_COLORSPACE_UNKNOWN = 0
        const val FPDF_COLORSPACE_DEVICEGRAY = 1
        const val FPDF_COLORSPACE_DEVICERGB = 2
        const val FPDF_COLORSPACE_DEVICECMYK = 3
        const val FPDF_COLORSPACE_CALGRAY = 4
        const val FPDF_COLORSPACE_CALRGB = 5
        const val FPDF_COLORSPACE_LAB = 6
        const val FPDF_COLORSPACE_ICCBASED = 7
        const val FPDF_COLORSPACE_SEPARATION = 8
        const val FPDF_COLORSPACE_DEVICEN = 9
        const val FPDF_COLORSPACE_INDEXED = 10
        const val FPDF_COLORSPACE_PATTERN = 11

        // Callback for unsupported objects (fpdf_ext.h) — called from JNI
        @JvmStatic var onUnsupportedObjectCallback: ((Int) -> Unit)? = null

        @JvmStatic fun onUnsupportedObject(nType: Int) {
            onUnsupportedObjectCallback?.invoke(nType)
        }

    }
    
    private var isInitialized = false
    
    /**
     * Initialize the PDFium library.
     * Must be called before any other operations.
     */
    fun initLibrary() {
        if (!isInitialized) {
            nativeInitLibrary()
            isInitialized = true
        }
    }
    
    /**
     * Destroy the PDFium library.
     * Call when completely done with PDFium.
     */
    fun destroyLibrary() {
        if (isInitialized) {
            nativeDestroyLibrary()
            isInitialized = false
        }
    }

    fun setSandBoxPolicy(policy: Int, enabled: Boolean) = nativeSetSandBoxPolicy(policy, enabled)
    fun setPrintMode(mode: Int): Boolean = nativeSetPrintMode(mode)
    fun getFileVersion(docPtr: Long): Int {
        val version = IntArray(1)
        if (nativeGetFileVersion(docPtr, version)) return version[0]
        return 0
    }

    fun getPageWidthFFloat(pagePtr: Long): Float = nativeGetPageWidthFFloat(pagePtr) / 1000000f
    fun getPageHeightFFloat(pagePtr: Long): Float = nativeGetPageHeightFFloat(pagePtr) / 1000000f
    fun getPageBoundingBoxFFloat(pagePtr: Long): FloatArray {
        val rect = FloatArray(4)
        if (nativeGetPageBoundingBoxFFloat(pagePtr, rect)) return rect
        return floatArrayOf(0f, 0f, 0f, 0f)
    }

    fun bitmapGetFormat(bitmapPtr: Long) = nativeBitmapGetFormat(bitmapPtr)
    fun setDefaultPrinterMode(mode: Int) = nativeSetDefaultPrinterMode(mode)
    fun getDefaultPrinterMode() = nativeGetDefaultPrinterMode()
    fun getDuplexOperation(docPtr: Long) = nativeGetDuplexOperation(docPtr)
    fun getRecommendedV8Flags() = nativeGetRecommendedV8Flags()
    fun getArrayBufferAllocatorSharedInstance() = nativeGetArrayBufferAllocatorSharedInstance()
    fun getXfaPacketCount(docPtr: Long) = nativeGetXFAPacketCount(docPtr)
    fun getXfaPacketName(docPtr: Long, index: Int) = nativeGetXFAPacketName(docPtr, index)
    fun getXfaPacketContent(docPtr: Long, index: Int) = nativeGetXFAPacketContent(docPtr, index)

    /**
     * Get the last error code.
     */
    fun getLastError(): PdfiumError {
        return PdfiumError.fromCode(nativeGetLastError())
    }
    
    /**
     * Open a PDF document from a file descriptor.
     * 
     * @param fd File descriptor of the PDF file
     * @param password Optional password for encrypted PDFs
     * @return PdfDocument or null if failed
     */
    fun openDocument(fd: Int, password: String? = null): PdfDocument? {
        val docPtr = nativeOpenDocument(fd, password)
        return if (docPtr != 0L) PdfDocument(this, docPtr) else null
    }
    
    /**
     * Open a PDF document from a byte array.
     * 
     * @param data PDF file bytes
     * @param password Optional password for encrypted PDFs
     * @return PdfDocument or null if failed
     */
    fun openDocument(data: ByteArray, password: String? = null): PdfDocument? {
        val docPtr = nativeOpenMemDocument(data, password)
        return if (docPtr != 0L) PdfDocument(this, docPtr) else null
    }

    fun openDocument64(data: ByteArray, password: String? = null): PdfDocument? {
        val docPtr = nativeOpenMemDocument64(data, password)
        return if (docPtr != 0L) PdfDocument(this, docPtr) else null
    }
    
    /**
     * Open a PDF document from a file path.
     *
     * @param path File path to the PDF
     * @param password Optional password for encrypted PDFs
     * @return PdfDocument or null if failed
     */
    fun openDocument(path: String, password: String? = null): PdfDocument? {
        val docPtr = nativeOpenDocumentPath(path, password)
        return if (docPtr != 0L) PdfDocument(this, docPtr) else null
    }

    /**
     * Create a new empty PDF document.
     * 
     * @return PdfDocument or null if failed
     */
    fun newDocument(): PdfDocument? {
        val docPtr = nativeNewDocument()
        return if (docPtr != 0L) PdfDocument(this, docPtr) else null
    }

    fun saveDocument(docPtr: Long, path: String): Boolean {
        return nativeSaveDocument(docPtr, path)
    }

    fun saveDocumentWithVersion(docPtr: Long, path: String, fileVersion: Int): Boolean {
        return nativeSaveDocumentWithVersion(docPtr, path, fileVersion)
    }

    fun newPage(docPtr: Long, index: Int, width: Double, height: Double): Long {
        return nativeNewPage(docPtr, index, width, height)
    }

    // Internal methods for PdfDocument to use
    fun closeDocument(docPtr: Long) {
        nativeCloseDocument(docPtr)
    }
    
    fun getPageCount(docPtr: Long): Int {
        return nativeGetPageCount(docPtr)
    }
    
    fun getMetaText(docPtr: Long, tag: String): String {
        return nativeGetMetaText(docPtr, tag) ?: ""
    }

    /**
     * Get page label (actual page number as displayed in PDF)
     * Returns empty string if no label is defined for the page
     */
    fun getPageLabel(docPtr: Long, pageIndex: Int): String {
        return nativeGetPageLabel(docPtr, pageIndex) ?: ""
    }

    fun getCatalogLanguage(docPtr: Long): String {
        return nativeGetCatalogLanguage(docPtr) ?: ""
    }

    fun setCatalogLanguage(docPtr: Long, language: String): Boolean {
        return nativeSetCatalogLanguage(docPtr, language)
    }

    fun catalogIsTagged(docPtr: Long): Boolean {
        return nativeCatalogIsTagged(docPtr)
    }

    // Page operations
    fun loadPage(docPtr: Long, pageIndex: Int): Long {
        return nativeLoadPage(docPtr, pageIndex)
    }

    fun closePage(pagePtr: Long) {
        nativeClosePage(pagePtr)
    }

    fun getPageWidth(pagePtr: Long): Double {
        return nativeGetPageWidth(pagePtr)
    }

    fun getPageHeight(pagePtr: Long): Double {
        return nativeGetPageHeight(pagePtr)
    }

    fun getPageBoundingBox(pagePtr: Long): FloatArray? {
        val result = FloatArray(4)
        return if (nativeGetPageBoundingBox(pagePtr, result)) result else null
    }

    /**
     * Get page size by index WITHOUT loading the page.
     * Much faster than loadPage+getWidth/getHeight for bulk size queries.
     * 
     * @param docPtr Document pointer
     * @param pageIndex Page index (0-based)
     * @return Pair of (width, height) in points
     */
    fun getPageSizeByIndex(docPtr: Long, pageIndex: Int): Pair<Double, Double> {
        val result = nativeGetPageSizeByIndex(docPtr, pageIndex)
        return if (result != null && result.size == 2) {
            Pair(result[0], result[1])
        } else {
            Pair(595.0, 842.0) // Default A4
        }
    }

    fun renderPageBitmap(
        pagePtr: Long,
        bitmap: Any,
        startX: Int,
        startY: Int,
        drawWidth: Int,
        drawHeight: Int,
        renderAnnot: Boolean = false
    ) {
        nativeRenderPageBitmap(pagePtr, bitmap, startX, startY, drawWidth, drawHeight, renderAnnot)
    }


    


    // Text Operations
    fun loadTextPage(docPtr: Long, pagePtr: Long): Long {
        return nativeLoadTextPage(docPtr, pagePtr)
    }

    fun closeTextPage(textPagePtr: Long) {
        nativeCloseTextPage(textPagePtr)
    }

    fun getTextCount(textPagePtr: Long): Int {
        return nativeTextCountChars(textPagePtr)
    }

    fun getText(textPagePtr: Long, startIndex: Int, count: Int): String {
        return nativeGetText(textPagePtr, startIndex, count) ?: ""
    }

    /**
     * Extract text from a bounded region on a text page.
     * @param textPagePtr Text page pointer
     * @param left,top,right,bottom Region bounds in PDF coordinates
     * @return Extracted text
     */
    fun getBoundedText(textPagePtr: Long, left: Double, top: Double, right: Double, bottom: Double): String {
        return nativeGetBoundedText(textPagePtr, left, top, right, bottom) ?: ""
    }

    fun getCharBox(textPagePtr: Long, index: Int): DoubleArray {
        val result = DoubleArray(4)
        nativeGetCharBox(textPagePtr, index, result)
        return result
    }

    fun getCharIndexAtPos(textPagePtr: Long, x: Double, y: Double, xTolerance: Double, yTolerance: Double): Int {
        return nativeGetCharIndexAtPos(textPagePtr, x, y, xTolerance, yTolerance)
    }

    fun getCharIndexFromTextIndex(textPagePtr: Long, nTextIndex: Int): Int {
        return nativeGetCharIndexFromTextIndex(textPagePtr, nTextIndex)
    }

    fun getTextIndexFromCharIndex(textPagePtr: Long, nCharIndex: Int): Int {
        return nativeGetTextIndexFromCharIndex(textPagePtr, nCharIndex)
    }

    fun textGetCharAngle(textPagePtr: Long, index: Int) = nativeTextGetCharAngle(textPagePtr, index)
    fun textGetCharOrigin(textPagePtr: Long, index: Int): DoubleArray? {
        val origin = DoubleArray(2)
        return if (nativeTextGetCharOrigin(textPagePtr, index, origin)) origin else null
    }

    fun textGetFillColor(textPagePtr: Long, index: Int): IntArray? {
        val color = IntArray(4)
        return if (nativeTextGetFillColor(textPagePtr, index, color)) color else null
    }

    fun textGetFontInfo(textPagePtr: Long, index: Int) = nativeTextGetFontInfo(textPagePtr, index)
    fun textGetFontSize(textPagePtr: Long, index: Int) = nativeTextGetFontSize(textPagePtr, index)
    fun textGetFontWeight(textPagePtr: Long, index: Int) = nativeTextGetFontWeight(textPagePtr, index)
    fun textGetLooseCharBox(textPagePtr: Long, index: Int): DoubleArray? {
        val rect = DoubleArray(4)
        return if (nativeTextGetLooseCharBox(textPagePtr, index, rect)) rect else null
    }

    fun textGetMatrix(textPagePtr: Long, index: Int): FloatArray? {
        val matrix = FloatArray(6)
        return if (nativeTextGetMatrix(textPagePtr, index, matrix)) matrix else null
    }

    fun textGetStrokeColor(textPagePtr: Long, index: Int): IntArray? {
        val color = IntArray(4)
        return if (nativeTextGetStrokeColor(textPagePtr, index, color)) color else null
    }

    fun textGetTextObject(textPagePtr: Long, index: Int) = nativeTextGetTextObject(textPagePtr, index)
    fun textGetUnicode(textPagePtr: Long, index: Int) = nativeTextGetUnicode(textPagePtr, index)
    fun textHasUnicodeMapError(textPagePtr: Long, index: Int) = nativeTextHasUnicodeMapError(textPagePtr, index)
    fun textIsGenerated(textPagePtr: Long, index: Int) = nativeTextIsGenerated(textPagePtr, index)
    fun textIsHyphen(textPagePtr: Long, index: Int) = nativeTextIsHyphen(textPagePtr, index)

    // Search Operations
    fun textFindStart(textPagePtr: Long, query: String, matchCase: Boolean, matchWholeWord: Boolean): Long {
        return nativeTextFindStart(textPagePtr, query, matchCase, matchWholeWord)
    }

    fun textFindNext(searchHandle: Long): Boolean {
        return nativeTextFindNext(searchHandle)
    }

    fun textFindPrev(searchHandle: Long): Boolean {
        return nativeTextFindPrev(searchHandle)
    }

    fun textGetSchResultIndex(searchHandle: Long): Int {
        return nativeTextGetSchResultIndex(searchHandle)
    }

    fun textGetSchCount(searchHandle: Long): Int {
        return nativeTextGetSchCount(searchHandle)
    }

    fun textFindClose(searchHandle: Long) {
        nativeTextFindClose(searchHandle)
    }

    // Bookmark Operations
    fun getFirstChildBookmark(docPtr: Long, bookmarkPtr: Long): Long {
        return nativeGetFirstChildBookmark(docPtr, bookmarkPtr)
    }

    fun getNextSiblingBookmark(docPtr: Long, bookmarkPtr: Long): Long {
        return nativeGetNextSiblingBookmark(docPtr, bookmarkPtr)
    }

    fun getBookmarkTitle(bookmarkPtr: Long): String {
        return nativeGetBookmarkTitle(bookmarkPtr) ?: ""
    }

    fun getBookmarkDestIndex(docPtr: Long, bookmarkPtr: Long): Long {
        return nativeGetBookmarkDestIndex(docPtr, bookmarkPtr)
    }

    // Link Operations
    fun getLinkAtPoint(pagePtr: Long, x: Double, y: Double): Long {
        return nativeGetLinkAtPoint(pagePtr, x, y)
    }

    fun getLinkDestIndex(docPtr: Long, linkPtr: Long): Int {
        return nativeGetLinkDestIndex(docPtr, linkPtr)
    }

    fun getLinkURI(docPtr: Long, linkPtr: Long): String? {
        return nativeGetLinkURI(docPtr, linkPtr)
    }

    fun getLinkRect(linkPtr: Long): DoubleArray {
        val result = DoubleArray(4)
        nativeGetLinkRect(linkPtr, result)
        return result
    }

    // Annotation Operations
    fun getAnnotCount(pagePtr: Long): Int {
        return nativeGetAnnotCount(pagePtr)
    }

    fun getAnnot(pagePtr: Long, index: Int): Long {
        return nativeGetAnnot(pagePtr, index)
    }

    fun closeAnnot(annotPtr: Long) {
        nativeCloseAnnot(annotPtr)
    }

    fun getAnnotSubtype(annotPtr: Long): Int {
        return nativeGetAnnotSubtype(annotPtr)
    }

    fun getAnnotRect(annotPtr: Long): DoubleArray {
        val result = DoubleArray(4)
        nativeGetAnnotRect(annotPtr, result)
        return result
    }

    // Native methods
    private external fun nativeInitLibrary()
    private external fun nativeDestroyLibrary()
    private external fun nativeSetSandBoxPolicy(policy: Int, enabled: Boolean)
    private external fun nativeSetPrintMode(mode: Int): Boolean
    private external fun nativeGetFileVersion(docPtr: Long, version: IntArray): Boolean
    private external fun nativeGetPageWidthFFloat(pagePtr: Long): Int
    private external fun nativeGetPageHeightFFloat(pagePtr: Long): Int
    private external fun nativeGetPageBoundingBoxFFloat(pagePtr: Long, rect: FloatArray): Boolean
    private external fun nativeBitmapGetFormat(bitmapPtr: Long): Int
    private external fun nativeSetDefaultPrinterMode(mode: Int)
    private external fun nativeGetDefaultPrinterMode(): Int
    private external fun nativeGetDuplexOperation(docPtr: Long): Int
    private external fun nativeGetRecommendedV8Flags(): String?
    private external fun nativeGetArrayBufferAllocatorSharedInstance(): Long
    private external fun nativeGetXFAPacketCount(docPtr: Long): Int
    private external fun nativeGetXFAPacketName(docPtr: Long, index: Int): String?
    private external fun nativeGetXFAPacketContent(docPtr: Long, index: Int): ByteArray?
    private external fun nativeBStrInit(bstrPtr: Long): Boolean
    private external fun nativeBStrSet(bstrPtr: Long, str: String): Boolean
    private external fun nativeBStrClear(bstrPtr: Long)
    private external fun nativeGetLastError(): Int
    private external fun nativeOpenDocument(fd: Int, password: String?): Long
    private external fun nativeOpenMemDocument(data: ByteArray, password: String?): Long
    private external fun nativeOpenMemDocument64(data: ByteArray, password: String?): Long
    private external fun nativeOpenDocumentPath(path: String, password: String?): Long
    private external fun nativeCloseDocument(docPtr: Long)
    private external fun nativeGetPageCount(docPtr: Long): Int
    private external fun nativeGetMetaText(docPtr: Long, tag: String): String?
    private external fun nativeGetPageLabel(docPtr: Long, pageIndex: Int): String?
    private external fun nativeGetCatalogLanguage(docPtr: Long): String?
    private external fun nativeSetCatalogLanguage(docPtr: Long, language: String): Boolean
    private external fun nativeCatalogIsTagged(docPtr: Long): Boolean
    
    // Creation & Saving Native methods
    private external fun nativeNewDocument(): Long
    private external fun nativeNewPage(docPtr: Long, index: Int, width: Double, height: Double): Long
    private external fun nativeSaveDocument(docPtr: Long, path: String): Boolean 
    private external fun nativeSaveDocumentWithVersion(docPtr: Long, path: String, fileVersion: Int): Boolean
    
    // Form Filling Native methods
    private external fun nativeInitFormFillEnvironment(docPtr: Long): Long
    private external fun nativeExitFormFillEnvironment(formHandlePtr: Long)
    private external fun nativeFORMOnAfterLoadPage(pagePtr: Long, formHandlePtr: Long)
    private external fun nativeFORMOnBeforeClosePage(pagePtr: Long, formHandlePtr: Long)
    private external fun nativeFPDFFFLDraw(
        formHandlePtr: Long, 
        bitmap: Any, 
        pagePtr: Long, 
        startX: Int, 
        startY: Int, 
        drawWidth: Int, 
        drawHeight: Int, 
        rotate: Int, 
        flags: Int
    )

    // Form Filling Operations
    fun initFormFillEnvironment(docPtr: Long): Long {
        return nativeInitFormFillEnvironment(docPtr)
    }

    fun exitFormFillEnvironment(formHandlePtr: Long) {
        nativeExitFormFillEnvironment(formHandlePtr)
    }

    fun formOnAfterLoadPage(pagePtr: Long, formHandlePtr: Long) {
        nativeFORMOnAfterLoadPage(pagePtr, formHandlePtr)
    }

    fun formOnBeforeClosePage(pagePtr: Long, formHandlePtr: Long) {
        nativeFORMOnBeforeClosePage(pagePtr, formHandlePtr)
    }

    fun renderFormBitmap(
        formHandlePtr: Long,
        bitmap: Any,
        pagePtr: Long,
        startX: Int,
        startY: Int,
        drawWidth: Int,
        drawHeight: Int,
        rotate: Int,
        flags: Int
    ) {
        nativeFPDFFFLDraw(formHandlePtr, bitmap, pagePtr, startX, startY, drawWidth, drawHeight, rotate, flags)
    }

    
    // Page Native methods
    private external fun nativeLoadPage(docPtr: Long, pageIndex: Int): Long
    private external fun nativeClosePage(pagePtr: Long)
    private external fun nativeGetPageWidth(pagePtr: Long): Double
    private external fun nativeGetPageHeight(pagePtr: Long): Double
    private external fun nativeGetPageBoundingBox(pagePtr: Long, result: FloatArray): Boolean
    private external fun nativeGetPageSizeByIndex(docPtr: Long, pageIndex: Int): DoubleArray?
    private external fun nativeRenderPageBitmap(
        pagePtr: Long, 
        bitmap: Any, 
        startX: Int, 
        startY: Int, 
        drawWidth: Int, 
        drawHeight: Int, 
        renderAnnot: Boolean
    )
    private external fun nativeDeviceToPage(
        pagePtr: Long, 
        startX: Int, startY: Int, 
        sizeX: Int, sizeY: Int, 
        rotate: Int, 
        deviceX: Int, deviceY: Int, 
        result: DoubleArray
    )
    private external fun nativePageToDevice(
        pagePtr: Long, 
        startX: Int, startY: Int, 
        sizeX: Int, sizeY: Int, 
        rotate: Int, 
        pageX: Double, pageY: Double, 
        result: IntArray
    )

    fun mapPageToDevice(
        pagePtr: Long,
        startX: Int, startY: Int,
        sizeX: Int, sizeY: Int,
        rotate: Int,
        pageX: Double, pageY: Double
    ): IntArray {
        val result = IntArray(2)
        nativePageToDevice(pagePtr, startX, startY, sizeX, sizeY, rotate, pageX, pageY, result)
        return result
    }

    fun mapDeviceToPage(
        pagePtr: Long,
        startX: Int, startY: Int,
        sizeX: Int, sizeY: Int,
        rotate: Int,
        deviceX: Int, deviceY: Int
    ): DoubleArray {
        val result = DoubleArray(2)
        nativeDeviceToPage(pagePtr, startX, startY, sizeX, sizeY, rotate, deviceX, deviceY, result)
        return result
    }

    // Text Native methods
    private external fun nativeLoadTextPage(docPtr: Long, pagePtr: Long): Long
    private external fun nativeCloseTextPage(textPagePtr: Long)
    private external fun nativeTextCountChars(textPagePtr: Long): Int
    private external fun nativeGetText(textPagePtr: Long, startIndex: Int, count: Int): String?
    private external fun nativeGetBoundedText(textPagePtr: Long, left: Double, top: Double, right: Double, bottom: Double): String?
    private external fun nativeGetCharBox(textPagePtr: Long, index: Int, result: DoubleArray)
    private external fun nativeGetCharIndexAtPos(textPagePtr: Long, x: Double, y: Double, xTolerance: Double, yTolerance: Double): Int
    private external fun nativeGetCharIndexFromTextIndex(textPagePtr: Long, nTextIndex: Int): Int
    private external fun nativeGetTextIndexFromCharIndex(textPagePtr: Long, nCharIndex: Int): Int
    private external fun nativeTextGetCharAngle(textPagePtr: Long, index: Int): Float
    private external fun nativeTextGetCharOrigin(textPagePtr: Long, index: Int, origin: DoubleArray): Boolean
    private external fun nativeTextGetFillColor(textPagePtr: Long, index: Int, color: IntArray): Boolean
    private external fun nativeTextGetFontInfo(textPagePtr: Long, index: Int): String?
    private external fun nativeTextGetFontSize(textPagePtr: Long, index: Int): Double
    private external fun nativeTextGetFontWeight(textPagePtr: Long, index: Int): Int
    private external fun nativeTextGetLooseCharBox(textPagePtr: Long, index: Int, rect: DoubleArray): Boolean
    private external fun nativeTextGetMatrix(textPagePtr: Long, index: Int, matrix: FloatArray): Boolean
    private external fun nativeTextGetStrokeColor(textPagePtr: Long, index: Int, color: IntArray): Boolean
    private external fun nativeTextGetTextObject(textPagePtr: Long, index: Int): Long
    private external fun nativeTextGetUnicode(textPagePtr: Long, index: Int): Char
    private external fun nativeTextHasUnicodeMapError(textPagePtr: Long, index: Int): Boolean
    private external fun nativeTextIsGenerated(textPagePtr: Long, index: Int): Boolean
    private external fun nativeTextIsHyphen(textPagePtr: Long, index: Int): Boolean
    
    // Search Native methods
    private external fun nativeTextFindStart(textPagePtr: Long, query: String, matchCase: Boolean, matchWholeWord: Boolean): Long
    private external fun nativeTextFindNext(searchHandle: Long): Boolean
    private external fun nativeTextFindPrev(searchHandle: Long): Boolean
    private external fun nativeTextGetSchResultIndex(searchHandle: Long): Int
    private external fun nativeTextGetSchCount(searchHandle: Long): Int
    private external fun nativeTextFindClose(searchHandle: Long)

    // Bookmark Native methods
    private external fun nativeGetFirstChildBookmark(docPtr: Long, bookmarkPtr: Long): Long
    private external fun nativeGetNextSiblingBookmark(docPtr: Long, bookmarkPtr: Long): Long
    private external fun nativeGetBookmarkTitle(bookmarkPtr: Long): String?
    private external fun nativeGetBookmarkDestIndex(docPtr: Long, bookmarkPtr: Long): Long
    
    // Link Native methods
    private external fun nativeGetLinkAtPoint(pagePtr: Long, x: Double, y: Double): Long
    private external fun nativeGetLinkDestIndex(docPtr: Long, linkPtr: Long): Int
    private external fun nativeGetLinkURI(docPtr: Long, linkPtr: Long): String?
    private external fun nativeGetLinkRect(linkPtr: Long, result: DoubleArray)

    // Annotation Native methods
    private external fun nativeGetAnnotCount(pagePtr: Long): Int
    private external fun nativeGetAnnot(pagePtr: Long, index: Int): Long
    private external fun nativeCloseAnnot(annotPtr: Long)
    private external fun nativeGetAnnotSubtype(annotPtr: Long): Int
    private external fun nativeGetAnnotRect(annotPtr: Long, result: DoubleArray)
    
    // Annotation Editing Native methods
    private external fun nativeCreateAnnot(pagePtr: Long, subtype: Int): Long
    private external fun nativeSetAnnotRect(annotPtr: Long, rect: DoubleArray): Boolean
    private external fun nativeSetAnnotContents(annotPtr: Long, contents: String): Boolean
    private external fun nativeSetAnnotColor(annotPtr: Long, type: Int, r: Int, g: Int, b: Int, a: Int): Boolean
    private external fun nativeSetAnnotFlags(annotPtr: Long, flags: Int): Boolean
    private external fun nativeGetLinkFromAnnot(annotPtr: Long): Long
    private external fun nativeGetAnnotStringValue(annotPtr: Long, key: String): String?
    private external fun nativePageGetAnnotIndex(pagePtr: Long, annotPtr: Long): Int
    private external fun nativeAnnotHasKey(annotPtr: Long, key: String): Boolean
    private external fun nativeAnnotGetValueType(annotPtr: Long, key: String): Int
    private external fun nativeAnnotGetNumberValue(annotPtr: Long, key: String): Double
    private external fun nativeAnnotSetURI(annotPtr: Long, uri: String): Boolean
    private external fun nativeAnnotGetBorder(annotPtr: Long, border: FloatArray): Boolean
    private external fun nativeAnnotSetBorder(annotPtr: Long, horizontal: Float, vertical: Float, corner: Float): Boolean
    private external fun nativeAnnotGetFontColor(annotPtr: Long, color: IntArray): Boolean
    private external fun nativeAnnotSetFontColor(annotPtr: Long, r: Int, g: Int, b: Int, a: Int): Boolean
    private external fun nativeAnnotGetFontSize(annotPtr: Long): Double
    private external fun nativeAnnotGetFormFieldType(annotPtr: Long): Int
    private external fun nativeAnnotGetFormFieldName(annotPtr: Long): String?
    private external fun nativeAnnotGetFormFieldValue(annotPtr: Long): String?
    private external fun nativeAnnotGetFormControlCount(annotPtr: Long): Int
    private external fun nativeAnnotGetFormControlIndex(annotPtr: Long): Int
    private external fun nativeAnnotGetFormFieldAlternateName(annotPtr: Long, index: Int): String?
    private external fun nativeAnnotGetOptionCount(annotPtr: Long): Int
    private external fun nativeAnnotGetOptionLabel(annotPtr: Long, index: Int): String?
    private external fun nativeAnnotIsOptionSelected(annotPtr: Long, index: Int): Boolean
    private external fun nativeAnnotIsChecked(annotPtr: Long): Boolean
    private external fun nativeAnnotGetFocusableSubtypesCount(annotPtr: Long): Int
    private external fun nativeAnnotGetFocusableSubtypes(annotPtr: Long, subtypes: IntArray): Boolean
    private external fun nativeAnnotSetFocusableSubtypes(annotPtr: Long, subtypes: IntArray): Boolean
    private external fun nativeAnnotGetLinkedAnnot(annotPtr: Long, subtype: Int): Long
    private external fun nativeAnnotGetLine(annotPtr: Long, line: DoubleArray): Boolean
    private external fun nativeAnnotGetVerticesCount(annotPtr: Long): Int
    private external fun nativeAnnotGetVertices(annotPtr: Long, vertices: FloatArray): Boolean
    private external fun nativeAnnotGetInkListCount(annotPtr: Long): Int
    private external fun nativeAnnotGetInkListPath(annotPtr: Long, index: Int, points: FloatArray): Boolean
    private external fun nativeAnnotRemoveInkList(annotPtr: Long): Boolean
    private external fun nativeAnnotAddInkStroke(annotPtr: Long, points: FloatArray): Boolean
    private external fun nativeAnnotHasAttachmentPoints(annotPtr: Long): Boolean
    private external fun nativeAnnotSetAttachmentPoints(annotPtr: Long, quadPoints: FloatArray): Boolean
    private external fun nativeAnnotAppendAttachmentPoints(annotPtr: Long, quadPoints: FloatArray): Boolean
    private external fun nativeAnnotCountAttachmentPoints(annotPtr: Long): Int
    private external fun nativeAnnotGetObjectCount(annotPtr: Long): Int
    private external fun nativeAnnotGetObject(annotPtr: Long, index: Int): Long
    private external fun nativeAnnotAppendObject(annotPtr: Long, objPtr: Long): Boolean
    private external fun nativeAnnotRemoveObject(annotPtr: Long, index: Int): Boolean
    private external fun nativeAnnotUpdateObject(annotPtr: Long, objPtr: Long): Boolean
    private external fun nativeAnnotGetAP(annotPtr: Long, mode: Int): Int
    private external fun nativeAnnotSetAP(annotPtr: Long, mode: Int, value: String): Int
    private external fun nativeAnnotGetFileAttachment(annotPtr: Long): Long
    private external fun nativeAnnotAddFileAttachment(annotPtr: Long, name: String): Boolean
    private external fun nativeAnnotGetFormFieldAtPoint(docPtr: Long, pagePtr: Long, x: Double, y: Double): Long
    private external fun nativeAnnotGetFormFieldFlags(annotPtr: Long): Int
    private external fun nativeAnnotSetFormFieldFlags(annotPtr: Long, flags: Int): Boolean
    private external fun nativeAnnotGetFormAdditionalActionJavaScript(annotPtr: Long, eventType: Int): String?
    private external fun nativeAnnotIsSupportedSubtype(subtype: Int): Boolean
    private external fun nativeAnnotIsObjectSupportedSubtype(subtype: Int): Boolean

    // Annotation Editing Helpers
    fun createAnnot(pagePtr: Long, subtype: Int): Long {
        return nativeCreateAnnot(pagePtr, subtype)
    }

    fun setAnnotRect(annotPtr: Long, rect: DoubleArray): Boolean {
        return nativeSetAnnotRect(annotPtr, rect)
    }

    fun setAnnotContents(annotPtr: Long, contents: String): Boolean {
        return nativeSetAnnotContents(annotPtr, contents)
    }

    /**
     * Get an annotation's string value for a given key (e.g. "Contents", "Author", "Subj").
     */
    fun getAnnotStringValue(annotPtr: Long, key: String): String {
        return nativeGetAnnotStringValue(annotPtr, key) ?: ""
    }

    fun setAnnotColor(annotPtr: Long, type: Int, r: Int, g: Int, b: Int, a: Int): Boolean {
        return nativeSetAnnotColor(annotPtr, type, r, g, b, a)
    }

    fun setAnnotFlags(annotPtr: Long, flags: Int): Boolean {
        return nativeSetAnnotFlags(annotPtr, flags)
    }

    fun getLinkFromAnnot(annotPtr: Long): Long {
        return nativeGetLinkFromAnnot(annotPtr)
    }

    fun pageGetAnnotIndex(pagePtr: Long, annotPtr: Long) = nativePageGetAnnotIndex(pagePtr, annotPtr)
    fun annotHasKey(annotPtr: Long, key: String) = nativeAnnotHasKey(annotPtr, key)
    fun annotGetValueType(annotPtr: Long, key: String) = nativeAnnotGetValueType(annotPtr, key)
    fun annotGetNumberValue(annotPtr: Long, key: String) = nativeAnnotGetNumberValue(annotPtr, key)
    fun annotSetURI(annotPtr: Long, uri: String) = nativeAnnotSetURI(annotPtr, uri)
    fun annotGetBorder(annotPtr: Long): FloatArray? {
        val border = FloatArray(3)
        return if (nativeAnnotGetBorder(annotPtr, border)) border else null
    }

    fun annotSetBorder(annotPtr: Long, horizontal: Float, vertical: Float, corner: Float) = nativeAnnotSetBorder(annotPtr, horizontal, vertical, corner)
    fun annotGetFontColor(annotPtr: Long): IntArray? {
        val color = IntArray(4)
        return if (nativeAnnotGetFontColor(annotPtr, color)) color else null
    }

    fun annotSetFontColor(annotPtr: Long, r: Int, g: Int, b: Int, a: Int) = nativeAnnotSetFontColor(annotPtr, r, g, b, a)
    fun annotGetFontSize(annotPtr: Long) = nativeAnnotGetFontSize(annotPtr)
    fun annotGetFormFieldType(annotPtr: Long) = nativeAnnotGetFormFieldType(annotPtr)
    fun annotGetFormFieldName(annotPtr: Long) = nativeAnnotGetFormFieldName(annotPtr) ?: ""
    fun annotGetFormFieldValue(annotPtr: Long) = nativeAnnotGetFormFieldValue(annotPtr) ?: ""
    fun annotGetFormControlCount(annotPtr: Long) = nativeAnnotGetFormControlCount(annotPtr)
    fun annotGetFormControlIndex(annotPtr: Long) = nativeAnnotGetFormControlIndex(annotPtr)
    fun annotGetFormFieldAlternateName(annotPtr: Long, index: Int) = nativeAnnotGetFormFieldAlternateName(annotPtr, index) ?: ""
    fun annotGetOptionCount(annotPtr: Long) = nativeAnnotGetOptionCount(annotPtr)
    fun annotGetOptionLabel(annotPtr: Long, index: Int) = nativeAnnotGetOptionLabel(annotPtr, index) ?: ""
    fun annotIsOptionSelected(annotPtr: Long, index: Int) = nativeAnnotIsOptionSelected(annotPtr, index)
    fun annotIsChecked(annotPtr: Long) = nativeAnnotIsChecked(annotPtr)
    fun annotGetFocusableSubtypesCount(annotPtr: Long): Int = nativeAnnotGetFocusableSubtypesCount(annotPtr)
    fun annotGetFocusableSubtypes(annotPtr: Long): IntArray? {
        val count = nativeAnnotGetFocusableSubtypesCount(annotPtr)
        if (count <= 0) return null
        val subtypes = IntArray(count)
        return if (nativeAnnotGetFocusableSubtypes(annotPtr, subtypes)) subtypes else null
    }
    fun annotSetFocusableSubtypes(annotPtr: Long, subtypes: IntArray) = nativeAnnotSetFocusableSubtypes(annotPtr, subtypes)
    fun annotGetLinkedAnnot(annotPtr: Long, subtype: Int) = nativeAnnotGetLinkedAnnot(annotPtr, subtype)
    fun annotGetLine(annotPtr: Long): DoubleArray? {
        val line = DoubleArray(4)
        return if (nativeAnnotGetLine(annotPtr, line)) line else null
    }

    fun annotGetVerticesCount(annotPtr: Long) = nativeAnnotGetVerticesCount(annotPtr)
    fun annotGetVertices(annotPtr: Long, vertices: FloatArray) = nativeAnnotGetVertices(annotPtr, vertices)
    fun annotGetInkListCount(annotPtr: Long) = nativeAnnotGetInkListCount(annotPtr)
    fun annotGetInkListPath(annotPtr: Long, index: Int, points: FloatArray) = nativeAnnotGetInkListPath(annotPtr, index, points)
    fun annotRemoveInkList(annotPtr: Long) = nativeAnnotRemoveInkList(annotPtr)
    fun annotAddInkStroke(annotPtr: Long, points: FloatArray) = nativeAnnotAddInkStroke(annotPtr, points)
    fun annotHasAttachmentPoints(annotPtr: Long) = nativeAnnotHasAttachmentPoints(annotPtr)
    fun annotSetAttachmentPoints(annotPtr: Long, quadPoints: FloatArray) = nativeAnnotSetAttachmentPoints(annotPtr, quadPoints)
    fun annotAppendAttachmentPoints(annotPtr: Long, quadPoints: FloatArray) = nativeAnnotAppendAttachmentPoints(annotPtr, quadPoints)
    fun annotCountAttachmentPoints(annotPtr: Long) = nativeAnnotCountAttachmentPoints(annotPtr)
    fun annotGetObjectCount(annotPtr: Long) = nativeAnnotGetObjectCount(annotPtr)
    fun annotGetObject(annotPtr: Long, index: Int) = nativeAnnotGetObject(annotPtr, index)
    fun annotAppendObject(annotPtr: Long, objPtr: Long) = nativeAnnotAppendObject(annotPtr, objPtr)
    fun annotRemoveObject(annotPtr: Long, index: Int) = nativeAnnotRemoveObject(annotPtr, index)
    fun annotUpdateObject(annotPtr: Long, objPtr: Long) = nativeAnnotUpdateObject(annotPtr, objPtr)
    fun annotGetAP(annotPtr: Long, mode: Int) = nativeAnnotGetAP(annotPtr, mode)
    fun annotSetAP(annotPtr: Long, mode: Int, value: String) = nativeAnnotSetAP(annotPtr, mode, value)
    fun annotGetFileAttachment(annotPtr: Long) = nativeAnnotGetFileAttachment(annotPtr)
    fun annotAddFileAttachment(annotPtr: Long, name: String) = nativeAnnotAddFileAttachment(annotPtr, name)
    fun annotGetFormFieldAtPoint(docPtr: Long, pagePtr: Long, x: Double, y: Double) = nativeAnnotGetFormFieldAtPoint(docPtr, pagePtr, x, y)
    fun annotGetFormFieldFlags(annotPtr: Long) = nativeAnnotGetFormFieldFlags(annotPtr)
    fun annotSetFormFieldFlags(annotPtr: Long, flags: Int) = nativeAnnotSetFormFieldFlags(annotPtr, flags)
    fun annotGetFormAdditionalActionJavaScript(annotPtr: Long, eventType: Int) = nativeAnnotGetFormAdditionalActionJavaScript(annotPtr, eventType) ?: ""
    fun annotIsSupportedSubtype(subtype: Int) = nativeAnnotIsSupportedSubtype(subtype)
    fun annotIsObjectSupportedSubtype(subtype: Int) = nativeAnnotIsObjectSupportedSubtype(subtype)

    // Attachment Native methods
    private external fun nativeGetAttachmentCount(docPtr: Long): Int
    private external fun nativeGetAttachmentName(docPtr: Long, index: Int): String?
    private external fun nativeGetAttachmentFile(docPtr: Long, index: Int): ByteArray?

    // Attachment Helpers
    fun getAttachmentCount(docPtr: Long): Int {
        return nativeGetAttachmentCount(docPtr)
    }

    fun getAttachmentName(docPtr: Long, index: Int): String {
        return nativeGetAttachmentName(docPtr, index) ?: ""
    }

    fun getAttachmentFile(docPtr: Long, index: Int): ByteArray? {
        return nativeGetAttachmentFile(docPtr, index)
    }

    // Page Object Native methods
    private external fun nativeCountPageObjects(pagePtr: Long): Int
    private external fun nativeGetPageObject(pagePtr: Long, index: Int): Long
    private external fun nativeGetPageObjectType(pageObjPtr: Long): Int

    // Page Object Helpers
    fun countPageObjects(pagePtr: Long): Int {
        return nativeCountPageObjects(pagePtr)
    }

    fun getPageObject(pagePtr: Long, index: Int): Long {
        return nativeGetPageObject(pagePtr, index)
    }

    fun getPageObjectType(pageObjPtr: Long): Int {
        return nativeGetPageObjectType(pageObjPtr)
    }

    // Phase 8: Page Editing Objects
    private external fun nativeNewTextObj(docPtr: Long, fontName: String, fontSize: Float): Long
    private external fun nativeSetTextObjText(textObjPtr: Long, text: String): Boolean
    private external fun nativeCreateNewPath(x: Float, y: Float): Long
    private external fun nativePathMoveTo(pathObjPtr: Long, x: Float, y: Float): Boolean
    private external fun nativePathLineTo(pathObjPtr: Long, x: Float, y: Float): Boolean
    private external fun nativePathBezierTo(pathObjPtr: Long, x1: Float, y1: Float, x2: Float, y2: Float, x3: Float, y3: Float): Boolean
    private external fun nativePathClose(pathObjPtr: Long): Boolean
    private external fun nativePathSetDrawMode(pathObjPtr: Long, fillMode: Int, stroke: Boolean): Boolean
    private external fun nativePathSetStrokeWidth(pathObjPtr: Long, width: Float): Boolean
    private external fun nativeNewImageObj(docPtr: Long): Long
    private external fun nativeInsertObject(pagePtr: Long, pageObjPtr: Long): Boolean
    private external fun nativeRemoveObject(pagePtr: Long, pageObjPtr: Long): Boolean
    private external fun nativeSetObjectFillColor(pageObjPtr: Long, r: Int, g: Int, b: Int, a: Int)
    private external fun nativeSetObjectStrokeColor(pageObjPtr: Long, r: Int, g: Int, b: Int, a: Int)
    private external fun nativeGenerateContent(pagePtr: Long)
    private external fun nativeAddExistingMark(pageObjPtr: Long, markPtr: Long): Boolean
    private external fun nativeSetTextObjFontSize(textObjPtr: Long, size: Float): Boolean
    private external fun nativeSetTextPositions(textObjPtr: Long, positions: FloatArray): Boolean

    // Phase 8b: Additional fpdf_edit.h Page Object Functions
    private external fun nativePageObjCreateNew(docPtr: Long): Long
    private external fun nativePageObjCreateNewRect(docPtr: Long): Long
    private external fun nativePageObjCreateTextObj(docPtr: Long, fontName: String): Long
    private external fun nativePageObjDestroy(pageObjPtr: Long)
    private external fun nativePageObjHasTransparency(pageObjPtr: Long): Boolean
    private external fun nativePageObjGetMatrix(pageObjPtr: Long, matrix: FloatArray): Boolean
    private external fun nativePageObjSetMatrix(pageObjPtr: Long, matrix: FloatArray): Boolean
    private external fun nativePageObjTransformF(pageObjPtr: Long, a: Float, b: Float, c: Float, d: Float, e: Float, f: Float): Boolean
    private external fun nativePageObjGetRotatedBounds(pageObjPtr: Long, rect: FloatArray): Boolean
    private external fun nativePageObjGetLineCap(pageObjPtr: Long): Int
    private external fun nativePageObjSetLineCap(pageObjPtr: Long, lineCap: Int): Boolean
    private external fun nativePageObjGetLineJoin(pageObjPtr: Long): Int
    private external fun nativePageObjSetLineJoin(pageObjPtr: Long, lineJoin: Int): Boolean
    private external fun nativePageObjGetStrokeWidth(pageObjPtr: Long): Float
    private external fun nativePageObjGetDashPhase(pageObjPtr: Long, phase: FloatArray): Boolean
    private external fun nativePageObjSetDashPhase(pageObjPtr: Long, phase: Float): Boolean
    private external fun nativePageObjGetDashCount(pageObjPtr: Long): Int
    private external fun nativePageObjGetDashArray(pageObjPtr: Long, dashes: FloatArray): Boolean
    private external fun nativePageObjSetDashArray(pageObjPtr: Long, dashes: FloatArray): Boolean
    private external fun nativePageObjGetIsActive(pageObjPtr: Long): Boolean
    private external fun nativePageObjSetIsActive(pageObjPtr: Long, active: Boolean): Boolean
    private external fun nativePageObjCountMarks(pageObjPtr: Long): Int
    private external fun nativePageObjGetMark(pageObjPtr: Long, index: Int): Long
    private external fun nativePageObjAddMark(pageObjPtr: Long, name: String): Long
    private external fun nativePageObjRemoveMark(pageObjPtr: Long, markPtr: Long): Boolean
    private external fun nativePageObjGetMarkedContentID(pageObjPtr: Long): Int
    private external fun nativePageObjSetBlendMode(pageObjPtr: Long, blendMode: Int): Boolean
    private external fun nativePathCountSegments(pathObjPtr: Long): Int
    private external fun nativePathGetPathSegment(pathObjPtr: Long, index: Int): Long
    private external fun nativePathGetDrawMode(pathObjPtr: Long): Int
    private external fun nativeTextObjGetFont(textObjPtr: Long): Long
    private external fun nativeTextObjGetFontSize(textObjPtr: Long): Double
    private external fun nativeTextObjGetText(docPtr: Long, pagePtr: Long, textObjPtr: Long): String?
    private external fun nativeTextObjGetTextRenderMode(textObjPtr: Long): Int
    private external fun nativeTextObjSetTextRenderMode(textObjPtr: Long, renderMode: Int): Boolean
    private external fun nativeImageObjSetMatrix(imageObjPtr: Long, a: Float, b: Float, c: Float, d: Float, e: Float, f: Float): Boolean
    private external fun nativeImageObjGetImageFilterCount(imageObjPtr: Long): Int
    private external fun nativeImageObjGetImageFilter(imageObjPtr: Long, index: Int): String?
    private external fun nativeImageObjGetImagePixelSize(imageObjPtr: Long, size: IntArray): Boolean
    private external fun nativeImageObjGetImageDataDecoded(imageObjPtr: Long): ByteArray?
    private external fun nativeImageObjGetImageDataRaw(imageObjPtr: Long): ByteArray?
    private external fun nativeImageObjGetBitmap(imageObjPtr: Long): Long
    private external fun nativeImageObjSetBitmap(imageObjPtr: Long, width: Int, height: Int, stride: Int, pixels: IntArray): Boolean
    private external fun nativeImageObjGetRenderedBitmap(docPtr: Long, pagePtr: Long, imageObjPtr: Long): Long
    private external fun nativeImageObjLoadJpegFile(imageObjPtr: Long, pagePtr: Long, jpegData: ByteArray): Boolean
    private external fun nativeImageObjLoadJpegFileInline(imageObjPtr: Long, pagePtr: Long, jpegData: ByteArray): Boolean
    private external fun nativeImageObjGetIccProfileDataDecoded(imageObjPtr: Long): ByteArray?
    private external fun nativeImageObjGetImageMetadata(imageObjPtr: Long, pagePtr: Long, intValues: IntArray, floatValues: FloatArray): Boolean

    // Phase 9: Document Utilities
    private external fun nativeImportPages(destDocPtr: Long, srcDocPtr: Long, pageRange: String?, insertIndex: Int): Boolean
    private external fun nativeCopyViewerPreferences(destDocPtr: Long, srcDocPtr: Long): Boolean
    private external fun nativeImportPagesByIndex(destDocPtr: Long, srcDocPtr: Long, pageIndices: IntArray, insertIndex: Int): Boolean
    private external fun nativeImportNPagesToOne(srcDocPtr: Long, outputWidth: Float, outputHeight: Float, numPagesX: Int, numPagesY: Int): Long
    private external fun nativeNewXObjectFromPage(destDocPtr: Long, srcDocPtr: Long, srcPageIndex: Int): Long
    private external fun nativeCloseXObject(xobjectPtr: Long)
    private external fun nativeNewFormObjectFromXObject(xobjectPtr: Long): Long
    private external fun nativeFlattenPage(pagePtr: Long, flags: Int): Int
    private external fun nativeSetPageMediaBox(pagePtr: Long, left: Float, bottom: Float, right: Float, top: Float): Boolean
    private external fun nativeSetPageCropBox(pagePtr: Long, left: Float, bottom: Float, right: Float, top: Float): Boolean
    private external fun nativeGetPageMediaBox(pagePtr: Long, result: FloatArray): Boolean
    private external fun nativeGetPageCropBox(pagePtr: Long, result: FloatArray): Boolean
    private external fun nativeGetPageRotation(pagePtr: Long): Int
    private external fun nativeSetPageRotation(pagePtr: Long, rotation: Int)
    private external fun nativeDeletePage(docPtr: Long, pageIndex: Int)

    // Phase 10: Thumbnails, StructTree
    private external fun nativeGetDecodedThumbnailData(pagePtr: Long): ByteArray?
    private external fun nativeGetRawThumbnailData(pagePtr: Long): ByteArray?
    private external fun nativeGetThumbnailAsBitmap(pagePtr: Long): Long
    private external fun nativeGetStructTreeForPage(pagePtr: Long): Long
    private external fun nativeCloseStructTree(structTreePtr: Long)
    private external fun nativeStructTreeCountChildren(structTreePtr: Long): Int
    private external fun nativeStructTreeGetChildAtIndex(structTreePtr: Long, index: Int): Long
    private external fun nativeStructElementGetType(structElemPtr: Long): String?
    private external fun nativeStructElementGetAltText(structElemPtr: Long): String?
    private external fun nativeGetStructElementExpansion(structElemPtr: Long): String?
    
    // Phase 11: Signatures, JS
    private external fun nativeGetSignatureCount(docPtr: Long): Int
    private external fun nativeGetSignatureObject(docPtr: Long, index: Int): Long
    private external fun nativeGetSignatureContents(sigObjPtr: Long): ByteArray?
    private external fun nativeGetSignatureReason(sigObjPtr: Long): String?
    private external fun nativeGetSignatureTime(sigObjPtr: Long): String?
    private external fun nativeGetSignatureByteRange(sigObjPtr: Long): IntArray?
    private external fun nativeGetSignatureSubFilter(sigObjPtr: Long): String?
    private external fun nativeGetSignatureDocMDPPermission(sigObjPtr: Long): Int
    private external fun nativeGetJavaScriptActionCount(docPtr: Long): Int
    private external fun nativeGetJavaScriptAction(docPtr: Long, index: Int): Long
    private external fun nativeCloseJavaScriptAction(jsActionPtr: Long)
    private external fun nativeGetJavaScriptActionName(jsActionPtr: Long): String?
    private external fun nativeGetJavaScriptActionScript(jsActionPtr: Long): String?

    // Phase 12a: fpdf_ext.h
    private external fun nativeSetUnSpObjProcessHandler(): Boolean

    // Phase 12b: fpdf_dataavail.h
    private external fun nativeCreateAvail(fileData: ByteArray): Long
    private external fun nativeDestroyAvail(availPtr: Long)
    private external fun nativeAvailGetDocument(availPtr: Long, password: String?): Long
    private external fun nativeAvailGetFirstPageNum(docPtr: Long): Int
    private external fun nativeAvailIsDocAvail(availPtr: Long): Int
    private external fun nativeAvailIsPageAvail(availPtr: Long, pageIndex: Int): Int
    private external fun nativeAvailIsFormAvail(availPtr: Long): Int

    // Phase 12c: fpdf_sysfontinfo.h
    private external fun nativeGetDefaultTTFMapCount(): Int
    private external fun nativeGetDefaultTTFMapEntry(index: Int, outCharset: IntArray): String?
    private external fun nativeAddInstalledFont(mapperPtr: Long, face: String, charset: Int)
    private external fun nativeGetDefaultSystemFontInfo(): Long
    private external fun nativeFreeDefaultSystemFontInfo(fontInfoPtr: Long)
    private external fun nativeSetSystemFontInfo(fontInfoPtr: Long)

    private external fun nativeLoadCustomDocument(data: ByteArray, password: String?): Long

    // Phase 12: WebLinks, Enums, etc.
    private external fun nativeLoadWebLinks(textPagePtr: Long): Long
    private external fun nativeCloseWebLinks(pageLinksPtr: Long)
    private external fun nativeCountWebLinks(pageLinksPtr: Long): Int
    private external fun nativeGetWebLinkURL(pageLinksPtr: Long, index: Int): String?
    private external fun nativeGetFormType(docPtr: Long): Int
    private external fun nativeGetPageMode(docPtr: Long): Int
    private external fun nativeGetDocPermissions(docPtr: Long): Long
    private external fun nativeTransformPageObj(pageObjPtr: Long, a: Double, b: Double, c: Double, d: Double, e: Double, f: Double)
    private external fun nativeGetPageObjBounds(pageObjPtr: Long, result: FloatArray): Boolean
    private external fun nativeRemoveAnnot(pagePtr: Long, index: Int): Boolean

    // ===== Exposed Helper Functions for Phases 8-12 =====
    
    // Page Editing
    fun newTextObject(docPtr: Long, fontName: String, fontSize: Float): Long = nativeNewTextObj(docPtr, fontName, fontSize)
    fun setTextObjectText(textObjPtr: Long, text: String): Boolean = nativeSetTextObjText(textObjPtr, text)
    fun createNewPath(x: Float, y: Float): Long = nativeCreateNewPath(x, y)
    fun pathMoveTo(pathObjPtr: Long, x: Float, y: Float): Boolean = nativePathMoveTo(pathObjPtr, x, y)
    fun pathLineTo(pathObjPtr: Long, x: Float, y: Float): Boolean = nativePathLineTo(pathObjPtr, x, y)
    fun pathBezierTo(pathObjPtr: Long, x1: Float, y1: Float, x2: Float, y2: Float, x3: Float, y3: Float): Boolean = nativePathBezierTo(pathObjPtr, x1, y1, x2, y2, x3, y3)
    fun pathSetDrawMode(pathObjPtr: Long, fillMode: Int, stroke: Boolean): Boolean = nativePathSetDrawMode(pathObjPtr, fillMode, stroke)
    fun pathSetStrokeWidth(pathObjPtr: Long, width: Float): Boolean = nativePathSetStrokeWidth(pathObjPtr, width)
    fun pathClose(pathObjPtr: Long): Boolean = nativePathClose(pathObjPtr)
    fun newImageObject(docPtr: Long): Long = nativeNewImageObj(docPtr)
    fun insertObject(pagePtr: Long, pageObjPtr: Long): Boolean = nativeInsertObject(pagePtr, pageObjPtr)
    fun removeObject(pagePtr: Long, pageObjPtr: Long): Boolean = nativeRemoveObject(pagePtr, pageObjPtr)
    fun setObjectFillColor(pageObjPtr: Long, r: Int, g: Int, b: Int, a: Int) = nativeSetObjectFillColor(pageObjPtr, r, g, b, a)
    fun setObjectStrokeColor(pageObjPtr: Long, r: Int, g: Int, b: Int, a: Int) = nativeSetObjectStrokeColor(pageObjPtr, r, g, b, a)
    fun generateContent(pagePtr: Long) = nativeGenerateContent(pagePtr)
    fun addExistingMark(pageObjPtr: Long, markPtr: Long): Boolean = nativeAddExistingMark(pageObjPtr, markPtr)
    fun setTextObjectFontSize(textObjPtr: Long, size: Float): Boolean = nativeSetTextObjFontSize(textObjPtr, size)
    fun setTextPositions(textObjPtr: Long, positions: FloatArray): Boolean = nativeSetTextPositions(textObjPtr, positions)

    // Additional Page Object Functions
    fun pageObjCreateNew(docPtr: Long): Long = nativePageObjCreateNew(docPtr)
    fun pageObjCreateNewRect(docPtr: Long): Long = nativePageObjCreateNewRect(docPtr)
    fun pageObjCreateTextObj(docPtr: Long, fontName: String): Long = nativePageObjCreateTextObj(docPtr, fontName)
    fun pageObjDestroy(pageObjPtr: Long) = nativePageObjDestroy(pageObjPtr)
    fun pageObjHasTransparency(pageObjPtr: Long): Boolean = nativePageObjHasTransparency(pageObjPtr)
    fun pageObjGetMatrix(pageObjPtr: Long, matrix: FloatArray): Boolean = nativePageObjGetMatrix(pageObjPtr, matrix)
    fun pageObjSetMatrix(pageObjPtr: Long, matrix: FloatArray): Boolean = nativePageObjSetMatrix(pageObjPtr, matrix)
    fun pageObjTransformF(pageObjPtr: Long, a: Float, b: Float, c: Float, d: Float, e: Float, f: Float): Boolean = nativePageObjTransformF(pageObjPtr, a, b, c, d, e, f)
    fun pageObjGetRotatedBounds(pageObjPtr: Long, rect: FloatArray): Boolean = nativePageObjGetRotatedBounds(pageObjPtr, rect)
    fun pageObjGetLineCap(pageObjPtr: Long): Int = nativePageObjGetLineCap(pageObjPtr)
    fun pageObjSetLineCap(pageObjPtr: Long, lineCap: Int): Boolean = nativePageObjSetLineCap(pageObjPtr, lineCap)
    fun pageObjGetLineJoin(pageObjPtr: Long): Int = nativePageObjGetLineJoin(pageObjPtr)
    fun pageObjSetLineJoin(pageObjPtr: Long, lineJoin: Int): Boolean = nativePageObjSetLineJoin(pageObjPtr, lineJoin)
    fun pageObjGetStrokeWidth(pageObjPtr: Long): Float = nativePageObjGetStrokeWidth(pageObjPtr)
    fun pageObjGetDashPhase(pageObjPtr: Long, phase: FloatArray): Boolean = nativePageObjGetDashPhase(pageObjPtr, phase)
    fun pageObjSetDashPhase(pageObjPtr: Long, phase: Float): Boolean = nativePageObjSetDashPhase(pageObjPtr, phase)
    fun pageObjGetDashCount(pageObjPtr: Long): Int = nativePageObjGetDashCount(pageObjPtr)
    fun pageObjGetDashArray(pageObjPtr: Long, dashes: FloatArray): Boolean = nativePageObjGetDashArray(pageObjPtr, dashes)
    fun pageObjSetDashArray(pageObjPtr: Long, dashes: FloatArray): Boolean = nativePageObjSetDashArray(pageObjPtr, dashes)
    fun pageObjGetIsActive(pageObjPtr: Long): Boolean = nativePageObjGetIsActive(pageObjPtr)
    fun pageObjSetIsActive(pageObjPtr: Long, active: Boolean): Boolean = nativePageObjSetIsActive(pageObjPtr, active)
    fun pageObjCountMarks(pageObjPtr: Long): Int = nativePageObjCountMarks(pageObjPtr)
    fun pageObjGetMark(pageObjPtr: Long, index: Int): Long = nativePageObjGetMark(pageObjPtr, index)
    fun pageObjAddMark(pageObjPtr: Long, name: String): Long = nativePageObjAddMark(pageObjPtr, name)
    fun pageObjRemoveMark(pageObjPtr: Long, markPtr: Long): Boolean = nativePageObjRemoveMark(pageObjPtr, markPtr)
    fun pageObjGetMarkedContentID(pageObjPtr: Long): Int = nativePageObjGetMarkedContentID(pageObjPtr)
    fun pageObjSetBlendMode(pageObjPtr: Long, blendMode: Int): Boolean = nativePageObjSetBlendMode(pageObjPtr, blendMode)
    fun pathCountSegments(pathObjPtr: Long): Int = nativePathCountSegments(pathObjPtr)
    fun pathGetPathSegment(pathObjPtr: Long, index: Int): Long = nativePathGetPathSegment(pathObjPtr, index)
    fun pathGetDrawMode(pathObjPtr: Long): Int = nativePathGetDrawMode(pathObjPtr)
    fun textObjGetFont(textObjPtr: Long): Long = nativeTextObjGetFont(textObjPtr)
    fun textObjGetFontSize(textObjPtr: Long): Double = nativeTextObjGetFontSize(textObjPtr)
    fun textObjGetText(docPtr: Long, pagePtr: Long, textObjPtr: Long): String? = nativeTextObjGetText(docPtr, pagePtr, textObjPtr)
    fun textObjGetTextRenderMode(textObjPtr: Long): Int = nativeTextObjGetTextRenderMode(textObjPtr)
    fun textObjSetTextRenderMode(textObjPtr: Long, renderMode: Int): Boolean = nativeTextObjSetTextRenderMode(textObjPtr, renderMode)
    fun imageObjSetMatrix(imageObjPtr: Long, a: Float, b: Float, c: Float, d: Float, e: Float, f: Float): Boolean = nativeImageObjSetMatrix(imageObjPtr, a, b, c, d, e, f)
    fun imageObjGetImageFilterCount(imageObjPtr: Long): Int = nativeImageObjGetImageFilterCount(imageObjPtr)
    fun imageObjGetImageFilter(imageObjPtr: Long, index: Int): String? = nativeImageObjGetImageFilter(imageObjPtr, index)
    fun imageObjGetImagePixelSize(imageObjPtr: Long): IntArray? {
        val size = IntArray(2)
        return if (nativeImageObjGetImagePixelSize(imageObjPtr, size)) size else null
    }
    fun imageObjGetImageDataDecoded(imageObjPtr: Long): ByteArray? = nativeImageObjGetImageDataDecoded(imageObjPtr)
    fun imageObjGetImageDataRaw(imageObjPtr: Long): ByteArray? = nativeImageObjGetImageDataRaw(imageObjPtr)
    fun imageObjGetBitmap(imageObjPtr: Long): Long = nativeImageObjGetBitmap(imageObjPtr)
    fun imageObjSetBitmap(imageObjPtr: Long, width: Int, height: Int, stride: Int, pixels: IntArray): Boolean = nativeImageObjSetBitmap(imageObjPtr, width, height, stride, pixels)
    fun imageObjGetRenderedBitmap(docPtr: Long, pagePtr: Long, imageObjPtr: Long): Long = nativeImageObjGetRenderedBitmap(docPtr, pagePtr, imageObjPtr)
    fun imageObjGetIccProfileDataDecoded(imageObjPtr: Long): ByteArray? = nativeImageObjGetIccProfileDataDecoded(imageObjPtr)
    fun imageObjLoadJpegFile(imageObjPtr: Long, pagePtr: Long, jpegData: ByteArray): Boolean =
        nativeImageObjLoadJpegFile(imageObjPtr, pagePtr, jpegData)
    fun imageObjLoadJpegFileInline(imageObjPtr: Long, pagePtr: Long, jpegData: ByteArray): Boolean =
        nativeImageObjLoadJpegFileInline(imageObjPtr, pagePtr, jpegData)

    data class ImageMetadata(
        val width: Int, val height: Int,
        val horizontalDpi: Float, val verticalDpi: Float,
        val bitsPerPixel: Int, val colorspace: Int, val markedContentId: Int
    )
    fun imageObjGetImageMetadata(imageObjPtr: Long, pagePtr: Long): ImageMetadata? {
        val intValues = IntArray(5)
        val floatValues = FloatArray(2)
        return if (nativeImageObjGetImageMetadata(imageObjPtr, pagePtr, intValues, floatValues)) {
            ImageMetadata(
                width = intValues[0], height = intValues[1],
                horizontalDpi = floatValues[0], verticalDpi = floatValues[1],
                bitsPerPixel = intValues[2], colorspace = intValues[3],
                markedContentId = intValues[4]
            )
        } else null
    }

    // fpdf_ext.h
    fun setUnSpObjProcessHandler(): Boolean = nativeSetUnSpObjProcessHandler()

    // fpdf_dataavail.h
    fun createAvail(fileData: ByteArray): Long = nativeCreateAvail(fileData)
    fun destroyAvail(availPtr: Long) = nativeDestroyAvail(availPtr)
    fun availGetDocument(availPtr: Long, password: String? = null): Long =
        nativeAvailGetDocument(availPtr, password)
    fun availGetFirstPageNum(docPtr: Long): Int = nativeAvailGetFirstPageNum(docPtr)
    fun availIsDocAvail(availPtr: Long): Int = nativeAvailIsDocAvail(availPtr)
    fun availIsPageAvail(availPtr: Long, pageIndex: Int): Int = nativeAvailIsPageAvail(availPtr, pageIndex)
    fun availIsFormAvail(availPtr: Long): Int = nativeAvailIsFormAvail(availPtr)

    // fpdf_sysfontinfo.h
    fun getDefaultTTFMapCount(): Int = nativeGetDefaultTTFMapCount()
    data class TTFMapEntry(val charset: Int, val fontName: String)
    fun getDefaultTTFMapEntry(index: Int): TTFMapEntry? {
        val charset = IntArray(1)
        val fontName = nativeGetDefaultTTFMapEntry(index, charset)
        return fontName?.let { TTFMapEntry(charset[0], it) }
    }
    fun getDefaultTTFMap(): List<TTFMapEntry> {
        val count = nativeGetDefaultTTFMapCount()
        val result = mutableListOf<TTFMapEntry>()
        for (i in 0 until count) {
            val entry = getDefaultTTFMapEntry(i) ?: continue
            result.add(entry)
        }
        return result
    }

    fun addInstalledFont(mapperPtr: Long, face: String, charset: Int) =
        nativeAddInstalledFont(mapperPtr, face, charset)
    fun getDefaultSystemFontInfo(): Long = nativeGetDefaultSystemFontInfo()
    fun freeDefaultSystemFontInfo(fontInfoPtr: Long) = nativeFreeDefaultSystemFontInfo(fontInfoPtr)
    fun setSystemFontInfo(fontInfoPtr: Long) = nativeSetSystemFontInfo(fontInfoPtr)

    // fpdfview.h — FPDF_LoadCustomDocument
    fun loadCustomDocument(data: ByteArray, password: String? = null): PdfDocument? {
        val docPtr = nativeLoadCustomDocument(data, password)
        return if (docPtr != 0L) PdfDocument(this, docPtr) else null
    }

    // Document Utilities
    fun importPages(destDocPtr: Long, srcDocPtr: Long, pageRange: String?, insertIndex: Int): Boolean = nativeImportPages(destDocPtr, srcDocPtr, pageRange, insertIndex)
    fun copyViewerPreferences(destDocPtr: Long, srcDocPtr: Long): Boolean = nativeCopyViewerPreferences(destDocPtr, srcDocPtr)
    fun importPagesByIndex(destDocPtr: Long, srcDocPtr: Long, pageIndices: IntArray, insertIndex: Int): Boolean = nativeImportPagesByIndex(destDocPtr, srcDocPtr, pageIndices, insertIndex)
    fun importNPagesToOne(srcDocPtr: Long, outputWidth: Float, outputHeight: Float, numPagesX: Int, numPagesY: Int): Long = nativeImportNPagesToOne(srcDocPtr, outputWidth, outputHeight, numPagesX, numPagesY)
    fun newXObjectFromPage(destDocPtr: Long, srcDocPtr: Long, srcPageIndex: Int): Long = nativeNewXObjectFromPage(destDocPtr, srcDocPtr, srcPageIndex)
    fun closeXObject(xobjectPtr: Long) = nativeCloseXObject(xobjectPtr)
    fun newFormObjectFromXObject(xobjectPtr: Long): Long = nativeNewFormObjectFromXObject(xobjectPtr)
    fun flattenPage(pagePtr: Long, flags: Int = 0): Int = nativeFlattenPage(pagePtr, flags)
    fun getPageRotation(pagePtr: Long): Int = nativeGetPageRotation(pagePtr)
    fun setPageRotation(pagePtr: Long, rotation: Int) = nativeSetPageRotation(pagePtr, rotation)
    fun deletePage(docPtr: Long, pageIndex: Int) = nativeDeletePage(docPtr, pageIndex)

    // Thumbnails
    fun getDecodedThumbnailData(pagePtr: Long): ByteArray? = nativeGetDecodedThumbnailData(pagePtr)
    fun getRawThumbnailData(pagePtr: Long): ByteArray? = nativeGetRawThumbnailData(pagePtr)
    fun getThumbnailAsBitmap(pagePtr: Long): Long = nativeGetThumbnailAsBitmap(pagePtr)

    // StructTree
    fun getStructTreeForPage(pagePtr: Long): Long = nativeGetStructTreeForPage(pagePtr)
    fun closeStructTree(structTreePtr: Long) = nativeCloseStructTree(structTreePtr)
    fun structTreeCountChildren(structTreePtr: Long): Int = nativeStructTreeCountChildren(structTreePtr)
    fun structTreeGetChildAtIndex(structTreePtr: Long, index: Int): Long = nativeStructTreeGetChildAtIndex(structTreePtr, index)
    fun structElementGetType(structElemPtr: Long): String = nativeStructElementGetType(structElemPtr) ?: ""
    fun structElementGetAltText(structElemPtr: Long): String = nativeStructElementGetAltText(structElemPtr) ?: ""
    fun getStructElementExpansion(structElemPtr: Long): String = nativeGetStructElementExpansion(structElemPtr) ?: ""
    fun structElementGetActualText(structElemPtr: Long): String = nativeStructElementGetActualText(structElemPtr) ?: ""
    fun structElementGetID(structElemPtr: Long): String = nativeStructElementGetID(structElemPtr) ?: ""
    fun structElementGetLang(structElemPtr: Long): String = nativeStructElementGetLang(structElemPtr) ?: ""
    fun structElementGetStringAttribute(structElemPtr: Long, attrName: String): String = nativeStructElementGetStringAttribute(structElemPtr, attrName) ?: ""
    fun structElementGetMarkedContentID(structElemPtr: Long): Int = nativeStructElementGetMarkedContentID(structElemPtr)
    fun structElementGetObjType(structElemPtr: Long): String = nativeStructElementGetObjType(structElemPtr) ?: ""
    fun structElementGetTitle(structElemPtr: Long): String = nativeStructElementGetTitle(structElemPtr) ?: ""
    fun structElementGetChildMarkedContentID(structElemPtr: Long, index: Int): Int = nativeStructElementGetChildMarkedContentID(structElemPtr, index)
    fun structElementGetParent(structElemPtr: Long): Long = nativeStructElementGetParent(structElemPtr)
    fun structElementGetAttributeCount(structElemPtr: Long): Int = nativeStructElementGetAttributeCount(structElemPtr)
    fun structElementGetAttributeAtIndex(structElemPtr: Long, index: Int): Long = nativeStructElementGetAttributeAtIndex(structElemPtr, index)
    fun structElementAttrGetCount(attrPtr: Long): Int = nativeStructElementAttrGetCount(attrPtr)
    fun structElementAttrGetName(attrPtr: Long, index: Int): String = nativeStructElementAttrGetName(attrPtr, index) ?: ""
    fun structElementAttrGetValue(attrPtr: Long, name: String): Long = nativeStructElementAttrGetValue(attrPtr, name)
    fun structElementAttrGetType(valuePtr: Long): Int = nativeStructElementAttrGetType(valuePtr)
    fun structElementAttrGetBooleanValue(valuePtr: Long, outValue: BooleanArray): Boolean = nativeStructElementAttrGetBooleanValue(valuePtr, outValue)
    fun structElementAttrGetNumberValue(valuePtr: Long, outValue: FloatArray): Boolean = nativeStructElementAttrGetNumberValue(valuePtr, outValue)
    fun structElementAttrGetStringValue(valuePtr: Long): Boolean = nativeStructElementAttrGetStringValue(valuePtr)
    fun structElementAttrGetBlobValue(valuePtr: Long): ByteArray? = nativeStructElementAttrGetBlobValue(valuePtr)
    fun structElementAttrCountChildren(valuePtr: Long): Int = nativeStructElementAttrCountChildren(valuePtr)
    fun structElementAttrGetChildAtIndex(valuePtr: Long, index: Int): Long = nativeStructElementAttrGetChildAtIndex(valuePtr, index)
    fun structElementGetMarkedContentIdCount(structElemPtr: Long): Int = nativeStructElementGetMarkedContentIdCount(structElemPtr)
    fun structElementGetMarkedContentIdAtIndex(structElemPtr: Long, index: Int): Int = nativeStructElementGetMarkedContentIdAtIndex(structElemPtr, index)

    // Signatures
    fun getSignatureCount(docPtr: Long): Int = nativeGetSignatureCount(docPtr)
    fun getSignatureObject(docPtr: Long, index: Int): Long = nativeGetSignatureObject(docPtr, index)
    fun getSignatureContents(sigObjPtr: Long): ByteArray? = nativeGetSignatureContents(sigObjPtr)
    fun getSignatureReason(sigObjPtr: Long): String = nativeGetSignatureReason(sigObjPtr) ?: ""
    fun getSignatureTime(sigObjPtr: Long): String = nativeGetSignatureTime(sigObjPtr) ?: ""
    fun getSignatureByteRange(sigObjPtr: Long): IntArray? = nativeGetSignatureByteRange(sigObjPtr)
    fun getSignatureSubFilter(sigObjPtr: Long): String = nativeGetSignatureSubFilter(sigObjPtr) ?: ""
    fun getSignatureDocMDPPermission(sigObjPtr: Long): Int = nativeGetSignatureDocMDPPermission(sigObjPtr)
    fun getJavaScriptActionCount(docPtr: Long): Int = nativeGetJavaScriptActionCount(docPtr)
    fun getJavaScriptAction(docPtr: Long, index: Int): Long = nativeGetJavaScriptAction(docPtr, index)
    fun closeJavaScriptAction(jsActionPtr: Long) = nativeCloseJavaScriptAction(jsActionPtr)
    fun getJavaScriptActionName(jsActionPtr: Long): String = nativeGetJavaScriptActionName(jsActionPtr) ?: ""
    fun getJavaScriptActionScript(jsActionPtr: Long): String = nativeGetJavaScriptActionScript(jsActionPtr) ?: ""

    // WebLinks
    fun loadWebLinks(textPagePtr: Long): Long = nativeLoadWebLinks(textPagePtr)
    fun closeWebLinks(pageLinksPtr: Long) = nativeCloseWebLinks(pageLinksPtr)
    fun countWebLinks(pageLinksPtr: Long): Int = nativeCountWebLinks(pageLinksPtr)
    fun getWebLinkURL(pageLinksPtr: Long, index: Int): String = nativeGetWebLinkURL(pageLinksPtr, index) ?: ""

    // Document Info
    fun getFormType(docPtr: Long): Int = nativeGetFormType(docPtr)
    fun getPageMode(docPtr: Long): Int = nativeGetPageMode(docPtr)
    fun getDocPermissions(docPtr: Long): Long = nativeGetDocPermissions(docPtr)

    // Object Transform
    fun transformPageObject(pageObjPtr: Long, a: Double, b: Double, c: Double, d: Double, e: Double, f: Double) = nativeTransformPageObj(pageObjPtr, a, b, c, d, e, f)
    fun getPageObjectBounds(pageObjPtr: Long): FloatArray? {
        val result = FloatArray(4)
        return if (nativeGetPageObjBounds(pageObjPtr, result)) result else null
    }

    // Annotation Remove
    fun removeAnnotation(pagePtr: Long, index: Int): Boolean = nativeRemoveAnnot(pagePtr, index)

    // Progressive Rendering
    private external fun nativeRenderPageBitmapStart(
        bitmap: Any, pagePtr: Long, startX: Int, startY: Int, 
        drawWidth: Int, drawHeight: Int, rotate: Int, flags: Int
    ): Int
    private external fun nativeRenderPageContinue(pagePtr: Long): Int
    private external fun nativeRenderPageClose(pagePtr: Long)
    private external fun nativeRenderPageBitmapWithColorSchemeStart(
        bitmapPtr: Long, pagePtr: Long,
        startX: Int, startY: Int, sizeX: Int, sizeY: Int,
        rotate: Int, flags: Int, colorScheme: IntArray
    ): Int

    /**
     * Start progressive rendering of a page to a bitmap.
     * @return Render status: RENDER_READY(0), RENDER_TOBECONTINUED(1), RENDER_DONE(2), RENDER_FAILED(3)
     */
    fun renderPageBitmapStart(
        bitmap: Any, pagePtr: Long, startX: Int, startY: Int,
        drawWidth: Int, drawHeight: Int, rotate: Int = 0, flags: Int = 0
    ): Int = nativeRenderPageBitmapStart(bitmap, pagePtr, startX, startY, drawWidth, drawHeight, rotate, flags)

    /**
     * Continue progressive rendering.
     * @return Render status
     */
    fun renderPageContinue(pagePtr: Long): Int = nativeRenderPageContinue(pagePtr)

    /**
     * Close/release resources for progressive rendering.
     */
    fun renderPageClose(pagePtr: Long) = nativeRenderPageClose(pagePtr)

    /**
     * Start progressive rendering with a custom color scheme.
     * @param colorScheme Array of 4 ARGB ints: [pathFill, pathStroke, textFill, textStroke]
     */
    fun renderPageBitmapWithColorSchemeStart(bitmapPtr: Long, pagePtr: Long,
                                             startX: Int, startY: Int, sizeX: Int, sizeY: Int,
                                             rotate: Int, flags: Int, colorScheme: IntArray): Int =
        nativeRenderPageBitmapWithColorSchemeStart(bitmapPtr, pagePtr, startX, startY, sizeX, sizeY, rotate, flags, colorScheme)

    // =========================================================================
    // COMPLETE IMPLEMENTATION - ALL REMAINING FEATURES
    // =========================================================================

    // --- Form Events ---
    private external fun nativeFormOnMouseMove(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double): Boolean
    private external fun nativeFormOnLButtonDown(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double): Boolean
    private external fun nativeFormOnLButtonUp(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double): Boolean
    private external fun nativeFormOnKeyDown(formPtr: Long, pagePtr: Long, keyCode: Int, modifier: Int): Boolean
    private external fun nativeFormOnKeyUp(formPtr: Long, pagePtr: Long, keyCode: Int, modifier: Int): Boolean
    private external fun nativeFormOnChar(formPtr: Long, pagePtr: Long, charCode: Int, modifier: Int): Boolean
    private external fun nativeFormOnFocus(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double): Boolean
    private external fun nativeFormCanUndo(formPtr: Long, pagePtr: Long): Boolean
    private external fun nativeFormCanRedo(formPtr: Long, pagePtr: Long): Boolean
    private external fun nativeFormUndo(formPtr: Long, pagePtr: Long): Boolean
    private external fun nativeFormRedo(formPtr: Long, pagePtr: Long): Boolean
    private external fun nativeFormSelectAllText(formPtr: Long, pagePtr: Long)
    private external fun nativeFormForceToKillFocus(formPtr: Long)
    private external fun nativeFormDoDocumentJSAction(formPtr: Long)
    private external fun nativeFormDoDocumentOpenAction(formPtr: Long)
    private external fun nativeFormDoDocumentAAction(formPtr: Long, aaType: Int)
    private external fun nativeFormDoPageAAction(formPtr: Long, pagePtr: Long, aaType: Int)
    private external fun nativeFormOnMouseWheel(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double, deltaX: Int, deltaY: Int): Boolean
    private external fun nativeFormOnRButtonDown(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double): Boolean
    private external fun nativeFormOnRButtonUp(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double): Boolean
    private external fun nativeFormOnLButtonDoubleClick(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double): Boolean
    private external fun nativeFormGetFocusedText(formPtr: Long, pagePtr: Long): String?
    private external fun nativeFormGetSelectedText(formPtr: Long, pagePtr: Long): String?
    private external fun nativeFormReplaceAndKeepSelection(formPtr: Long, pagePtr: Long, text: String)
    private external fun nativeFormReplaceSelection(formPtr: Long, pagePtr: Long, text: String)
    private external fun nativeFormGetFocusedAnnot(formPtr: Long, pageIndex: IntArray, annotPtr: LongArray): Boolean
    private external fun nativeFormSetFocusedAnnot(formPtr: Long, annotPtr: Long): Boolean
    private external fun nativePageHasFormFieldAtPoint(formPtr: Long, pagePtr: Long, x: Double, y: Double): Int
    private external fun nativePageFormFieldZOrderAtPoint(formPtr: Long, pagePtr: Long, x: Double, y: Double): Int
    private external fun nativeSetFormFieldHighlightColor(formPtr: Long, fieldType: Int, color: Long)
    private external fun nativeSetFormFieldHighlightAlpha(formPtr: Long, alpha: Int)
    private external fun nativeRemoveFormFieldHighlight(formPtr: Long)
    private external fun nativeFormSetIndexSelected(formPtr: Long, pagePtr: Long, index: Int, selected: Boolean): Boolean
    private external fun nativeFormIsIndexSelected(formPtr: Long, pagePtr: Long, index: Int): Boolean
    private external fun nativeLoadXFA(docPtr: Long): Boolean

    fun formOnMouseMove(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double) = nativeFormOnMouseMove(formPtr, pagePtr, modifier, x, y)
    fun formOnLButtonDown(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double) = nativeFormOnLButtonDown(formPtr, pagePtr, modifier, x, y)
    fun formOnLButtonUp(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double) = nativeFormOnLButtonUp(formPtr, pagePtr, modifier, x, y)
    fun formOnKeyDown(formPtr: Long, pagePtr: Long, keyCode: Int, modifier: Int) = nativeFormOnKeyDown(formPtr, pagePtr, keyCode, modifier)
    fun formOnKeyUp(formPtr: Long, pagePtr: Long, keyCode: Int, modifier: Int) = nativeFormOnKeyUp(formPtr, pagePtr, keyCode, modifier)
    fun formOnChar(formPtr: Long, pagePtr: Long, charCode: Int, modifier: Int) = nativeFormOnChar(formPtr, pagePtr, charCode, modifier)
    fun formOnFocus(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double) = nativeFormOnFocus(formPtr, pagePtr, modifier, x, y)
    fun formCanUndo(formPtr: Long, pagePtr: Long) = nativeFormCanUndo(formPtr, pagePtr)
    fun formCanRedo(formPtr: Long, pagePtr: Long) = nativeFormCanRedo(formPtr, pagePtr)
    fun formUndo(formPtr: Long, pagePtr: Long) = nativeFormUndo(formPtr, pagePtr)
    fun formRedo(formPtr: Long, pagePtr: Long) = nativeFormRedo(formPtr, pagePtr)
    fun formSelectAllText(formPtr: Long, pagePtr: Long) = nativeFormSelectAllText(formPtr, pagePtr)
    fun formForceToKillFocus(formPtr: Long) = nativeFormForceToKillFocus(formPtr)
    fun formDoDocumentJSAction(formPtr: Long) = nativeFormDoDocumentJSAction(formPtr)
    fun formDoDocumentOpenAction(formPtr: Long) = nativeFormDoDocumentOpenAction(formPtr)
    fun formDoDocumentAAction(formPtr: Long, aaType: Int) = nativeFormDoDocumentAAction(formPtr, aaType)
    fun formDoPageAAction(formPtr: Long, pagePtr: Long, aaType: Int) = nativeFormDoPageAAction(formPtr, pagePtr, aaType)
    fun formOnMouseWheel(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double, deltaX: Int, deltaY: Int): Boolean = nativeFormOnMouseWheel(formPtr, pagePtr, modifier, x, y, deltaX, deltaY)
    fun formOnRButtonDown(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double): Boolean = nativeFormOnRButtonDown(formPtr, pagePtr, modifier, x, y)
    fun formOnRButtonUp(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double): Boolean = nativeFormOnRButtonUp(formPtr, pagePtr, modifier, x, y)
    fun formOnLButtonDoubleClick(formPtr: Long, pagePtr: Long, modifier: Int, x: Double, y: Double): Boolean = nativeFormOnLButtonDoubleClick(formPtr, pagePtr, modifier, x, y)
    fun formGetFocusedText(formPtr: Long, pagePtr: Long): String? = nativeFormGetFocusedText(formPtr, pagePtr)
    fun formGetSelectedText(formPtr: Long, pagePtr: Long): String? = nativeFormGetSelectedText(formPtr, pagePtr)
    fun formReplaceAndKeepSelection(formPtr: Long, pagePtr: Long, text: String) = nativeFormReplaceAndKeepSelection(formPtr, pagePtr, text)
    fun formReplaceSelection(formPtr: Long, pagePtr: Long, text: String) = nativeFormReplaceSelection(formPtr, pagePtr, text)
    fun formGetFocusedAnnot(formPtr: Long): Pair<Int, Long>? {
        val pageIndex = IntArray(1)
        val annotPtr = LongArray(1)
        return if (nativeFormGetFocusedAnnot(formPtr, pageIndex, annotPtr)) Pair(pageIndex[0], annotPtr[0]) else null
    }
    fun formSetFocusedAnnot(formPtr: Long, annotPtr: Long): Boolean = nativeFormSetFocusedAnnot(formPtr, annotPtr)
    fun pageHasFormFieldAtPoint(formPtr: Long, pagePtr: Long, x: Double, y: Double): Int = nativePageHasFormFieldAtPoint(formPtr, pagePtr, x, y)
    fun pageFormFieldZOrderAtPoint(formPtr: Long, pagePtr: Long, x: Double, y: Double): Int = nativePageFormFieldZOrderAtPoint(formPtr, pagePtr, x, y)
    fun setFormFieldHighlightColor(formPtr: Long, fieldType: Int, color: Long) = nativeSetFormFieldHighlightColor(formPtr, fieldType, color)
    fun setFormFieldHighlightAlpha(formPtr: Long, alpha: Int) = nativeSetFormFieldHighlightAlpha(formPtr, alpha)
    fun removeFormFieldHighlight(formPtr: Long) = nativeRemoveFormFieldHighlight(formPtr)
    fun formSetIndexSelected(formPtr: Long, pagePtr: Long, index: Int, selected: Boolean): Boolean = nativeFormSetIndexSelected(formPtr, pagePtr, index, selected)
    fun formIsIndexSelected(formPtr: Long, pagePtr: Long, index: Int): Boolean = nativeFormIsIndexSelected(formPtr, pagePtr, index)
    fun loadXFA(docPtr: Long): Boolean = nativeLoadXFA(docPtr)

    // --- Annotation Getters ---
    private external fun nativeGetAnnotColor(annotPtr: Long, colorType: Int, result: IntArray): Boolean
    private external fun nativeGetAnnotFlags(annotPtr: Long): Int

    fun getAnnotColor(annotPtr: Long, colorType: Int): IntArray? {
        val result = IntArray(4)
        return if (nativeGetAnnotColor(annotPtr, colorType, result)) result else null
    }
    fun getAnnotFlags(annotPtr: Long) = nativeGetAnnotFlags(annotPtr)

    // --- Actions ---
    private external fun nativeGetActionType(actionPtr: Long): Int
    private external fun nativeGetActionDest(docPtr: Long, actionPtr: Long): Long
    private external fun nativeGetActionFilePath(actionPtr: Long): String?

    fun getActionType(actionPtr: Long) = nativeGetActionType(actionPtr)
    fun getActionDest(docPtr: Long, actionPtr: Long) = nativeGetActionDest(docPtr, actionPtr)
    fun getActionFilePath(actionPtr: Long) = nativeGetActionFilePath(actionPtr) ?: ""

    // --- Bookmarks ---
    private external fun nativeFindBookmark(docPtr: Long, title: String): Long
    private external fun nativeGetBookmarkDest(docPtr: Long, bookmarkPtr: Long): Long
    private external fun nativeGetBookmarkAction(bookmarkPtr: Long): Long
    private external fun nativeGetLinkAction(linkPtr: Long): Long
    private external fun nativeBookmarkGetCount(bookmarkPtr: Long): Int
    private external fun nativeLinkGetLinkZOrderAtPoint(pagePtr: Long, x: Double, y: Double): Int
    private external fun nativeLinkEnumerate(pagePtr: Long, startIndex: IntArray, linkPtr: LongArray): Boolean
    private external fun nativeLinkGetAnnot(pagePtr: Long, linkPtr: Long): Long
    private external fun nativeLinkCountQuadPoints(linkPtr: Long): Int
    private external fun nativeDestGetView(destPtr: Long, numParams: LongArray, params: FloatArray): Long
    private external fun nativeDestGetLocationInPage(destPtr: Long, hasXYZ: BooleanArray, location: FloatArray): Boolean
    private external fun nativeGetFileIdentifier(docPtr: Long, idType: Int): ByteArray?

    fun findBookmark(docPtr: Long, title: String) = nativeFindBookmark(docPtr, title)
    fun getBookmarkDest(docPtr: Long, bookmarkPtr: Long) = nativeGetBookmarkDest(docPtr, bookmarkPtr)
    fun getBookmarkAction(bookmarkPtr: Long) = nativeGetBookmarkAction(bookmarkPtr)
    fun getLinkAction(linkPtr: Long) = nativeGetLinkAction(linkPtr)
    fun bookmarkGetCount(bookmarkPtr: Long) = nativeBookmarkGetCount(bookmarkPtr)
    fun linkGetLinkZOrderAtPoint(pagePtr: Long, x: Double, y: Double) = nativeLinkGetLinkZOrderAtPoint(pagePtr, x, y)
    fun linkEnumerate(pagePtr: Long, startIndex: Int): Pair<Int, Long> {
        val startArr = IntArray(1)
        startArr[0] = startIndex
        val linkArr = LongArray(1)
        if (nativeLinkEnumerate(pagePtr, startArr, linkArr)) return Pair(startArr[0], linkArr[0])
        return Pair(startIndex, 0L)
    }

    fun linkGetAnnot(pagePtr: Long, linkPtr: Long) = nativeLinkGetAnnot(pagePtr, linkPtr)
    fun linkCountQuadPoints(linkPtr: Long) = nativeLinkCountQuadPoints(linkPtr)
    fun destGetView(destPtr: Long): Triple<Long, Long, FloatArray> {
        val numParams = LongArray(1)
        val params = FloatArray(4)
        val viewType = nativeDestGetView(destPtr, numParams, params)
        return Triple(viewType, numParams[0], params)
    }

    fun destGetLocationInPage(destPtr: Long): Pair<BooleanArray, FloatArray>? {
        val hasXYZ = BooleanArray(3)
        val location = FloatArray(3)
        if (nativeDestGetLocationInPage(destPtr, hasXYZ, location)) return Pair(hasXYZ, location)
        return null
    }

    fun getFileIdentifier(docPtr: Long, idType: Int) = nativeGetFileIdentifier(docPtr, idType)

    // --- Text Rectangles ---
    private external fun nativeTextCountRects(textPagePtr: Long, startIndex: Int, count: Int): Int
    private external fun nativeTextGetRect(textPagePtr: Long, index: Int, result: DoubleArray): Boolean

    fun textCountRects(textPagePtr: Long, startIndex: Int, count: Int) = nativeTextCountRects(textPagePtr, startIndex, count)
    fun textGetRect(textPagePtr: Long, index: Int): DoubleArray? {
        val result = DoubleArray(4)
        return if (nativeTextGetRect(textPagePtr, index, result)) result else null
    }

    // --- Attachment Operations ---
    private external fun nativeAddAttachment(docPtr: Long, name: String): Long
    private external fun nativeDeleteAttachment(docPtr: Long, index: Int): Boolean
    private external fun nativeAttachmentHasKey(attachmentPtr: Long, key: String): Boolean
    private external fun nativeAttachmentGetValueType(attachmentPtr: Long, key: String): Int
    private external fun nativeAttachmentSetStringValue(attachmentPtr: Long, key: String, value: String): Boolean
    private external fun nativeAttachmentGetStringValue(attachmentPtr: Long, key: String): String?
    private external fun nativeAttachmentSetFile(attachmentPtr: Long, docPtr: Long, contents: ByteArray): Boolean
    private external fun nativeAttachmentGetSubtype(attachmentPtr: Long): String?

    fun addAttachment(docPtr: Long, name: String) = nativeAddAttachment(docPtr, name)
    fun deleteAttachment(docPtr: Long, index: Int) = nativeDeleteAttachment(docPtr, index)
    fun attachmentHasKey(attachmentPtr: Long, key: String): Boolean = nativeAttachmentHasKey(attachmentPtr, key)
    fun attachmentGetValueType(attachmentPtr: Long, key: String): Int = nativeAttachmentGetValueType(attachmentPtr, key)
    fun attachmentSetStringValue(attachmentPtr: Long, key: String, value: String): Boolean = nativeAttachmentSetStringValue(attachmentPtr, key, value)
    fun attachmentGetStringValue(attachmentPtr: Long, key: String): String = nativeAttachmentGetStringValue(attachmentPtr, key) ?: ""
    fun attachmentSetFile(attachmentPtr: Long, docPtr: Long, contents: ByteArray): Boolean = nativeAttachmentSetFile(attachmentPtr, docPtr, contents)
    fun attachmentGetSubtype(attachmentPtr: Long): String = nativeAttachmentGetSubtype(attachmentPtr) ?: ""

    // --- Page Object Colors (Get) ---
    private external fun nativeGetObjectStrokeColor(pageObjPtr: Long, result: IntArray): Boolean
    private external fun nativeGetObjectFillColor(pageObjPtr: Long, result: IntArray): Boolean

    fun getObjectStrokeColor(pageObjPtr: Long): IntArray? {
        val result = IntArray(4)
        return if (nativeGetObjectStrokeColor(pageObjPtr, result)) result else null
    }
    fun getObjectFillColor(pageObjPtr: Long): IntArray? {
        val result = IntArray(4)
        return if (nativeGetObjectFillColor(pageObjPtr, result)) result else null
    }

    // --- Page Boxes (Media, Crop) ---
    fun getPageMediaBox(pagePtr: Long): FloatArray? {
        val result = FloatArray(4)
        return if (nativeGetPageMediaBox(pagePtr, result)) result else null
    }

    fun setPageMediaBox(pagePtr: Long, left: Float, bottom: Float, right: Float, top: Float): Boolean {
        return nativeSetPageMediaBox(pagePtr, left, bottom, right, top)
    }

    fun getPageCropBox(pagePtr: Long): FloatArray? {
        val result = FloatArray(4)
        return if (nativeGetPageCropBox(pagePtr, result)) result else null
    }

    fun setPageCropBox(pagePtr: Long, left: Float, bottom: Float, right: Float, top: Float): Boolean {
        return nativeSetPageCropBox(pagePtr, left, bottom, right, top)
    }

    // --- Page Boxes (Bleed, Trim, Art) ---
    private external fun nativeSetPageBleedBox(pagePtr: Long, left: Float, bottom: Float, right: Float, top: Float)
    private external fun nativeSetPageTrimBox(pagePtr: Long, left: Float, bottom: Float, right: Float, top: Float)
    private external fun nativeSetPageArtBox(pagePtr: Long, left: Float, bottom: Float, right: Float, top: Float)
    private external fun nativeGetPageBleedBox(pagePtr: Long, result: FloatArray): Boolean
    private external fun nativeGetPageTrimBox(pagePtr: Long, result: FloatArray): Boolean
    private external fun nativeGetPageArtBox(pagePtr: Long, result: FloatArray): Boolean

    fun setPageBleedBox(pagePtr: Long, left: Float, bottom: Float, right: Float, top: Float) = nativeSetPageBleedBox(pagePtr, left, bottom, right, top)
    fun setPageTrimBox(pagePtr: Long, left: Float, bottom: Float, right: Float, top: Float) = nativeSetPageTrimBox(pagePtr, left, bottom, right, top)
    fun setPageArtBox(pagePtr: Long, left: Float, bottom: Float, right: Float, top: Float) = nativeSetPageArtBox(pagePtr, left, bottom, right, top)
    fun getPageBleedBox(pagePtr: Long): FloatArray? {
        val result = FloatArray(4)
        return if (nativeGetPageBleedBox(pagePtr, result)) result else null
    }
    fun getPageTrimBox(pagePtr: Long): FloatArray? {
        val result = FloatArray(4)
        return if (nativeGetPageTrimBox(pagePtr, result)) result else null
    }
    fun getPageArtBox(pagePtr: Long): FloatArray? {
        val result = FloatArray(4)
        return if (nativeGetPageArtBox(pagePtr, result)) result else null
    }

    // --- StructTree Extended ---
    private external fun nativeStructElementCountChildren(structElemPtr: Long): Int
    private external fun nativeStructElementGetChildAtIndex(structElemPtr: Long, index: Int): Long
    private external fun nativeStructElementGetActualText(structElemPtr: Long): String?
    private external fun nativeStructElementGetID(structElemPtr: Long): String?
    private external fun nativeStructElementGetLang(structElemPtr: Long): String?
    private external fun nativeStructElementGetStringAttribute(structElemPtr: Long, attrName: String): String?
    private external fun nativeStructElementGetMarkedContentID(structElemPtr: Long): Int
    private external fun nativeStructElementGetObjType(structElemPtr: Long): String?
    private external fun nativeStructElementGetTitle(structElemPtr: Long): String?
    private external fun nativeStructElementGetChildMarkedContentID(structElemPtr: Long, index: Int): Int
    private external fun nativeStructElementGetParent(structElemPtr: Long): Long
    private external fun nativeStructElementGetAttributeCount(structElemPtr: Long): Int
    private external fun nativeStructElementGetAttributeAtIndex(structElemPtr: Long, index: Int): Long
    private external fun nativeStructElementAttrGetCount(attrPtr: Long): Int
    private external fun nativeStructElementAttrGetName(attrPtr: Long, index: Int): String?
    private external fun nativeStructElementAttrGetValue(attrPtr: Long, name: String): Long
    private external fun nativeStructElementAttrGetType(valuePtr: Long): Int
    private external fun nativeStructElementAttrGetBooleanValue(valuePtr: Long, outValue: BooleanArray): Boolean
    private external fun nativeStructElementAttrGetNumberValue(valuePtr: Long, outValue: FloatArray): Boolean
    private external fun nativeStructElementAttrGetStringValue(valuePtr: Long): Boolean
    private external fun nativeStructElementAttrGetBlobValue(valuePtr: Long): ByteArray?
    private external fun nativeStructElementAttrCountChildren(valuePtr: Long): Int
    private external fun nativeStructElementAttrGetChildAtIndex(valuePtr: Long, index: Int): Long
    private external fun nativeStructElementGetMarkedContentIdCount(structElemPtr: Long): Int
    private external fun nativeStructElementGetMarkedContentIdAtIndex(structElemPtr: Long, index: Int): Int
    private external fun nativeTransformPageWithClip(pagePtr: Long, a: Float, b: Float, c: Float, d: Float, e: Float, f: Float, clipLeft: Float, clipBottom: Float, clipRight: Float, clipTop: Float): Boolean
    private external fun nativeTransformClipPath(pageObjPtr: Long, a: Double, b: Double, c: Double, d: Double, e: Double, f: Double)
    private external fun nativeGetPageObjectClipPath(pageObjPtr: Long): Long
    private external fun nativeClipPathCountPaths(clipPathPtr: Long): Int
    private external fun nativeClipPathCountPathSegments(clipPathPtr: Long, pathIndex: Int): Int
    private external fun nativeClipPathGetPathSegment(clipPathPtr: Long, pathIndex: Int, segmentIndex: Int): Long
    private external fun nativeCreateClipPath(left: Float, bottom: Float, right: Float, top: Float): Long
    private external fun nativeDestroyClipPath(clipPathPtr: Long)
    private external fun nativeInsertClipPath(pagePtr: Long, clipPathPtr: Long)

    fun structElementCountChildren(structElemPtr: Long) = nativeStructElementCountChildren(structElemPtr)
    fun structElementGetChildAtIndex(structElemPtr: Long, index: Int) = nativeStructElementGetChildAtIndex(structElemPtr, index)

    // --- Clip Path Operations ---
    fun transformPageWithClip(pagePtr: Long, matrix: FloatArray, clipRect: FloatArray): Boolean {
        return nativeTransformPageWithClip(pagePtr, matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], clipRect[0], clipRect[1], clipRect[2], clipRect[3])
    }

    fun transformClipPath(pageObjPtr: Long, a: Double, b: Double, c: Double, d: Double, e: Double, f: Double) {
        nativeTransformClipPath(pageObjPtr, a, b, c, d, e, f)
    }

    fun getPageObjectClipPath(pageObjPtr: Long) = nativeGetPageObjectClipPath(pageObjPtr)
    fun clipPathCountPaths(clipPathPtr: Long) = nativeClipPathCountPaths(clipPathPtr)
    fun clipPathCountPathSegments(clipPathPtr: Long, pathIndex: Int) = nativeClipPathCountPathSegments(clipPathPtr, pathIndex)
    fun clipPathGetPathSegment(clipPathPtr: Long, pathIndex: Int, segmentIndex: Int) = nativeClipPathGetPathSegment(clipPathPtr, pathIndex, segmentIndex)
    fun createClipPath(left: Float, bottom: Float, right: Float, top: Float) = nativeCreateClipPath(left, bottom, right, top)
    fun destroyClipPath(clipPathPtr: Long) = nativeDestroyClipPath(clipPathPtr)
    fun insertClipPath(pagePtr: Long, clipPathPtr: Long) = nativeInsertClipPath(pagePtr, clipPathPtr)

    // --- Font Loading ---
    private external fun nativeLoadStandardFont(docPtr: Long, fontName: String): Long
    private external fun nativeCloseFont(fontPtr: Long)

    fun loadStandardFont(docPtr: Long, fontName: String) = nativeLoadStandardFont(docPtr, fontName)
    fun closeFont(fontPtr: Long) = nativeCloseFont(fontPtr)

    // --- Data Availability ---
    private external fun nativeIsLinearized(availPtr: Long): Boolean

    fun isLinearized(availPtr: Long) = nativeIsLinearized(availPtr)
}

/**
 * PDFium error codes
 */
enum class PdfiumError(val code: Int) {
    SUCCESS(0),
    UNKNOWN(1),
    FILE(2),
    FORMAT(3),
    PASSWORD(4),
    SECURITY(5),
    PAGE(6);
    
    companion object {
        fun fromCode(code: Int): PdfiumError {
            return entries.find { it.code == code } ?: UNKNOWN
        }
    }
}
