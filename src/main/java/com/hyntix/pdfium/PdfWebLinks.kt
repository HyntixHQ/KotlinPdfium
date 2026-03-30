package com.hyntix.pdfium

import java.io.Closeable

/**
 * Represents a collection of web links detected on a text page.
 * 
 * Create instances using [PdfTextPage.loadWebLinks].
 * Always call [close] when done to release native resources.
 */
class PdfWebLinks internal constructor(
    private val core: PdfiumCore,
    private val linksPtr: Long
) : Closeable {

    private var isClosed = false

    /**
     * Get the number of detected web links.
     */
    val count: Int
        get() {
            checkNotClosed()
            return core.countWebLinks(linksPtr)
        }

    /**
     * Get the URL of the link at the specified index.
     */
    fun getURL(index: Int): String {
        checkNotClosed()
        return core.getWebLinkURL(linksPtr, index)
    }

    /**
     * Get the bounding rectangles of the link at the specified index.
     * Note: Native PDFium Android bindings don't expose WebLink rects directly,
     * so we search for the URL text in the page content to find its location.
     */
    fun getRects(index: Int, textPage: PdfTextPage): List<android.graphics.RectF> {
        checkNotClosed()
        val url = getURL(index)
        if (url.isEmpty()) return emptyList()

        // Search for the URL text to find its bounds
        // This is a workaround because getWebLinkRect() isn't exposed
        val matches = textPage.search(url, matchCase = false, matchWholeWord = false)
        val rects = ArrayList<android.graphics.RectF>()
        
        for (match in matches) {
            rects.addAll(textPage.getTextRects(match.startIndex, match.count))
        }
        return rects
    }

    override fun close() {
        if (!isClosed) {
            core.closeWebLinks(linksPtr)
            isClosed = true
        }
    }

    private fun checkNotClosed() {
        check(!isClosed) { "WebLinks has been closed" }
    }
}
