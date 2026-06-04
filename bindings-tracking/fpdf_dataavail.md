# fpdf_dataavail.h — Missing Kotlin Bindings

## Summary
- Total functions: 6
- Already bound: 1 (`FPDFAvail_IsLinearized`)
- Missing: 5 (all require callback struct interfaces)

## Missing Kotlin Bindings (Complex — require FX_FILEAVAIL / FX_DOWNLOADHINTS callbacks)

- [ ] FPDFAvail_Create — needs FX_FILEAVAIL callback struct
- [ ] FPDFAvail_Destroy
- [ ] FPDFAvail_IsDocAvail — needs FX_DOWNLOADHINTS callback struct
- [ ] FPDFAvail_GetDocument
- [ ] FPDFAvail_GetFirstPageNum
- [ ] FPDFAvail_IsPageAvail — needs FX_DOWNLOADHINTS callback struct
- [ ] FPDFAvail_IsFormAvail — needs FX_DOWNLOADHINTS callback struct

## Already Bound (verified)

- [x] FPDFAvail_IsLinearized
