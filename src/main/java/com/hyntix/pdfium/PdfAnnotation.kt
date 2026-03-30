package com.hyntix.pdfium

import android.graphics.RectF

/**
 * Represents an annotation on a PDF page.
 */
data class PdfAnnotation(
    val rect: RectF,
    val subtype: Int
    // Add more properties as needed and supported by JNI
) {
    companion object {
        const val SUBTYPE_TEXT = 1
        const val SUBTYPE_LINK = 2
        const val SUBTYPE_FREETEXT = 3
        const val SUBTYPE_LINE = 4
        const val SUBTYPE_SQUARE = 5
        const val SUBTYPE_CIRCLE = 6
        const val SUBTYPE_POLYGON = 7
        const val SUBTYPE_POLYLINE = 8
        const val SUBTYPE_HIGHLIGHT = 9
        const val SUBTYPE_UNDERLINE = 10
        const val SUBTYPE_SQUIGGLY = 11
        const val SUBTYPE_STRIKEOUT = 12
        const val SUBTYPE_STAMP = 13
        const val SUBTYPE_CARET = 14
        const val SUBTYPE_INK = 15
        const val SUBTYPE_POPUP = 16
        const val SUBTYPE_FILEATTACHMENT = 17
        const val SUBTYPE_SOUND = 18
        const val SUBTYPE_MOVIE = 19
        const val SUBTYPE_WIDGET = 20
        const val SUBTYPE_SCREEN = 21
        const val SUBTYPE_PRINTERMARK = 22
        const val SUBTYPE_TRAPNET = 23
        const val SUBTYPE_WATERMARK = 24
        const val SUBTYPE_3D = 25
        const val SUBTYPE_RICHMEDIA = 26
        const val SUBTYPE_XFAWIDGET = 27
    }
}
