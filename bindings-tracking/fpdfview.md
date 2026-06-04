# fpdfview.h — Missing Kotlin Bindings

## Summary
- Total functions: ~57
- Already bound: ~50 ✅
- Missing: ~7 (Skia + BStr + niche platform functions)

## Missing Kotlin Bindings

- [ ] FPDF_LoadMemDocument64 (64-bit size variant)
- [ ] FPDF_LoadDocumentFromSeekable
- [ ] FPDF_RenderPageSkia (Skia-specific, not useful on Android)
- [ ] FPDF_BStr_Init
- [ ] FPDF_BStr_Set
- [ ] FPDF_BStr_Clear
- [ ] FPDF_GetArrayBufferAllocatorSharedInstance

## Recently Added

- [x] FPDF_SetSandBoxPolicy
- [x] FPDF_SetPrintMode
- [x] FPDF_GetFileVersion
- [x] FPDF_GetPageWidthF (as float*1000000 int)
- [x] FPDF_GetPageHeightF
- [x] FPDF_GetPageBoundingBoxF
- [x] FPDFBitmap_GetFormat
- [x] FPDF_SetDefaultPrinterMode
- [x] FPDF_GetDefaultPrinterMode
- [x] FPDF_GetDuplexOperation
- [x] FPDF_GetRecommendedV8Flags
- [x] FPDF_GetArrayBufferAllocatorSharedInstance
- [x] FPDF_GetXFAPacketCount
- [x] FPDF_GetXFAPacketName
- [x] FPDF_GetXFAPacketContent

## Already Bound (verified)

- [x] FPDF_InitLibrary
- [x] FPDF_DestroyLibrary
- [x] FPDF_GetLastError
- [x] FPDF_GetPageCount
- [x] FPDF_LoadPage
- [x] FPDF_GetPageWidth
- [x] FPDF_GetPageHeight
- [x] FPDF_GetPageBoundingBox
- [x] FPDF_GetPageSizeByIndex
- [x] FPDF_ClosePage
- [x] FPDF_CloseDocument
- [x] FPDF_DeviceToPage
- [x] FPDF_PageToDevice
- [x] FPDFBitmap_CreateEx
- [x] FPDFBitmap_FillRect
- [x] FPDFBitmap_GetBuffer
- [x] FPDFBitmap_GetWidth
- [x] FPDFBitmap_GetHeight
- [x] FPDFBitmap_GetStride
- [x] FPDFBitmap_Destroy
- [x] FPDF_RenderPageBitmap
- [x] FPDF_RenderPageBitmapStart
- [x] FPDF_RenderPageBitmapWithColorScheme_Start
- [x] FPDF_RenderPageContinue
- [x] FPDF_RenderPageClose
