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

    internal fun saveDocument(docPtr: Long, path: String): Boolean {
        return nativeSaveDocument(docPtr, path)
    }

    internal fun newPage(docPtr: Long, index: Int, width: Double, height: Double): Long {
        return nativeNewPage(docPtr, index, width, height)
    }

    // Internal methods for PdfDocument to use
    internal fun closeDocument(docPtr: Long) {
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
    internal fun loadTextPage(docPtr: Long, pagePtr: Long): Long {
        return nativeLoadTextPage(docPtr, pagePtr)
    }

    internal fun closeTextPage(textPagePtr: Long) {
        nativeCloseTextPage(textPagePtr)
    }

    internal fun getTextCount(textPagePtr: Long): Int {
        return nativeTextCountChars(textPagePtr)
    }

    internal fun getText(textPagePtr: Long, startIndex: Int, count: Int): String {
        return nativeGetText(textPagePtr, startIndex, count) ?: ""
    }

    internal fun getCharBox(textPagePtr: Long, index: Int): DoubleArray {
        val result = DoubleArray(4)
        nativeGetCharBox(textPagePtr, index, result)
        return result
    }

    internal fun getCharIndexAtPos(textPagePtr: Long, x: Double, y: Double, xTolerance: Double, yTolerance: Double): Int {
        return nativeGetCharIndexAtPos(textPagePtr, x, y, xTolerance, yTolerance)
    }

    // Search Operations
    internal fun textFindStart(textPagePtr: Long, query: String, matchCase: Boolean, matchWholeWord: Boolean): Long {
        return nativeTextFindStart(textPagePtr, query, matchCase, matchWholeWord)
    }

    internal fun textFindNext(searchHandle: Long): Boolean {
        return nativeTextFindNext(searchHandle)
    }

    internal fun textFindPrev(searchHandle: Long): Boolean {
        return nativeTextFindPrev(searchHandle)
    }

    internal fun textGetSchResultIndex(searchHandle: Long): Int {
        return nativeTextGetSchResultIndex(searchHandle)
    }

    internal fun textGetSchCount(searchHandle: Long): Int {
        return nativeTextGetSchCount(searchHandle)
    }

    internal fun textFindClose(searchHandle: Long) {
        nativeTextFindClose(searchHandle)
    }

    // Bookmark Operations
    internal fun getFirstChildBookmark(docPtr: Long, bookmarkPtr: Long): Long {
        return nativeGetFirstChildBookmark(docPtr, bookmarkPtr)
    }

    internal fun getNextSiblingBookmark(docPtr: Long, bookmarkPtr: Long): Long {
        return nativeGetNextSiblingBookmark(docPtr, bookmarkPtr)
    }

    internal fun getBookmarkTitle(bookmarkPtr: Long): String {
        return nativeGetBookmarkTitle(bookmarkPtr) ?: ""
    }

    internal fun getBookmarkDestIndex(docPtr: Long, bookmarkPtr: Long): Long {
        return nativeGetBookmarkDestIndex(docPtr, bookmarkPtr)
    }

    // Link Operations
    internal fun getLinkAtPoint(pagePtr: Long, x: Double, y: Double): Long {
        return nativeGetLinkAtPoint(pagePtr, x, y)
    }

    internal fun getLinkDestIndex(docPtr: Long, linkPtr: Long): Int {
        return nativeGetLinkDestIndex(docPtr, linkPtr)
    }

    internal fun getLinkURI(docPtr: Long, linkPtr: Long): String? {
        return nativeGetLinkURI(docPtr, linkPtr)
    }

    internal fun getLinkRect(linkPtr: Long): DoubleArray {
        val result = DoubleArray(4)
        nativeGetLinkRect(linkPtr, result)
        return result
    }

    // Annotation Operations
    internal fun getAnnotCount(pagePtr: Long): Int {
        return nativeGetAnnotCount(pagePtr)
    }

    internal fun getAnnot(pagePtr: Long, index: Int): Long {
        return nativeGetAnnot(pagePtr, index)
    }

    internal fun closeAnnot(annotPtr: Long) {
        nativeCloseAnnot(annotPtr)
    }

    internal fun getAnnotSubtype(annotPtr: Long): Int {
        return nativeGetAnnotSubtype(annotPtr)
    }

    internal fun getAnnotRect(annotPtr: Long): DoubleArray {
        val result = DoubleArray(4)
        nativeGetAnnotRect(annotPtr, result)
        return result
    }

    // Native methods
    private external fun nativeInitLibrary()
    private external fun nativeDestroyLibrary()
    private external fun nativeGetLastError(): Int
    private external fun nativeOpenDocument(fd: Int, password: String?): Long
    private external fun nativeOpenMemDocument(data: ByteArray, password: String?): Long
    private external fun nativeOpenDocumentPath(path: String, password: String?): Long
    private external fun nativeCloseDocument(docPtr: Long)
    private external fun nativeGetPageCount(docPtr: Long): Int
    private external fun nativeGetMetaText(docPtr: Long, tag: String): String?
    private external fun nativeGetPageLabel(docPtr: Long, pageIndex: Int): String?
    
    // Creation & Saving Native methods
    private external fun nativeNewDocument(): Long
    private external fun nativeNewPage(docPtr: Long, index: Int, width: Double, height: Double): Long
    private external fun nativeSaveDocument(docPtr: Long, path: String): Boolean 
    
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
    internal fun initFormFillEnvironment(docPtr: Long): Long {
        return nativeInitFormFillEnvironment(docPtr)
    }

    internal fun exitFormFillEnvironment(formHandlePtr: Long) {
        nativeExitFormFillEnvironment(formHandlePtr)
    }

    internal fun formOnAfterLoadPage(pagePtr: Long, formHandlePtr: Long) {
        nativeFORMOnAfterLoadPage(pagePtr, formHandlePtr)
    }

    internal fun formOnBeforeClosePage(pagePtr: Long, formHandlePtr: Long) {
        nativeFORMOnBeforeClosePage(pagePtr, formHandlePtr)
    }

    internal fun renderFormBitmap(
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

    internal fun mapPageToDevice(
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

    internal fun mapDeviceToPage(
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
    private external fun nativeGetCharBox(textPagePtr: Long, index: Int, result: DoubleArray)
    private external fun nativeGetCharIndexAtPos(textPagePtr: Long, x: Double, y: Double, xTolerance: Double, yTolerance: Double): Int
    
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

    internal fun getLinkFromAnnot(annotPtr: Long): Long {
        return nativeGetLinkFromAnnot(annotPtr)
    }

    // Annotation Editing Helpers
    internal fun createAnnot(pagePtr: Long, subtype: Int): Long {
        return nativeCreateAnnot(pagePtr, subtype)
    }

    internal fun setAnnotRect(annotPtr: Long, rect: DoubleArray): Boolean {
        return nativeSetAnnotRect(annotPtr, rect)
    }

    internal fun setAnnotContents(annotPtr: Long, contents: String): Boolean {
        return nativeSetAnnotContents(annotPtr, contents)
    }

    internal fun setAnnotColor(annotPtr: Long, type: Int, r: Int, g: Int, b: Int, a: Int): Boolean {
        return nativeSetAnnotColor(annotPtr, type, r, g, b, a)
    }

    internal fun setAnnotFlags(annotPtr: Long, flags: Int): Boolean {
        return nativeSetAnnotFlags(annotPtr, flags)
    }

    // Attachment Native methods
    private external fun nativeGetAttachmentCount(docPtr: Long): Int
    private external fun nativeGetAttachmentName(docPtr: Long, index: Int): String?
    private external fun nativeGetAttachmentFile(docPtr: Long, index: Int): ByteArray?

    // Attachment Helpers
    internal fun getAttachmentCount(docPtr: Long): Int {
        return nativeGetAttachmentCount(docPtr)
    }

    internal fun getAttachmentName(docPtr: Long, index: Int): String {
        return nativeGetAttachmentName(docPtr, index) ?: ""
    }

    internal fun getAttachmentFile(docPtr: Long, index: Int): ByteArray? {
        return nativeGetAttachmentFile(docPtr, index)
    }

    // Page Object Native methods
    private external fun nativeCountPageObjects(pagePtr: Long): Int
    private external fun nativeGetPageObject(pagePtr: Long, index: Int): Long
    private external fun nativeGetPageObjectType(pageObjPtr: Long): Int

    // Page Object Helpers
    internal fun countPageObjects(pagePtr: Long): Int {
        return nativeCountPageObjects(pagePtr)
    }

    internal fun getPageObject(pagePtr: Long, index: Int): Long {
        return nativeGetPageObject(pagePtr, index)
    }

    internal fun getPageObjectType(pageObjPtr: Long): Int {
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
    private external fun nativeInsertObject(pagePtr: Long, pageObjPtr: Long)
    private external fun nativeRemoveObject(pagePtr: Long, pageObjPtr: Long): Boolean
    private external fun nativeSetObjectFillColor(pageObjPtr: Long, r: Int, g: Int, b: Int, a: Int)
    private external fun nativeSetObjectStrokeColor(pageObjPtr: Long, r: Int, g: Int, b: Int, a: Int)
    private external fun nativeGenerateContent(pagePtr: Long)

    // Phase 9: Document Utilities
    private external fun nativeImportPages(destDocPtr: Long, srcDocPtr: Long, pageRange: String?, insertIndex: Int): Boolean
    private external fun nativeCopyViewerPreferences(destDocPtr: Long, srcDocPtr: Long): Boolean
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
    private external fun nativeGetStructTreeForPage(pagePtr: Long): Long
    private external fun nativeCloseStructTree(structTreePtr: Long)
    private external fun nativeStructTreeCountChildren(structTreePtr: Long): Int
    private external fun nativeStructTreeGetChildAtIndex(structTreePtr: Long, index: Int): Long
    private external fun nativeStructElementGetType(structElemPtr: Long): String?
    private external fun nativeStructElementGetAltText(structElemPtr: Long): String?

    // Phase 11: Signatures, JS
    private external fun nativeGetSignatureCount(docPtr: Long): Int
    private external fun nativeGetSignatureObject(docPtr: Long, index: Int): Long
    private external fun nativeGetSignatureContents(sigObjPtr: Long): ByteArray?
    private external fun nativeGetSignatureReason(sigObjPtr: Long): String?
    private external fun nativeGetSignatureTime(sigObjPtr: Long): String?
    private external fun nativeGetJavaScriptActionCount(docPtr: Long): Int

    // Phase 12: WebLinks, Enums, etc.
    private external fun nativeLoadWebLinks(textPagePtr: Long): Long
    private external fun nativeCloseWebLinks(pageLinksPtr: Long)
    private external fun nativeCountWebLinks(pageLinksPtr: Long): Int
    private external fun nativeGetWebLinkURL(pageLinksPtr: Long, index: Int): String?
    private external fun nativeGetFormType(docPtr: Long): Int
    private external fun nativeGetPageMode(docPtr: Long): Int
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
    fun pathClose(pathObjPtr: Long): Boolean = nativePathClose(pathObjPtr)
    fun newImageObject(docPtr: Long): Long = nativeNewImageObj(docPtr)
    fun insertObject(pagePtr: Long, pageObjPtr: Long) = nativeInsertObject(pagePtr, pageObjPtr)
    fun removeObject(pagePtr: Long, pageObjPtr: Long): Boolean = nativeRemoveObject(pagePtr, pageObjPtr)
    fun setObjectFillColor(pageObjPtr: Long, r: Int, g: Int, b: Int, a: Int) = nativeSetObjectFillColor(pageObjPtr, r, g, b, a)
    fun setObjectStrokeColor(pageObjPtr: Long, r: Int, g: Int, b: Int, a: Int) = nativeSetObjectStrokeColor(pageObjPtr, r, g, b, a)
    fun generateContent(pagePtr: Long) = nativeGenerateContent(pagePtr)

    // Document Utilities
    fun importPages(destDocPtr: Long, srcDocPtr: Long, pageRange: String?, insertIndex: Int): Boolean = nativeImportPages(destDocPtr, srcDocPtr, pageRange, insertIndex)
    fun copyViewerPreferences(destDocPtr: Long, srcDocPtr: Long): Boolean = nativeCopyViewerPreferences(destDocPtr, srcDocPtr)
    fun flattenPage(pagePtr: Long, flags: Int = 0): Int = nativeFlattenPage(pagePtr, flags)
    fun getPageRotation(pagePtr: Long): Int = nativeGetPageRotation(pagePtr)
    fun setPageRotation(pagePtr: Long, rotation: Int) = nativeSetPageRotation(pagePtr, rotation)
    fun deletePage(docPtr: Long, pageIndex: Int) = nativeDeletePage(docPtr, pageIndex)

    // Thumbnails
    fun getDecodedThumbnailData(pagePtr: Long): ByteArray? = nativeGetDecodedThumbnailData(pagePtr)
    fun getRawThumbnailData(pagePtr: Long): ByteArray? = nativeGetRawThumbnailData(pagePtr)

    // StructTree
    fun getStructTreeForPage(pagePtr: Long): Long = nativeGetStructTreeForPage(pagePtr)
    fun closeStructTree(structTreePtr: Long) = nativeCloseStructTree(structTreePtr)
    fun structTreeCountChildren(structTreePtr: Long): Int = nativeStructTreeCountChildren(structTreePtr)
    fun structTreeGetChildAtIndex(structTreePtr: Long, index: Int): Long = nativeStructTreeGetChildAtIndex(structTreePtr, index)
    fun structElementGetType(structElemPtr: Long): String = nativeStructElementGetType(structElemPtr) ?: ""
    fun structElementGetAltText(structElemPtr: Long): String = nativeStructElementGetAltText(structElemPtr) ?: ""

    // Signatures
    fun getSignatureCount(docPtr: Long): Int = nativeGetSignatureCount(docPtr)
    fun getSignatureObject(docPtr: Long, index: Int): Long = nativeGetSignatureObject(docPtr, index)
    fun getSignatureContents(sigObjPtr: Long): ByteArray? = nativeGetSignatureContents(sigObjPtr)
    fun getSignatureReason(sigObjPtr: Long): String = nativeGetSignatureReason(sigObjPtr) ?: ""
    fun getSignatureTime(sigObjPtr: Long): String = nativeGetSignatureTime(sigObjPtr) ?: ""
    fun getJavaScriptActionCount(docPtr: Long): Int = nativeGetJavaScriptActionCount(docPtr)

    // WebLinks
    fun loadWebLinks(textPagePtr: Long): Long = nativeLoadWebLinks(textPagePtr)
    fun closeWebLinks(pageLinksPtr: Long) = nativeCloseWebLinks(pageLinksPtr)
    fun countWebLinks(pageLinksPtr: Long): Int = nativeCountWebLinks(pageLinksPtr)
    fun getWebLinkURL(pageLinksPtr: Long, index: Int): String = nativeGetWebLinkURL(pageLinksPtr, index) ?: ""

    // Document Info
    fun getFormType(docPtr: Long): Int = nativeGetFormType(docPtr)
    fun getPageMode(docPtr: Long): Int = nativeGetPageMode(docPtr)

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

    fun findBookmark(docPtr: Long, title: String) = nativeFindBookmark(docPtr, title)
    fun getBookmarkDest(docPtr: Long, bookmarkPtr: Long) = nativeGetBookmarkDest(docPtr, bookmarkPtr)
    fun getBookmarkAction(bookmarkPtr: Long) = nativeGetBookmarkAction(bookmarkPtr)
    fun getLinkAction(linkPtr: Long) = nativeGetLinkAction(linkPtr)

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

    fun addAttachment(docPtr: Long, name: String) = nativeAddAttachment(docPtr, name)
    fun deleteAttachment(docPtr: Long, index: Int) = nativeDeleteAttachment(docPtr, index)

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

    fun structElementCountChildren(structElemPtr: Long) = nativeStructElementCountChildren(structElemPtr)
    fun structElementGetChildAtIndex(structElemPtr: Long, index: Int) = nativeStructElementGetChildAtIndex(structElemPtr, index)

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
