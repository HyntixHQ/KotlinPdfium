package com.hyntix.pdfium.util

data class Size(val width: Int, val height: Int) {
    override fun toString(): String {
        return "$width x $height"
    }

    override fun equals(other: Any?): Boolean {
        if (other == null) {
            return false
        }
        if (this === other) {
            return true
        }
        if (other is Size) {
            return width == other.width && height == other.height
        }
        return false
    }

    override fun hashCode(): Int {
        return width xor ((height shl 16) or (height ushr 16))
    }
}
