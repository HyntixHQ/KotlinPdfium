package com.hyntix.pdfium

/**
 * Represents an embedded file attachment in a PDF document.
 */
data class PdfAttachment(
    val index: Int,
    val name: String,
    val data: ByteArray?
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is PdfAttachment) return false
        return index == other.index && name == other.name
    }
    
    override fun hashCode(): Int {
        return 31 * index + name.hashCode()
    }
}
