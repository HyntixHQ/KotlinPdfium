# fpdf_sysfontinfo.h — Missing Kotlin Bindings

## Summary
- Total functions: 6
- Already bound: 0
- Missing: 6 (all require callback struct / complex pointer interfaces)

## Missing Kotlin Bindings (Complex — require FPDF_SYSFONTINFO callback struct)

- [ ] FPDF_GetDefaultTTFMap — returns pointer to static array
- [ ] FPDF_GetDefaultTTFMapCount
- [ ] FPDF_GetDefaultTTFMapEntry — returns pointer to struct
- [ ] FPDF_AddInstalledFont — needs opaque mapper pointer
- [ ] FPDF_SetSystemFontInfo — needs FPDF_SYSFONTINFO with function pointers
- [ ] FPDF_GetDefaultSystemFontInfo — returns FPDF_SYSFONTINFO pointer
- [ ] FPDF_FreeDefaultSystemFontInfo

## Notes
All functions in this header require callback function pointers or return pointers to static structs. Low priority for JNI binding.
