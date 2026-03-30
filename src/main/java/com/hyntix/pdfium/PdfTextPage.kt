package com.hyntix.pdfium

import java.io.Closeable
import java.util.ArrayList

/**
 * Represents a text page extracted from a PDF page.
 * 
 * Create instances using [PdfPage.openTextPage].
 * Always call [close] when done to release native resources.
 */
class PdfTextPage internal constructor(
    private val core: PdfiumCore,
    private val docPtr: Long,
    private val pagePtr: Long,
    internal val textPagePtr: Long
) : Closeable {

    private var isClosed = false

    /**
     * Get the total number of characters in the page.
     */
    val charCount: Int
        get() {
            checkNotClosed()
            return core.getTextCount(textPagePtr)
        }

    /**
     * Get the full text of the page.
     */
    val text: String
        get() = extractText(0, charCount)

    /**
     * Extract text from the specified range.
     * 
     * @param startIndex Start index (0-based)
     * @param count Number of characters to extract
     */
    fun extractText(startIndex: Int, count: Int): String {
        checkNotClosed()
        if (count == 0) return ""
        return core.getText(textPagePtr, startIndex, count)
    }

    /**
     * Get the bounding box of a character at the specified index.
     * Returns [left, top, right, bottom].
     */
    fun getCharBox(index: Int): DoubleArray {
        checkNotClosed()
        return core.getCharBox(textPagePtr, index)
    }

    /**
     * Get the index of the character at the specified point (x, y).
     * 
     * @param x X-coordinate
     * @param y Y-coordinate
     * @param xTolerance Horizontal tolerance
     * @param yTolerance Vertical tolerance
     * @return 0-based index of the character, or -1 if no character found.
     */
    fun getIndexAtPos(x: Double, y: Double, xTolerance: Double, yTolerance: Double): Int {
        checkNotClosed()
        return core.getCharIndexAtPos(textPagePtr, x, y, xTolerance, yTolerance)
    }

    /**
     * Search for text on the page.
     * 
     * @param query The text to search for
     * @param matchCase Whether to match case
     * @param matchWholeWord Whether to match whole words
     * @return List of matches
     */
    fun search(query: String, matchCase: Boolean = false, matchWholeWord: Boolean = false): List<PdfTextSearchMatch> {
        checkNotClosed()
        val matches = ArrayList<PdfTextSearchMatch>()
        
        val searchHandle = core.textFindStart(textPagePtr, query, matchCase, matchWholeWord)
        if (searchHandle == 0L) return matches
        
        try {
            while (core.textFindNext(searchHandle)) {
                val index = core.textGetSchResultIndex(searchHandle)
                val count = core.textGetSchCount(searchHandle)
                matches.add(PdfTextSearchMatch(index, count))
            }
        } finally {
            core.textFindClose(searchHandle)
        }
        
        return matches
    }

    /**
     * Get the bounding rectangles for a range of text.
     * Handles multi-line text by returning multiple rectangles.
     */
    fun getTextRects(startIndex: Int, count: Int): List<android.graphics.RectF> {
        checkNotClosed()
        val rects = ArrayList<android.graphics.RectF>()
        val rectCount = core.textCountRects(textPagePtr, startIndex, count)
        
        for (i in 0 until rectCount) {
            val rectArray = core.textGetRect(textPagePtr, i)
            if (rectArray != null) {
                rects.add(android.graphics.RectF(
                    rectArray[0].toFloat(), // left
                    rectArray[1].toFloat(), // top
                    rectArray[2].toFloat(), // right
                    rectArray[3].toFloat()  // bottom
                ))
            }
        }
        return rects
    }
    override fun close() {
        if (!isClosed) {
            core.closeTextPage(textPagePtr)
            isClosed = true
        }
    }

    private fun checkNotClosed() {
        check(!isClosed) { "TextPage has been closed" }
    }
    /**
     * Scan the page for web links.
     */
    fun loadWebLinks(): PdfWebLinks {
        checkNotClosed()
        val linksPtr = core.loadWebLinks(textPagePtr)
        return PdfWebLinks(core, linksPtr)
    }
}

/**
 * Represents a text search result match.
 * 
 * @property startIndex The 0-based start index of the match in the page text.
 * @property count The number of characters in the match.
 */
data class PdfTextSearchMatch(
    val startIndex: Int,
    val count: Int
)
