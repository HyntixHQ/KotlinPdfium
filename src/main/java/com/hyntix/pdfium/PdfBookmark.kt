package com.hyntix.pdfium

import java.util.ArrayList

/**
 * Represents a bookmark in the PDF Table of Contents.
 */
data class PdfBookmark(
    val title: String,
    val pageIndex: Long,
    val children: List<PdfBookmark> = emptyList(),
    /** Actual page label as displayed in the PDF (e.g., "i", "ii", "1", "2", "A-1") */
    val pageLabel: String = ""
) {
    /**
     * Helper to verify if the bookmark points to a valid page.
     */
    fun hasDestination(): Boolean = pageIndex >= 0
}
