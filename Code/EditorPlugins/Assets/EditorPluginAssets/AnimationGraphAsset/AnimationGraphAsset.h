#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

using plAnimationClipResourceHandle = plTypedResourceHandle<class plAnimationClipResource>;

class plAnimGraphInstance;
class plAnimGraphNode;

class plAnimationGraphNodePin : public plPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimationGraphNodePin, plPin);

public:
  plAnimationGraphNodePin(Type type, const char* szName, const plColorGammaUB& color, const plDocumentObject* pObject);
  ~plAnimationGraphNodePin();

  bool m_bMultiInputPin = false;
  plAnimGraphPin::Type m_DataType = plAnimGraphPin::Invalid;
};

class plAnimationGraphNodeManager : public plDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const plDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const plDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const override;

  virtual plStatus InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_result) const override;

private:
  virtual bool InternalIsDynamicPinProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp) const override;
};

class plAnimationGraphAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimationGraphAssetProperties, plReflectedClass);

public:
  plDynamicArray<plString> m_IncludeGraphs;
  plDynamicArray<plAnimationClipMapping> m_AnimationClipMapping;
};

class plAnimationGraphAssetDocument : public plSimpleAssetDocument<plAnimationGraphAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimationGraphAssetDocument, plSimpleAssetDocument<plAnimationGraphAssetProperties>);

public:
  plAnimationGraphAssetDocument(plStringView sDocumentPath);

protected:
  struct PinCount
  {
    plUInt16 m_uiInputCount = 0;
    plUInt16 m_uiInputIdx = 0;
    plUInt16 m_uiOutputCount = 0;
    plUInt16 m_uiOutputIdx = 0;
  };

  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const override;
  virtual bool Paste(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable) override;
};
