# fpdf_ext.h — Missing Kotlin Bindings

## Summary
- Total functions in header: 4
- Already bound: 2
- Remaining unbound: 2

## Recently Added
- [x] FSDK_SetUnSpObjProcessHandler (with UNSUPPORT_INFO callback via JNI static method)

## Remaining (Will Not Bind)
- [ ] FSDK_SetTimeFunction — testing-only, raw fn pointer without context, unsafe from JNI
- [ ] FSDK_SetLocaltimeFunction — testing-only, raw fn pointer without context, unsafe from JNI

## Already Bound (verified)
- [x] FPDFDoc_GetPageMode (stub returning -1, function not in this PDFium build)
- [x] FSDK_SetUnSpObjProcessHandler

## Header Complete (2 test-only functions intentionally skipped)
