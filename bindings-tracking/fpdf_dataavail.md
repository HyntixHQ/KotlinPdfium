# fpdf_dataavail.h — Missing Kotlin Bindings

## Summary
- Total functions: 6
- Already bound: 6 ✅
- Missing: 0 ✅

## All Functions Complete ✅

All 6 functions from fpdf_dataavail.h are now bound:

### Recently Added
- [x] FPDFAvail_Create — with FPDF_FILEACCESS buffer + FX_FILEAVAIL callbacks
- [x] FPDFAvail_Destroy
- [x] FPDFAvail_GetDocument
- [x] FPDFAvail_GetFirstPageNum
- [x] FPDFAvail_IsDocAvail (passes NULL for hints — basic check)
- [x] FPDFAvail_IsPageAvail (passes NULL for hints — basic check)
- [x] FPDFAvail_IsFormAvail (passes NULL for hints — basic check)

### Already Bound (verified)
- [x] FPDFAvail_IsLinearized

## Notes
- For FPDFAvail_Create, the file data is provided upfront as ByteArray.
  FX_FILEAVAIL.IsDataAvail always returns true (all data available).
  Download hints (FX_DOWNLOADHINTS) are not populated since all data is present.
- For IsDocAvail, IsPageAvail, IsFormAvail: passing NULL for FX_DOWNLOADHINTS
  still allows basic availability checking — hints just won't be populated.
