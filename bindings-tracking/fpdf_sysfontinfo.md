# fpdf_sysfontinfo.h — Missing Kotlin Bindings

## Summary
- Total functions: 6
- Already bound: 6 ✅
- Missing: 0 ✅

## All Functions Complete ✅

All 6 functions from fpdf_sysfontinfo.h are now bound:

- [x] FPDF_GetDefaultTTFMapCount
- [x] FPDF_GetDefaultTTFMapEntry
- [x] FPDF_GetDefaultTTFMap (exposed via Kotlin using count + entry)
- [x] FPDF_AddInstalledFont
- [x] FPDF_GetDefaultSystemFontInfo (returns FPDF_SYSFONTINFO* as Long)
- [x] FPDF_FreeDefaultSystemFontInfo
- [x] FPDF_SetSystemFontInfo

## Notes
FPDF_AddInstalledFont is only useful inside the EnumFonts callback of a
FPDF_SYSFONTINFO struct, but is bound as a simple pass-through for completeness.
FPDF_GetDefaultSystemFontInfo may return NULL on Android/Linux.
