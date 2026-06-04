# fpdf_formfill.h — Missing Kotlin Bindings

## Summary
- Total functions: ~26 (excluding struct callbacks)
- Already bound: 13 (event handlers + undo/redo/select/kill)
- Missing: 0 ✅

## Recently Added (24 functions)

### Document Actions
- [x] FORM_DoDocumentJSAction
- [x] FORM_DoDocumentOpenAction
- [x] FORM_DoDocumentAAction
- [x] FORM_DoPageAAction

### Mouse Events
- [x] FORM_OnMouseWheel
- [x] FORM_OnRButtonDown
- [x] FORM_OnRButtonUp
- [x] FORM_OnLButtonDoubleClick

### Text Selection/Replacement
- [x] FORM_GetFocusedText
- [x] FORM_GetSelectedText
- [x] FORM_ReplaceAndKeepSelection
- [x] FORM_ReplaceSelection

### Annotation Focus
- [x] FORM_GetFocusedAnnot
- [x] FORM_SetFocusedAnnot

### Field Queries
- [x] FPDFPage_HasFormFieldAtPoint
- [x] FPDFPage_FormFieldZOrderAtPoint

### Highlight Management
- [x] FPDF_SetFormFieldHighlightColor
- [x] FPDF_SetFormFieldHighlightAlpha
- [x] FPDF_RemoveFormFieldHighlight

### Selection Management
- [x] FORM_SetIndexSelected
- [x] FORM_IsIndexSelected

### XFA
- [x] FPDF_LoadXFA

### Already Bound
- [x] FORM_OnAfterLoadPage
- [x] FORM_OnBeforeClosePage
- [x] FORM_OnMouseMove
- [x] FORM_OnLButtonDown
- [x] FORM_OnLButtonUp
- [x] FORM_OnKeyDown
- [x] FORM_OnKeyUp
- [x] FORM_OnChar
- [x] FORM_OnFocus
- [x] FORM_CanUndo
- [x] FORM_CanRedo
- [x] FORM_Undo
- [x] FORM_Redo
- [x] FORM_SelectAllText
- [x] FORM_ForceToKillFocus
- [x] FPDFDOC_InitFormFillEnvironment
- [x] FPDFDOC_ExitFormFillEnvironment
- [x] FPDF_FFLDraw
- [x] FPDF_GetFormType
