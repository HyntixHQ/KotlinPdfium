# fpdfview.h — Missing Kotlin Bindings

## Summary
- Total functions: ~57
- Already bound: ~56 ✅
- Missing: ~1 (Skia only)

## Recently Added
- [x] FPDF_LoadMemDocument64 (64-bit size variant)
- [x] FPDF_LoadCustomDocument (FPDF_FILEACCESS callback pattern)
- [x] FPDF_SetSandBoxPolicy
- [x] FPDF_SetPrintMode
- [x] FPDF_GetFileVersion
- [x] FPDF_GetPageWidthF
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
- [x] FPDF_BStr_Init
- [x] FPDF_BStr_Set
- [x] FPDF_BStr_Clear

## Remaining (Will Not Bind)
- [ ] FPDF_RenderPageSkia — Skia-specific, Android uses Agg renderer

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
