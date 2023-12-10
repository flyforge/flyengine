#pragma once

#include <EditorFramework/DragDrop/DragDropHandler.h>

class plDocument;

class PLASMA_EDITORFRAMEWORK_DLL plAssetDragDropHandler : public plDragDropHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAssetDragDropHandler, plDragDropHandler);

public:
protected:
  bool IsAssetType(const plDragDropInfo* pInfo) const;

  plString GetAssetGuidString(const plDragDropInfo* pInfo) const;

  plUuid GetAssetGuid(const plDragDropInfo* pInfo) const { return plConversionUtils::ConvertStringToUuid(GetAssetGuidString(pInfo)); }

  plString GetAssetsDocumentTypeName(const plUuid& assetTypeGuid) const;

  bool IsSpecificAssetType(const plDragDropInfo* pInfo, const char* szType) const;

  plDocument* m_pDocument;
};
