#pragma once

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plDocumentObject;
struct plDocumentTypeDescriptor;

class PLASMA_TOOLSFOUNDATION_DLL plDocumentUtils
{
public:
  static plStatus IsValidSaveLocationForDocument(plStringView sDocument, const plDocumentTypeDescriptor** out_pTypeDesc = nullptr);
};
