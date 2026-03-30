package com.hyntix.pdfium

/**
 * Represents a digital signature in a PDF document.
 */
data class PdfSignature(
    val index: Int,
    val reason: String,
    val signingTime: String
) {
    /**
     * Whether this signature has a reason specified.
     */
    val hasReason: Boolean get() = reason.isNotBlank()
    
    /**
     * Whether this signature has a signing time specified.
     */
    val hasSigningTime: Boolean get() = signingTime.isNotBlank()
}
