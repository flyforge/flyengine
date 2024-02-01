#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class plVisualScriptClassAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plVisualScriptClassAssetProperties, plReflectedClass);

public:
  plString m_sBaseClass;
  plDynamicArray<plVisualScriptVariable> m_Variables;
  bool m_bDumpAST;
};

class plVisualScriptClassAssetDocument : public plSimpleAssetDocument<plVisualScriptClassAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plVisualScriptClassAssetDocument, plAssetDocument);

public:
  plVisualScriptClassAssetDocument(plStringView sDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;

  virtual void GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const override;
  virtual bool Paste(
    const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable) override;
};
