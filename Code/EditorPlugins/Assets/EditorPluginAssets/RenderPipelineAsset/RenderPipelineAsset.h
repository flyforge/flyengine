#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class plRenderPipelineNodeManager : public plDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const plDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const plDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const override;

  virtual plStatus InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_result) const override;
};

/// \brief This custom mirror additionally sends over the connection meta data as properties to the engine side so that the graph can be reconstructed there.
/// This is necessary as DocumentNodeManager_DefaultConnection does not contain any data, instead all data is in the DocumentNodeManager_ConnectionMetaData object.
class plRenderPipelineObjectMirrorEditor : public plIPCObjectMirrorEditor
{
  using SUPER = plIPCObjectMirrorEditor;

public:
  void InitNodeSender(const plDocumentNodeManager* pNodeManager);
  void DeInitNodeSender();
  virtual void ApplyOp(plObjectChange& ref_change) override;

private:
  void NodeEventsHandler(const plDocumentNodeManagerEvent& e);
  void SendConnection(const plConnection& connection);

  const plDocumentNodeManager* m_pNodeManager = nullptr;
};

class plRenderPipelineAssetDocument : public plAssetDocument
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRenderPipelineAssetDocument, plAssetDocument);

public:
  plRenderPipelineAssetDocument(plStringView sDocumentPath);
  ~plRenderPipelineAssetDocument();

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const override;
  virtual bool Paste(
    const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable) override;
};
