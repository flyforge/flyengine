#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>

class plPin;

class plProcGenGraphAssetDocument : public plAssetDocument
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGenGraphAssetDocument, plAssetDocument);

public:
  plProcGenGraphAssetDocument(plStringView sDocumentPath);

  void SetDebugPin(const plPin* pDebugPin);

  plStatus WriteAsset(plStreamWriter& inout_stream, const plPlatformProfile* pAssetProfile, bool bAllowDebug) const;

protected:
  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const override;
  virtual bool Paste(
    const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType) override;

  virtual void AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable) override;

  void GetAllOutputNodes(plDynamicArray<const plDocumentObject*>& placementNodes, plDynamicArray<const plDocumentObject*>& vertexColorNodes) const;

private:
  friend class plProcGenAction;

  virtual void InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const override;

  struct GenerateContext;

  plExpressionAST::Node* GenerateExpressionAST(const plDocumentObject* outputNode, const char* szOutputName, GenerateContext& context, plExpressionAST& out_Ast) const;
  plExpressionAST::Node* GenerateDebugExpressionAST(GenerateContext& context, plExpressionAST& out_Ast) const;

  void DumpSelectedOutput(bool bAst, bool bDisassembly) const;

  void CreateDebugNode();

  const plPin* m_pDebugPin = nullptr;
  plUniquePtr<plProcGen_PlacementOutput> m_pDebugNode;
};
