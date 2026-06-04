# fpdf_edit.h — Missing Kotlin Bindings

## Summary
- Total functions: 67
- Already bound: 67 ✅
- Missing: 0 ✅

## All Functions Complete ✅

All 67 functions from fpdf_edit.h are now bound. The 3 previously-stubbed ImageObj
functions have been upgraded to real implementations:

- [x] FPDFImageObj_GetImageMetadata — reads FPDF_IMAGEOBJ_METADATA struct fields
- [x] FPDFImageObj_LoadJpegFile — uses FPDF_FILEACCESS with SyncFileReadBlock callback
- [x] FPDFImageObj_LoadJpegFileInline — same as LoadJpegFile

### FPDFPageObj_ (30 added)
- [x] FPDFPageObj_AddMark
- [x] FPDFPageObj_CountMarks
- [x] FPDFPageObj_CreateNew
- [x] FPDFPageObj_CreateNewRect
- [x] FPDFPageObj_CreateTextObj
- [x] FPDFPageObj_Destroy
- [x] FPDFPageObj_GetDashArray
- [x] FPDFPageObj_GetDashCount
- [x] FPDFPageObj_GetDashPhase
- [x] FPDFPageObj_GetIsActive
- [x] FPDFPageObj_GetLineCap
- [x] FPDFPageObj_GetLineJoin
- [x] FPDFPageObj_GetMark
- [x] FPDFPageObj_GetMarkedContentID
- [x] FPDFPageObj_GetMatrix
- [x] FPDFPageObj_GetRotatedBounds
- [x] FPDFPageObj_GetStrokeWidth
- [x] FPDFPageObj_HasTransparency
- [x] FPDFPageObj_RemoveMark
- [x] FPDFPageObj_SetBlendMode
- [x] FPDFPageObj_SetDashArray
- [x] FPDFPageObj_SetDashPhase
- [x] FPDFPageObj_SetIsActive
- [x] FPDFPageObj_SetLineCap
- [x] FPDFPageObj_SetLineJoin
- [x] FPDFPageObj_SetMatrix
- [x] FPDFPageObj_TransformF

### FPDFPath_ (3 added)
- [x] FPDFPath_CountSegments
- [x] FPDFPath_GetDrawMode
- [x] FPDFPath_GetPathSegment

### FPDFTextObj_ (5 added)
- [x] FPDFTextObj_GetFont
- [x] FPDFTextObj_GetFontSize
- [x] FPDFTextObj_GetText
- [x] FPDFTextObj_GetTextRenderMode
- [x] FPDFTextObj_SetTextRenderMode

### FPDFImageObj_ (12 added)
- [x] FPDFImageObj_GetBitmap
- [x] FPDFImageObj_GetIccProfileDataDecoded
- [x] FPDFImageObj_GetImageDataDecoded
- [x] FPDFImageObj_GetImageDataRaw
- [x] FPDFImageObj_GetImageFilter
- [x] FPDFImageObj_GetImageFilterCount
- [x] FPDFImageObj_GetImageMetadata
- [x] FPDFImageObj_GetImagePixelSize
- [x] FPDFImageObj_GetRenderedBitmap
- [x] FPDFImageObj_LoadJpegFile
- [x] FPDFImageObj_LoadJpegFileInline
- [x] FPDFImageObj_SetBitmap
- [x] FPDFImageObj_SetMatrix

## Already Bound (verified)
- [x] FPDFPageObj_GetType
- [x] FPDFPageObj_GetBounds
- [x] FPDFPageObj_GetClipPath
- [x] FPDFPageObj_Transform
- [x] FPDFPageObj_GetFillColor
- [x] FPDFPageObj_SetFillColor
- [x] FPDFPageObj_GetStrokeColor
- [x] FPDFPageObj_SetStrokeColor
- [x] FPDFPageObj_SetStrokeWidth
- [x] FPDFPageObj_CreateNewPath
- [x] FPDFPageObj_NewTextObj
- [x] FPDFPageObj_NewImageObj
- [x] FPDFPageObj_AddExistingMark
- [x] FPDFPath_MoveTo
- [x] FPDFPath_LineTo
- [x] FPDFPath_BezierTo
- [x] FPDFPath_Close
- [x] FPDFPath_SetDrawMode
- [x] FPDFTextObj_SetFontSize
