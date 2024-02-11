#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderPipelineAssetDocument, 5, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

bool plRenderPipelineNodeManager::InternalIsNode(const plDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<plRenderPipelinePass>() || pType->IsDerivedFrom<plExtractor>();
}

void plRenderPipelineNodeManager::InternalCreatePins(const plDocumentObject* pObject, NodeInternal& ref_node)
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
      if (plStringUtils::IsEqual(pProp->GetPropertyName(), "DepthStencil"))
      {
        plColorScheme::Enum color = plColorScheme::Pink;
        pinColor = plColorScheme::GetColor(color, 2);
      }
      else
      {
        plColorScheme::Enum color = plColorScheme::Gray;
        pinColor = plColorScheme::GetColor(color, 7);
      }

    }

    if (pProp->GetSpecificType()->IsDerivedFrom<plRenderPipelineNodeInputPin>())
    {
      auto pPin = PL_DEFAULT_NEW(plPin, plPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<plRenderPipelineNodeOutputPin>())
    {
      auto pPin = PL_DEFAULT_NEW(plPin, plPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<plRenderPipelineNodePassThrougPin>())
    {
      auto pPinIn = PL_DEFAULT_NEW(plPin, plPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Inputs.PushBack(pPinIn);
      auto pPinOut = PL_DEFAULT_NEW(plPin, plPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPinOut);
    }
  }
}

void plRenderPipelineNodeManager::GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const
{
  plSet<const plRTTI*> typeSet;
  plReflectionUtils::GatherTypesDerivedFromClass(plGetStaticRTTI<plRenderPipelinePass>(), typeSet);
  plReflectionUtils::GatherTypesDerivedFromClass(plGetStaticRTTI<plExtractor>(), typeSet);
  ref_types.Clear();
  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(plTypeFlags::Abstract))
      continue;

    ref_types.PushBack(pType);
  }
}

plStatus plRenderPipelineNodeManager::InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNto1;
  return plStatus(PL_SUCCESS);
}

plRenderPipelineAssetDocument::plRenderPipelineAssetDocument(plStringView sDocumentPath)
  : plAssetDocument(sDocumentPath, PL_DEFAULT_NEW(plRenderPipelineNodeManager), plAssetDocEngineConnection::FullObjectMirroring)
{
}

plRenderPipelineAssetDocument::~plRenderPipelineAssetDocument()
{
  static_cast<plRenderPipelineObjectMirrorEditor*>(m_pMirror.Borrow())->DeInitNodeSender();
}


void plRenderPipelineAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  m_pMirror = PL_DEFAULT_NEW(plRenderPipelineObjectMirrorEditor);
  static_cast<plRenderPipelineObjectMirrorEditor*>(m_pMirror.Borrow())->InitNodeSender(static_cast<const plDocumentNodeManager*>(GetObjectManager()));
}

plTransformStatus plRenderPipelineAssetDocument::InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
  const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  return plAssetDocument::RemoteExport(AssetHeader, szTargetFile);
}

plTransformStatus plRenderPipelineAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  PL_REPORT_FAILURE("Should not be called");
  return plTransformStatus();
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
  out_MimeTypes.PushBack("application/plEditor.RenderPipelineGraph");
}

bool plRenderPipelineAssetDocument::CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const
{
  out_MimeType = "application/plEditor.RenderPipelineGraph";

  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool plRenderPipelineAssetDocument::Paste(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType)
{
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, plQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

void plRenderPipelineObjectMirrorEditor::InitNodeSender(const plDocumentNodeManager* pNodeManager)
{
  m_pNodeManager = pNodeManager;
  m_pNodeManager->m_NodeEvents.AddEventHandler(plMakeDelegate(&plRenderPipelineObjectMirrorEditor::NodeEventsHandler, this));
}

void plRenderPipelineObjectMirrorEditor::DeInitNodeSender()
{
  m_pNodeManager->m_NodeEvents.RemoveEventHandler(plMakeDelegate(&plRenderPipelineObjectMirrorEditor::NodeEventsHandler, this));
}

void plRenderPipelineObjectMirrorEditor::ApplyOp(plObjectChange& ref_change)
{
  // SUPER::ApplyOp will move the data out of the payload, so we have to check for connections before.
  const plConnection* pConnection = nullptr;
  if (ref_change.m_Change.m_Operation == plObjectChangeType::NodeAdded)
  {
    const plDocumentObject* pObject = m_pNodeManager->GetObject(ref_change.m_Change.m_Value.Get<plUuid>());
    if (pObject != nullptr && m_pNodeManager->IsConnection(pObject))
    {
      pConnection = m_pNodeManager->GetConnectionIfExists(pObject);
    }
  }
  SUPER::ApplyOp(ref_change);

  // We need to handle this case in addition to the NodeEventsHandler because after loading the meta data is restored before the object mirror is initialized so we miss all the NodeEventsHandler calls.
  if (pConnection)
    SendConnection(*pConnection);
}

void plRenderPipelineObjectMirrorEditor::NodeEventsHandler(const plDocumentNodeManagerEvent& e)
{
  if (e.m_EventType == plDocumentNodeManagerEvent::Type::AfterPinsConnected)
  {
    const plConnection& connection = m_pNodeManager->GetConnection(e.m_pObject);
    SendConnection(connection);
  }
}

void plRenderPipelineObjectMirrorEditor::SendConnection(const plConnection& connection)
{
  const plPin& sourcePin = connection.GetSourcePin();
  const plPin& targetPin = connection.GetTargetPin();

  plUuid Source = sourcePin.GetParent()->GetGuid();
  plUuid Target = targetPin.GetParent()->GetGuid();
  plString SourcePin = sourcePin.GetName();
  plString TargetPin = targetPin.GetName();

  auto SendMetaData = [this](const plDocumentObject* pObject, const char* szProperty, plVariant value) {
    plObjectChange change;
    CreatePath(change, pObject, szProperty);
    change.m_Change.m_Operation = plObjectChangeType::PropertySet;
    change.m_Change.m_Value = value;
    ApplyOp(change);
  };
  SendMetaData(connection.GetParent(), "Connection::Source", Source);
  SendMetaData(connection.GetParent(), "Connection::Target", Target);
  SendMetaData(connection.GetParent(), "Connection::SourcePin", SourcePin);
  SendMetaData(connection.GetParent(), "Connection::TargetPin", TargetPin);
}
