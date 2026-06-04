# fpdf_ext.h — Missing Kotlin Bindings

## Summary
- Total functions in header: 4
- Already bound: 1 (stub returning -1)
- Missing: 3

## Missing Kotlin Bindings

- [ ] FSDK_SetUnSpObjProcessHandler (complex — callback struct)
- [ ] FSDK_SetTimeFunction (complex — function pointer, testing only)
- [ ] FSDK_SetLocaltimeFunction (complex — function pointer, testing only)

## Already Bound (verified)

- [x] FPDFDoc_GetPageMode (stub returning -1)

## Notes

- FSDK_SetTimeFunction and FSDK_SetLocaltimeFunction are testing-only APIs that replace system time functions. They require function pointer callbacks which are complex to implement via JNI.
- FSDK_SetUnSpObjProcessHandler requires a callback struct. Could be implemented with a Java callback interface.
