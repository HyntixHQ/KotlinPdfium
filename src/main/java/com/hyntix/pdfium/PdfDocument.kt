package com.hyntix.pdfium

import java.io.Closeable

/**
 * Represents an opened PDF document.
 * 
 * Use [PdfiumCore.openDocument] to create instances.
 * Always call [close] when done to release native resources.
 */
class PdfDocument internal constructor(
    private val core: PdfiumCore,
    private val docPtr: Long
) : Closeable {
    
    private var isClosed = false
    
    /**
     * Total number of pages in the document.
     */
    val pageCount: Int
        get() {
            checkNotClosed()
            return core.getPageCount(docPtr)
        }
    
    /**
     * Document title from metadata.
     */
    val title: String get() = getMetaText("Title")
    
    /**
     * Document author from metadata.
     */
    val author: String get() = getMetaText("Author")
    
    /**
     * Document subject from metadata.
     */
    val subject: String get() = getMetaText("Subject")
    
    /**
     * Document keywords from metadata.
     */
    val keywords: String get() = getMetaText("Keywords")
    
    /**
     * PDF creator application from metadata.
     */
    val creator: String get() = getMetaText("Creator")
    
    /**
     * PDF producer from metadata.
     */
    val producer: String get() = getMetaText("Producer")
    
    /**
     * Document creation date from metadata.
     */
    val creationDate: String get() = getMetaText("CreationDate")
    
    /**
     * Document modification date from metadata.
     */
    val modificationDate: String get() = getMetaText("ModDate")
    
    /**
     * Get metadata text by tag name.
     * 
     * Supported tags: Title, Author, Subject, Keywords, Creator, Producer, CreationDate, ModDate
     */
    fun getMetaText(tag: String): String {
        checkNotClosed()
        return core.getMetaText(docPtr, tag)
    }
    
    /**
     * Check if the document has been closed.
     */
    fun isClosed(): Boolean = isClosed
    
    /**
     * Close the document and release native resources.
     */
    override fun close() {
        if (!isClosed) {
            core.closeDocument(docPtr)
            isClosed = true
        }
    }
    
    /**
     * Get the native document pointer.
     * For internal use only.
     */
    internal fun getPointer(): Long {
        checkNotClosed()
        return docPtr
    }
    
    private fun checkNotClosed() {
        check(!isClosed) { "Document has been closed" }
    }
    
    /**
     * Open a page at the specified index.
     * Use this to access page content, dimensions, and render it.
     */
    fun openPage(index: Int): PdfPage {
        checkNotClosed()
        require(index in 0 until pageCount) { "Page index $index out of bounds" }
        
        val pagePtr = core.loadPage(docPtr, index)
        if (pagePtr == 0L) {
            throw IllegalStateException("Failed to load page $index")
        }
        
        return PdfPage(core, docPtr, pagePtr, index)
    }
    
    /**
     * Get all page sizes in a single batch operation.
     * Uses FPDF_GetPageSizeByIndex which is much faster than opening each page.
     * 
     * @return List of Pair(width, height) for all pages
     */
    fun getAllPageSizes(): List<Pair<Double, Double>> {
        checkNotClosed()
        val count = pageCount
        val sizes = ArrayList<Pair<Double, Double>>(count)
        
        for (i in 0 until count) {
            // Uses FPDF_GetPageSizeByIndex - no page loading needed
            sizes.add(core.getPageSizeByIndex(docPtr, i))
        }
        
        return sizes
    }
    
    /**
     * Get page size without keeping the page open.
     * Useful when only dimensions are needed.
     */
    fun getPageSize(index: Int): Pair<Double, Double> {
        checkNotClosed()
        require(index in 0 until pageCount) { "Page index $index out of bounds" }
        
        val pagePtr = core.loadPage(docPtr, index)
        return if (pagePtr != 0L) {
            val width = core.getPageWidth(pagePtr)
            val height = core.getPageHeight(pagePtr)
            core.closePage(pagePtr)
            Pair(width, height)
        } else {
            Pair(595.0, 842.0) // Default A4
        }
    }

    /**
     * Get page label (actual page number as displayed in PDF)
     * Returns empty string if no label is defined for the page
     * 
     * @param pageIndex 0-based page index
     * @return The page label (e.g., "i", "ii", "1", "2", "A-1") or empty string
     */
    fun getPageLabel(pageIndex: Int): String {
        checkNotClosed()
        return core.getPageLabel(docPtr, pageIndex)
    }

    /**
     * Get the Table of Contents (Bookmarks) of the document.
     */
    fun getTableOfContents(): List<PdfBookmark> {
        checkNotClosed()
        val bookmarks = ArrayList<PdfBookmark>()
        val firstPtr = core.getFirstChildBookmark(docPtr, 0)
        if (firstPtr != 0L) {
            recursiveGetBookmarks(bookmarks, firstPtr)
        }
        return bookmarks
    }

    private fun recursiveGetBookmarks(tree: MutableList<PdfBookmark>, bookmarkPtr: Long) {
        var currentPtr = bookmarkPtr
        while (currentPtr != 0L) {
            val title = core.getBookmarkTitle(currentPtr)
            val pageIndex = core.getBookmarkDestIndex(docPtr, currentPtr)
            val children = ArrayList<PdfBookmark>()
            
            // Get the page label for this bookmark's destination page
            val pageLabel = if (pageIndex >= 0) core.getPageLabel(docPtr, pageIndex.toInt()) else ""
            
            val childPtr = core.getFirstChildBookmark(docPtr, currentPtr)
            if (childPtr != 0L) {
                recursiveGetBookmarks(children, childPtr)
            }
            
            tree.add(PdfBookmark(title, pageIndex, children, pageLabel))
            currentPtr = core.getNextSiblingBookmark(docPtr, currentPtr)
        }
    }

    /**
     * Save the document to a file.
     * 
     * @param path File path to save the PDF to
     * @return True if successful
     */
    fun saveAs(path: String): Boolean {
        checkNotClosed()
        return core.saveDocument(docPtr, path)
    }

    /**
     * Add a new blank page to the document.
     * 
     * @param index Insertion index (0-based)
     * @param width Page width in points
     * @param height Page height in points
     * @return The newly created PdfPage
     */
    fun addNewPage(index: Int, width: Double, height: Double): PdfPage {
        checkNotClosed()
        val pagePtr = core.newPage(docPtr, index, width, height)
        if (pagePtr == 0L) {
            throw IllegalStateException("Failed to create new page")
        }
        return PdfPage(core, docPtr, pagePtr, index)
    }
    
    /**
     * Import pages from another document into this document.
     * 
     * @param sourceDoc Source document to import from
     * @param pageRange Range of pages to import (e.g. "1,3,5-7"), or null for all pages.
     * @param insertIndex Index to insert pages at, default is end of document.
     * @return True if successful
     */
    fun importPages(sourceDoc: PdfDocument, pageRange: String? = null, insertIndex: Int = pageCount): Boolean {
        checkNotClosed()
        // We can access internal getPointer() because we are in the same module
        return core.importPages(docPtr, sourceDoc.getPointer(), pageRange, insertIndex)
    }

    // --- Attachments ---
    
    /**
     * Number of embedded file attachments in the document.
     */
    val attachmentCount: Int
        get() {
            checkNotClosed()
            return core.getAttachmentCount(docPtr)
        }
    
    /**
     * Get attachment at specified index.
     */
    fun getAttachment(index: Int): PdfAttachment {
        checkNotClosed()
        require(index in 0 until attachmentCount) { "Attachment index $index out of bounds" }
        val name = core.getAttachmentName(docPtr, index)
        val data = core.getAttachmentFile(docPtr, index)
        return PdfAttachment(index, name, data)
    }
    
    /**
     * Get all attachments in the document.
     */
    fun getAllAttachments(): List<PdfAttachment> {
        checkNotClosed()
        val count = attachmentCount
        return (0 until count).map { getAttachment(it) }
    }
    
    // --- Digital Signatures ---
    
    /**
     * Number of digital signatures in the document.
     */
    val signatureCount: Int
        get() {
            checkNotClosed()
            return core.getSignatureCount(docPtr)
        }
    
    /**
     * Check if the document is digitally signed.
     */
    val isSigned: Boolean get() = signatureCount > 0
    
    /**
     * Get signature at specified index.
     */
    fun getSignature(index: Int): PdfSignature {
        checkNotClosed()
        require(index in 0 until signatureCount) { "Signature index $index out of bounds" }
        val sigPtr = core.getSignatureObject(docPtr, index)
        val reason = core.getSignatureReason(sigPtr)
        val time = core.getSignatureTime(sigPtr)
        return PdfSignature(index, reason, time)
    }
    
    /**
     * Get all signatures in the document.
     */
    fun getAllSignatures(): List<PdfSignature> {
        checkNotClosed()
        val count = signatureCount
        return (0 until count).map { getSignature(it) }
    }
    
    override fun toString(): String {
        return "PdfDocument(pageCount=$pageCount, title='$title', closed=$isClosed)"
    }
}
