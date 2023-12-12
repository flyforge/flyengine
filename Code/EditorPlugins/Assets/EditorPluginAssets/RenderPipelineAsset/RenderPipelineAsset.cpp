#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderPipelineAssetDocument, 4, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

bool plRenderPipelineNodeManager::InternalIsNode(const plDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<plRenderPipelinePass>() || pType->IsDerivedFrom<plExtractor>();
}

void plRenderPipelineNodeManager::InternalCreatePins(const plDocumentObject* pObject, NodeInternal& node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<plRenderPipelinePass>())
    return;

  plHybridArray<const plAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != plPropertyCategory::Member)
      continue;

    if (!pProp->GetSpecificType()->IsDerivedFrom<plRenderPipelineNodePin>())
      continue;

    plColor pinColor;
    if (const plColorAttribute* pAttr = pProp->GetAttributeByType<plColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }
    else
    {
      plColorScheme::Enum color = plColorScheme::Gray;
      if (plStringUtils::IsEqual(pProp->GetPropertyName(), "DepthStencil"))
        color = plColorScheme::Pink;

      pinColor = plColorScheme::DarkUI(color);
    }

    if (pProp->GetSpecificType()->IsDerivedFrom<plRenderPipelineNodeInputPin>())
    {
      auto pPin = PLASMA_DEFAULT_NEW(plPin, plPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<plRenderPipelineNodeOutputPin>())
    {
      auto pPin = PLASMA_DEFAULT_NEW(plPin, plPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<plRenderPipelineNodePassThrougPin>())
    {
      auto pPinIn = PLASMA_DEFAULT_NEW(plPin, plPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Inputs.PushBack(pPinIn);
      auto pPinOut = PLASMA_DEFAULT_NEW(plPin, plPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Outputs.PushBack(pPinOut);
    }
  }
}

void plRenderPipelineNodeManager::GetCreateableTypes(plHybridArray<const plRTTI*, 32>& Types) const
{
  plSet<const plRTTI*> typeSet;
  plReflectionUtils::GatherTypesDerivedFromClass(plGetStaticRTTI<plRenderPipelinePass>(), typeSet);
  plReflectionUtils::GatherTypesDerivedFromClass(plGetStaticRTTI<plExtractor>(), typeSet);
  Types.Clear();
  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(plTypeFlags::Abstract))
      continue;

    Types.PushBack(pType);
  }
}

plStatus plRenderPipelineNodeManager::InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNto1;
  return plStatus(PLASMA_SUCCESS);
}

plRenderPipelineAssetDocument::plRenderPipelineAssetDocument(const char* szDocumentPath)
  : plAssetDocument(szDocumentPath, PLASMA_DEFAULT_NEW(plRenderPipelineNodeManager), plAssetDocEngineConnection::None)
{
}

plTransformStatus plRenderPipelineAssetDocument::InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());

  const plUInt8 uiVersion = 1;
  stream << uiVersion;

  plAbstractObjectGraph graph;
  plDocumentObjectConverterWriter objectConverter(&graph, GetObjectManager());

  auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (plDocumentObject* pObject : children)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    if (pType->IsDerivedFrom<plRenderPipelinePass>())
    {
      objectConverter.AddObjectToGraph(pObject, "Pass");
    }
    else if (pType->IsDerivedFrom<plExtractor>())
    {
      objectConverter.AddObjectToGraph(pObject, "Extractor");
    }
    else if (pManager->IsConnection(pObject))
    {
      objectConverter.AddObjectToGraph(pObject, "Connection");
    }
  }

  pManager->AttachMetaDataBeforeSaving(graph);

  plDefaultMemoryStreamStorage storage;
  plMemoryStreamWriter writer(&storage);
  plAbstractGraphBinarySerializer::Write(writer, &graph);

  plUInt32 uiSize = storage.GetStorageSize32();
  stream << uiSize;
  return storage.CopyToStream(stream);
}

void plRenderPipelineAssetDocument::InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const
{
  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void plRenderPipelineAssetDocument::AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void plRenderPipelineAssetDocument::RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void plRenderPipelineAssetDocument::GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/PlasmaEditor.RenderPipelineGraph");
}

bool plRenderPipelineAssetDocument::CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const
{
  out_MimeType = "application/PlasmaEditor.RenderPipelineGraph";

  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool plRenderPipelineAssetDocument::Paste(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, plQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
