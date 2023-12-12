#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <GameEngine/StateMachine/StateMachineBuiltins.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineAssetDocument, 3, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachineAssetDocument::plStateMachineAssetDocument(const char* szDocumentPath)
  : plAssetDocument(szDocumentPath, PLASMA_DEFAULT_NEW(plStateMachineNodeManager), plAssetDocEngineConnection::None)
{
}

plTransformStatus plStateMachineAssetDocument::InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  auto pManager = static_cast<plStateMachineNodeManager*>(GetObjectManager());

  plAbstractObjectGraph abstractObjectGraph;
  plDocumentObjectConverterWriter objectWriter(&abstractObjectGraph, pManager);
  plRttiConverterContext converterContext;
  plRttiConverterReader converter(&abstractObjectGraph, &converterContext);

  plStateMachineDescription desc;
  plHashTable<const plDocumentObject*, plUInt32> objectToStateIndex;
  plSet<plString> stateNames;

  auto AddState = [&](const plDocumentObject* pObject) {
    plVariant nameVar = pObject->GetTypeAccessor().GetValue("Name");
    PLASMA_ASSERT_DEV(nameVar.IsA<plString>(), "Implementation error");

    const plString& name = nameVar.Get<plString>();
    if (stateNames.Contains(name))
    {
      return plStatus(plFmt("A state named '{}' already exists. State names have to be unique.", name));
    }
    stateNames.Insert(name);

    plVariant type = pObject->GetTypeAccessor().GetValue("Type");
    PLASMA_ASSERT_DEV(type.IsA<plUuid>(), "Implementation error");

    if (auto pStateObject = pObject->GetChild(type.Get<plUuid>()))
    {
      plAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(pStateObject);
      auto pState = converter.CreateObjectFromNode(pAbstractNode).Cast<plStateMachineState>();
      pState->SetName(name);

      const plUInt32 uiStateIndex = desc.AddState(pState);
      objectToStateIndex.Insert(pObject, uiStateIndex);
    }
    else
    {
      return plStatus(plFmt("State '{}' has no state type assigned", name));
    }

    return plStatus(PLASMA_SUCCESS);
  };

  auto& allObjects = pManager->GetRootObject()->GetChildren();

  if (allObjects.IsEmpty() == false)
  {
    if (auto pObject = pManager->GetInitialState())
    {
      PLASMA_SUCCEED_OR_RETURN(AddState(pObject));
      PLASMA_ASSERT_DEV(objectToStateIndex[pObject] == 0, "Initial state has to have index 0");
    }
    else
    {
      return plStatus("Initial state is not set");
    }
  }

  for (const plDocumentObject* pObject : allObjects)
  {
    if (pManager->IsNode(pObject) == false || pManager->IsInitialState(pObject) || pManager->IsAnyState(pObject))
      continue;

    PLASMA_SUCCEED_OR_RETURN(AddState(pObject));
  }

  for (const plDocumentObject* pObject : allObjects)
  {
    if (pManager->IsConnection(pObject) == false)
      continue;

    plVariant type = pObject->GetTypeAccessor().GetValue("Type");
    PLASMA_ASSERT_DEV(type.IsA<plUuid>(), "Implementation error");

    plUniquePtr<plStateMachineTransition> pTransition;
    if (auto pTransitionObject = pObject->GetChild(type.Get<plUuid>()))
    {
      plAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(pTransitionObject);
      pTransition = converter.CreateObjectFromNode(pAbstractNode).Cast<plStateMachineTransition>();
    }
    else
    {
      pTransition = PLASMA_DEFAULT_NEW(plStateMachineTransition_Timeout);
    }

    const plConnection& connection = pManager->GetConnection(pObject);
    plUInt32 uiFromStateIndex = plInvalidIndex;
    plUInt32 uiToStateIndex = plInvalidIndex;
    if (pManager->IsAnyState(connection.GetSourcePin().GetParent()) == false)
    {
      PLASMA_VERIFY(objectToStateIndex.TryGetValue(connection.GetSourcePin().GetParent(), uiFromStateIndex), "Implementation error");
    }
    PLASMA_VERIFY(objectToStateIndex.TryGetValue(connection.GetTargetPin().GetParent(), uiToStateIndex), "Implementation error");

    desc.AddTransition(uiFromStateIndex, uiToStateIndex, std::move(pTransition));
  }

  plDefaultMemoryStreamStorage storage;
  plMemoryStreamWriter writer(&storage);
  PLASMA_SUCCEED_OR_RETURN(desc.Serialize(stream));

  stream << storage.GetStorageSize32();
  return storage.CopyToStream(stream);
}

constexpr const char* s_szIsInitialState = "IsInitialState";

void plStateMachineAssetDocument::InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const
{
  auto pManager = static_cast<const plStateMachineNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);

  if (pManager->IsInitialState(pObject))
  {
    inout_uiHash = plHashingUtils::xxHash64String(s_szIsInitialState, inout_uiHash);
  }
}

void plStateMachineAssetDocument::AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const auto pManager = static_cast<const plStateMachineNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);

  if (auto pObject = pManager->GetInitialState())
  {
    plAbstractObjectNode* pAbstractObject = graph.GetNode(pObject->GetGuid());
    pAbstractObject->AddProperty(s_szIsInitialState, true);
  }
}

void plStateMachineAssetDocument::RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  auto pManager = static_cast<plStateMachineNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);

  for (auto it : graph.GetAllNodes())
  {
    auto pAbstractObject = it.Value();
    if (auto pProperty = pAbstractObject->FindProperty(s_szIsInitialState))
    {
      if (pProperty->m_Value.ConvertTo<bool>() == false)
        continue;

      plDocumentObject* pObject = pManager->GetObject(pAbstractObject->GetGuid());
      pManager->SetInitialState(pObject);
    }
  }
}

void plStateMachineAssetDocument::GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/PlasmaEditor.StateMachineGraph");
}

bool plStateMachineAssetDocument::CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const
{
  out_MimeType = "application/PlasmaEditor.StateMachineGraph";

  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool plStateMachineAssetDocument::Paste(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, plQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
