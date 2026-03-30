package com.hyntix.pdfium

import android.graphics.Bitmap
import java.io.Closeable
import kotlin.math.min
import kotlin.math.max

/**
 * Represents a page in a PDF document.
 * 
 * Create instances using [PdfDocument.openPage].
 * Always call [close] when done to release native resources.
 */
class PdfPage internal constructor(
    private val core: PdfiumCore,
    private val docPtr: Long,
    private val pagePtr: Long,
    val index: Int
) : Closeable {

    private var isClosed = false


    /**
     * Page width in points (1/72 inch).
     */
    val width: Double by lazy {
        checkNotClosed()
        core.getPageWidth(pagePtr)
    }

    /**
     * Page height in points (1/72 inch).
     */
    val height: Double by lazy {
        checkNotClosed()
        core.getPageHeight(pagePtr)
    }

    /**
     * Render the page to an Android Bitmap.
     * 
     * @param bitmap The target bitmap. Must be mutable and configured as ARGB_8888.
     * @param startX X-coordinate of the upper-left corner of the drawing area.
     * @param startY Y-coordinate of the upper-left corner of the drawing area.
     * @param drawWidth Width of the drawing area.
     * @param drawHeight Height of the drawing area.
     * @param renderAnnot Whether to render annotations.
     */
    fun render(
        bitmap: Bitmap,
        startX: Int = 0,
        startY: Int = 0,
        drawWidth: Int = bitmap.width,
        drawHeight: Int = bitmap.height,
        renderAnnot: Boolean = true
    ) {
        checkNotClosed()
        core.renderPageBitmap(pagePtr, bitmap, startX, startY, drawWidth, drawHeight, renderAnnot)
    }

    /**
     * Check if the page has been closed.
     */
    fun isClosed(): Boolean = isClosed
    
    fun openTextPage(): PdfTextPage {
        checkNotClosed()
        val textPagePtr = core.loadTextPage(docPtr, pagePtr)
        if (textPagePtr == 0L) {
            throw IllegalStateException("Failed to load text page")
        }
        return PdfTextPage(core, docPtr, pagePtr, textPagePtr)
    }

    /**
     * Get all annotations on the page.
     */
    fun getAnnotations(): List<PdfAnnotation> {
        checkNotClosed()
        val count = core.getAnnotCount(pagePtr)
        val annotations = ArrayList<PdfAnnotation>(count)
        
        for (i in 0 until count) {
            val annotPtr = core.getAnnot(pagePtr, i)
            if (annotPtr != 0L) {
                val subtype = core.getAnnotSubtype(annotPtr)
                val rectArray = core.getAnnotRect(annotPtr)
                val rect = android.graphics.RectF(
                    rectArray[0].toFloat(),
                    rectArray[1].toFloat(),
                    rectArray[2].toFloat(),
                    rectArray[3].toFloat()
                )
                annotations.add(PdfAnnotation(rect, subtype))
                core.closeAnnot(annotPtr)
            }
        }
        return annotations
    }

    /**
     * Get all links on the page.
     */
    fun getLinks(): List<PdfLink> {
        checkNotClosed()
        val count = core.getAnnotCount(pagePtr)
        val links = ArrayList<PdfLink>()

        for (i in 0 until count) {
            val annotPtr = core.getAnnot(pagePtr, i)
            if (annotPtr != 0L) {
                val subtype = core.getAnnotSubtype(annotPtr)
                if (subtype == 2) { // FPDF_ANNOT_LINK
                    val rectArray = core.getAnnotRect(annotPtr)
                    val rect = android.graphics.RectF(
                        rectArray[0].toFloat(), rectArray[1].toFloat(),
                        rectArray[2].toFloat(), rectArray[3].toFloat()
                    )

                    val linkPtr = core.getLinkFromAnnot(annotPtr)
                    val destIndex = core.getLinkDestIndex(docPtr, linkPtr)
                    val uri = core.getLinkURI(docPtr, linkPtr)

                    links.add(PdfLink(rect, destIndex, uri))
                }
                core.closeAnnot(annotPtr)
            }
        }
        return links
    }

    /**
     * Get a link at the specified coordinates.
     */
    fun getLinkAt(x: Double, y: Double): PdfLink? {
        checkNotClosed()
        val linkPtr = core.getLinkAtPoint(pagePtr, x, y)
        if (linkPtr != 0L) {
            val destIndex = core.getLinkDestIndex(docPtr, linkPtr)
            val uri = core.getLinkURI(docPtr, linkPtr)
            val rectArray = core.getLinkRect(linkPtr)
            val rect = android.graphics.RectF(
                rectArray[0].toFloat(), rectArray[1].toFloat(),
                rectArray[2].toFloat(), rectArray[3].toFloat()
            )
            return PdfLink(rect, destIndex, uri)
        }
        return null
    }

    /**
     * Close the page and release native resources.
     */
    override fun close() {
        if (!isClosed) {
            core.closePage(pagePtr)
            isClosed = true
        }
    }

    private fun checkNotClosed() {
        check(!isClosed) { "Page has been closed" }
    }
    
    /**
     * Map a rectangle from page coordinates to device coordinates.
     */
    fun mapRectToDevice(
        startX: Int, startY: Int,
        sizeX: Int, sizeY: Int,
        rect: android.graphics.RectF
    ): android.graphics.RectF {
        checkNotClosed()
        val leftTop = core.mapPageToDevice(pagePtr, startX, startY, sizeX, sizeY, 0, rect.left.toDouble(), rect.top.toDouble())
        val rightBottom = core.mapPageToDevice(pagePtr, startX, startY, sizeX, sizeY, 0, rect.right.toDouble(), rect.bottom.toDouble())
        
        val x1 = leftTop[0].toFloat()
        val y1 = leftTop[1].toFloat()
        val x2 = rightBottom[0].toFloat()
        val y2 = rightBottom[1].toFloat()

        return android.graphics.RectF(
            min(x1, x2),
            min(y1, y2),
            max(x1, x2),
            max(y1, y2)
        )
    }

    /**
     * Map device (screen) coordinates to page (PDF) coordinates.
     */
    fun mapDeviceToPage(
        startX: Int, startY: Int,
        sizeX: Int, sizeY: Int,
        deviceX: Int, deviceY: Int
    ): DoubleArray {
        checkNotClosed()
        return core.mapDeviceToPage(pagePtr, startX, startY, sizeX, sizeY, 0, deviceX, deviceY)
    }

    internal fun getPointer(): Long {
        checkNotClosed()
        return pagePtr
    }
}

