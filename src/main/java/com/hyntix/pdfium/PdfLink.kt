package com.hyntix.pdfium

import android.graphics.RectF

/**
 * Represents a link on a PDF page.
 */
data class PdfLink(
    val rect: RectF,
    val destPageIndex: Int,
    val uri: String? = null
)
