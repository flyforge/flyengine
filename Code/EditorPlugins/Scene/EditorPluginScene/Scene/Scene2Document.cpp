#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneLayerBase, 1, plRTTINoAllocator)
{
  //PLASMA_BEGIN_PROPERTIES
  //{
  //}
  //PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneLayer, 1, plRTTIDefaultAllocator<plSceneLayer>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Layer", m_Layer)
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneDocumentSettings, 2, plRTTIDefaultAllocator<plSceneDocumentSettings>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Layers", m_Layers)->AddFlags(plPropertyFlags::PointerOwner)
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plScene2Document, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


plSceneLayerBase::plSceneLayerBase()
{
}

plSceneLayerBase::~plSceneLayerBase()
{
}

//////////////////////////////////////////////////////////////////////////

plSceneLayer::plSceneLayer()
{
}

plSceneLayer::~plSceneLayer()
{
}

//////////////////////////////////////////////////////////////////////////

plSceneDocumentSettings::plSceneDocumentSettings()
{
}

plSceneDocumentSettings::~plSceneDocumentSettings()
{
  for (plSceneLayerBase* pLayer : m_Layers)
  {
    PLASMA_DEFAULT_DELETE(pLayer);
  }
}

plScene2Document::plScene2Document(const char* szDocumentPath)
  : plSceneDocument(szDocumentPath, plSceneDocument::DocumentType::Scene)
{
  // Separate selection for the layer panel.
  m_pLayerSelection = PLASMA_DEFAULT_NEW(plSelectionManager, m_pObjectManager.Borrow());
}

plScene2Document::~plScene2Document()
{
  SetActiveLayer(GetGuid()).LogFailure();

  // We need to clear all things that are dependent in the current object manager, selection etc setup before we swap the managers as otherwise those will fail to de-register.
  plVisualizerManager::GetSingleton()->SetVisualizersActive(this, false);
  m_pSelectionManager->Clear();

  // Game object document subscribed to the true document originally but we rerouted that to the mock data.
  // In order to destroy the game object document we need to revert this and subscribe to the true document again.
  UnsubscribeGameObjectEventHandlers();

  // Move the preserved real scene document back.
  m_pSelectionManager = std::move(m_pSceneSelectionManager);
  m_pCommandHistory = std::move(m_pSceneCommandHistory);
  m_pObjectManager = std::move(m_pSceneObjectManager);
  m_pObjectAccessor = std::move(m_pSceneObjectAccessor);
  m_DocumentObjectMetaData = std::move(m_pSceneDocumentObjectMetaData);
  m_GameObjectMetaData = std::move(m_pSceneGameObjectMetaData);

  // See comment above for UnsubscribeGameObjectEventHandlers.
  SubscribeGameObjectEventHandlers();

  m_DocumentManagerEventSubscriber.Unsubscribe();
  m_LayerSelectionEventSubscriber.Unsubscribe();
  m_StructureEventSubscriber.Unsubscribe();
  m_CommandHistoryEventSubscriber.Unsubscribe();

  m_pLayerSelection = nullptr;

  for (auto it : m_Layers)
  {
    auto pDoc = it.Value().m_pLayer;

    if (pDoc && pDoc != this)
    {
      plDocumentManager* pManager = pDoc->GetDocumentManager();
      pManager->CloseDocument(pDoc);
    }
  }
}

void plScene2Document::InitializeAfterLoading(bool bFirstTimeCreation)
{
  EnsureSettingsObjectExist();

  m_ActiveLayerGuid = GetGuid();
  plObjectDirectAccessor accessor(GetObjectManager());
  plObjectAccessorBase* pAccessor = &accessor;
  auto pRoot = GetObjectManager()->GetObject(GetSettingsObject()->GetGuid());
  if (pRoot->GetChildren().IsEmpty())
  {
    plUuid objectGuid;
    pAccessor->AddObject(pRoot, "Layers", 0, plGetStaticRTTI<plSceneLayer>(), objectGuid).IgnoreResult();
    const plDocumentObject* pObject = pAccessor->GetObject(objectGuid);
    pAccessor->SetValue(pObject, "Layer", GetGuid()).IgnoreResult();
  }

  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

void plScene2Document::InitializeAfterLoadingAndSaving()
{
  m_pLayerSelection->m_Events.AddEventHandler(plMakeDelegate(&plScene2Document::LayerSelectionEventHandler, this), m_LayerSelectionEventSubscriber);
  m_pObjectManager->m_StructureEvents.AddEventHandler(plMakeDelegate(&plScene2Document::StructureEventHandler, this), m_StructureEventSubscriber);
  m_pCommandHistory->m_Events.AddEventHandler(plMakeDelegate(&plScene2Document::CommandHistoryEventHandler, this), m_CommandHistoryEventSubscriber);
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plScene2Document::DocumentManagerEventHandler, this), m_DocumentManagerEventSubscriber);

  SUPER::InitializeAfterLoadingAndSaving();

  // Game object document subscribed to the true document originally but want to reroute it to the mock data so that is picks up the layer content on its own.
  // Therefore we need to unsubscribe the original subscriptions and replace them with ones to the mock data.
  UnsubscribeGameObjectEventHandlers();

  // These preserve the real scene document.
  m_pSceneObjectManager = std::move(m_pObjectManager);
  m_pSceneCommandHistory = std::move(m_pCommandHistory);
  m_pSceneSelectionManager = std::move(m_pSelectionManager);
  m_pSceneObjectAccessor = std::move(m_pObjectAccessor);
  m_pSceneDocumentObjectMetaData = std::move(m_DocumentObjectMetaData);
  m_pSceneGameObjectMetaData = std::move(m_GameObjectMetaData);

  // Replace real scene elements with copies.
  m_pObjectManager = PLASMA_DEFAULT_NEW(plSceneObjectManager);
  m_pObjectManager->SetDocument(this);
  m_pObjectManager->SwapStorage(m_pSceneObjectManager->GetStorage());
  m_pCommandHistory = PLASMA_DEFAULT_NEW(plCommandHistory, this);
  m_pCommandHistory->SwapStorage(m_pSceneCommandHistory->GetStorage());
  m_pSelectionManager = PLASMA_DEFAULT_NEW(plSelectionManager, m_pSceneObjectManager.Borrow());
  m_pSelectionManager->SwapStorage(m_pSceneSelectionManager->GetStorage());
  m_pObjectAccessor = PLASMA_DEFAULT_NEW(plObjectCommandAccessor, m_pCommandHistory.Borrow());
  using ObjectMetaData = plObjectMetaData<plUuid, plDocumentObjectMetaData>;
  m_DocumentObjectMetaData = PLASMA_DEFAULT_NEW(ObjectMetaData);
  m_DocumentObjectMetaData->SwapStorage(m_pSceneDocumentObjectMetaData->GetStorage());
  using GameObjectMetaData = plObjectMetaData<plUuid, plGameObjectMetaData>;
  m_GameObjectMetaData = PLASMA_DEFAULT_NEW(GameObjectMetaData);
  m_GameObjectMetaData->SwapStorage(m_pSceneGameObjectMetaData->GetStorage());

  // See comment above for UnsubscribeGameObjectEventHandlers.
  SubscribeGameObjectEventHandlers();

  UpdateLayers();
  if (const plDocumentObject* pLayerObject = GetLayerObject(GetActiveLayer()))
  {
    m_pLayerSelection->SetSelection(pLayerObject);
  }
}

const plDocumentObject* plScene2Document::GetSettingsObject() const
{
  /// This function is overwritten so that after redirecting to the active document this still accesses the original content and is not redirected.
  if (m_pSceneObjectManager == nullptr)
    return SUPER::GetSettingsObject();

  auto pRoot = GetSceneObjectManager()->GetRootObject();
  plVariant value;
  PLASMA_VERIFY(GetSceneObjectAccessor()->GetValue(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  plUuid id = value.Get<plUuid>();
  return GetSceneObjectManager()->GetObject(id);
}

void plScene2Document::HandleEngineMessage(const PlasmaEditorEngineDocumentMsg* pMsg)
{
  if (const plPushObjectStateMsgToEditor* msg = plDynamicCast<const plPushObjectStateMsgToEditor*>(pMsg))
  {
    HandleObjectStateFromEngineMsg2(msg);
    return;
  }

  SUPER::HandleEngineMessage(pMsg);
}

plTaskGroupID plScene2Document::InternalSaveDocument(AfterSaveCallback callback)
{
  // We need to switch the active layer back to the original content as otherwise the scene will not save itself but instead the active layer's content into itself.
  SetActiveLayer(GetGuid()).LogFailure();
  return SUPER::InternalSaveDocument(callback);
}

void plScene2Document::SendGameWorldToEngine()
{
  SUPER::SendGameWorldToEngine();
  for (auto layer : m_Layers)
  {
    plSceneDocument* pLayer = layer.Value().m_pLayer;
    if (pLayer != this && pLayer != nullptr)
    {
      PlasmaEditorEngineProcessConnection::GetSingleton()->SendDocumentOpenMessage(pLayer, true);
    }
  }
}

void plScene2Document::LayerSelectionEventHandler(const plSelectionManagerEvent& e)
{
  const plDocumentObject* pObject = m_pLayerSelection->GetCurrentObject();
  // We can't change the active layer while a transaction is in progress at it will swap out the data storage the transaction is currently modifying.
  if (pObject && !m_pCommandHistory->IsInTransaction() && !m_pSceneCommandHistory->IsInTransaction())
  {
    if (pObject->GetType()->IsDerivedFrom(plGetStaticRTTI<plSceneLayer>()))
    {
      plUuid layerGuid = GetSceneObjectAccessor()->Get<plUuid>(pObject, "Layer");
      if (IsLayerLoaded(layerGuid))
      {
        SetActiveLayer(layerGuid).LogFailure();
      }
    }
  }
}

void plScene2Document::StructureEventHandler(const plDocumentObjectStructureEvent& e)
{
}

void plScene2Document::CommandHistoryEventHandler(const plCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
    case plCommandHistoryEvent::Type::UndoEnded:
    case plCommandHistoryEvent::Type::RedoEnded:
    case plCommandHistoryEvent::Type::TransactionEnded:
      UpdateLayers();
      break;
    default:
      return;
  }
}

void plScene2Document::DocumentManagerEventHandler(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentOpened:
    {
      if (plLayerDocument* pLayer = plDynamicCast<plLayerDocument*>(e.m_pDocument))
      {
        if (pLayer->GetMainDocument() != this)
          return;

        plUuid layerGuid = e.m_pDocument->GetGuid();
        LayerInfo* pInfo = nullptr;
        // Either the layer is currently being creating, in which case m_Layers can't be filled yet,
        // or an existing layer's state is toggled either internally by the scene or externally by the editor in which case the layer is known and we must react to it.
        if (m_Layers.TryGetValue(layerGuid, pInfo) && pInfo->m_pLayer != pLayer)
        {
          pInfo->m_pLayer = pLayer;

          plScene2LayerEvent e;
          e.m_Type = plScene2LayerEvent::Type::LayerLoaded;
          e.m_layerGuid = layerGuid;
          m_LayerEvents.Broadcast(e);
        }
      }
    }
    break;
    case plDocumentManager::Event::Type::DocumentClosing:
    {
      if (e.m_pDocument->GetDynamicRTTI()->IsDerivedFrom<plLayerDocument>())
      {
        plUuid layerGuid = e.m_pDocument->GetGuid();
        LayerInfo* pInfo = nullptr;
        if (m_Layers.TryGetValue(layerGuid, pInfo))
        {
          pInfo->m_pLayer = nullptr;

          plScene2LayerEvent e;
          e.m_Type = plScene2LayerEvent::Type::LayerUnloaded;
          e.m_layerGuid = layerGuid;
          m_LayerEvents.Broadcast(e);
        }
      }
    }
    break;
    default:
      break;
  }
}

void plScene2Document::HandleObjectStateFromEngineMsg2(const plPushObjectStateMsgToEditor* pMsg)
{
  plMap<plUuid, plHybridArray<const plPushObjectStateData*, 1>> layerToChanges;
  for (const plPushObjectStateData& change : pMsg->m_ObjectStates)
  {
    layerToChanges[change.m_LayerGuid].PushBack(&change);
  }

  const plUuid activeLayer = m_ActiveLayerGuid;
  for (auto it : layerToChanges)
  {
    if (SetActiveLayer(it.Key()).Failed())
      continue;

    auto pHistory = GetCommandHistory();

    pHistory->StartTransaction("Pull Object State");

    for (const plPushObjectStateData* pState : it.Value())
    {
      auto pObject = GetObjectManager()->GetObject(pState->m_ObjectGuid);

      if (!pObject)
        continue;

      // set the general transform of the object
      SetGlobalTransform(pObject, plTransform(pState->m_vPosition, pState->m_qRotation), TransformationChanges::Translation | TransformationChanges::Rotation);

      // if we also have bone transforms, attempt to set them as well
      if (pState->m_BoneTransforms.IsEmpty())
        continue;

      auto pAccessor = GetObjectAccessor();

      // check all components
      for (auto pComponent : pObject->GetChildren())
      {
        auto pComponentType = pComponent->GetType();

        const auto* pBoneManipAttr = pComponentType->GetAttributeByType<plBoneManipulatorAttribute>();

        // we can only apply bone transforms on components that have the plBoneManipulatorAttribute attribute
        if (pBoneManipAttr == nullptr)
          continue;

        auto pBonesProperty = pComponentType->FindPropertyByName(pBoneManipAttr->GetTransformProperty());
        PLASMA_ASSERT_DEBUG(pBonesProperty, "Invalid transform property set on plBoneManipulatorAttribute");

        const plExposedParametersAttribute* pExposedParamsAttr = pBonesProperty->GetAttributeByType<plExposedParametersAttribute>();
        PLASMA_ASSERT_DEBUG(pExposedParamsAttr, "Expected exposed parameters on plBoneManipulatorAttribute property");

        const plAbstractProperty* pParameterSourceProp = pComponentType->FindPropertyByName(pExposedParamsAttr->GetParametersSource());
        PLASMA_ASSERT_DEBUG(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pExposedParamsAttr->GetParametersSource(), pComponentType->GetTypeName());

        // retrieve all the bone keys and values, these will contain the exposed default values, in case a bone has never been overridden before
        plVariantArray boneValues, boneKeys;
        plExposedParameterCommandAccessor proxy(pAccessor, pBonesProperty, pParameterSourceProp);
        proxy.GetValues(pComponent, pBonesProperty, boneValues).IgnoreResult();
        proxy.GetKeys(pComponent, pBonesProperty, boneKeys).IgnoreResult();

        // apply all the new bone transforms
        for (const auto& bone : pState->m_BoneTransforms)
        {
          // ignore bones that are unknown (not exposed somehow)
          plUInt32 idx = boneKeys.IndexOf(bone.Key());
          if (idx == plInvalidIndex)
            continue;

          PLASMA_ASSERT_DEBUG(boneValues[idx].GetReflectedType() == plGetStaticRTTI<plExposedBone>(), "Expected an plExposedBone in variant");

          // retrieve the default/previous value of the bone
          const plExposedBone* pDefVal = reinterpret_cast<const plExposedBone*>(boneValues[idx].GetData());

          plExposedBone b;
          b.m_sName = pDefVal->m_sName;     // same as the key
          b.m_sParent = pDefVal->m_sParent; // this is what we don't have and therefore needed to retrieve the default values
          b.m_Transform = bone.Value();

          plVariant var;
          var.CopyTypedObject(&b, plGetStaticRTTI<plExposedBone>());

          proxy.SetValue(pComponent, pBonesProperty, var, bone.Key()).IgnoreResult();
        }

        // found a component/property to apply bones to, so we can stop
        break;
      }
    }

    pHistory->FinishTransaction();
  }
  SetActiveLayer(activeLayer).IgnoreResult();
}

void plScene2Document::UpdateLayers()
{
  plSet<plUuid> layersBefore;
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    layersBefore.Insert(it.Key());
  }
  plSet<plUuid> layersAfter;
  plMap<plUuid, plUuid> LayerToSceneObject;
  const plSceneDocumentSettings* pSettings = GetSettings<plSceneDocumentSettings>();
  for (const plSceneLayerBase* pLayerBase : pSettings->m_Layers)
  {
    if (const plSceneLayer* pLayer = plDynamicCast<const plSceneLayer*>(pLayerBase))
    {
      layersAfter.Insert(pLayer->m_Layer);
      plUuid objectGuid = m_Context.GetObjectGUID(plGetStaticRTTI<plSceneLayer>(), pLayer);
      LayerToSceneObject.Insert(pLayer->m_Layer, objectGuid);
    }
  }

  plSet<plUuid> layersRemoved = layersBefore;
  layersRemoved.Difference(layersAfter);
  for (auto it = layersRemoved.GetIterator(); it.IsValid(); ++it)
  {
    LayerRemoved(it.Key());
  }

  plSet<plUuid> layersAdded = layersAfter;
  layersAdded.Difference(layersBefore);
  for (auto it = layersAdded.GetIterator(); it.IsValid(); ++it)
  {
    LayerAdded(it.Key(), LayerToSceneObject[it.Key()]);
  }
}

void plScene2Document::SendLayerVisibility()
{
  plLayerVisibilityChangedMsgToEngine msg;
  for (auto& layer : m_Layers)
  {
    if (!layer.Value().m_bVisible)
    {
      // We are sending the hidden state because the default state is visible so we have less to send and less often.
      msg.m_HiddenLayers.PushBack(layer.Key());
    }
  }
  SendMessageToEngine(&msg);
}

void plScene2Document::LayerAdded(const plUuid& layerGuid, const plUuid& layerObjectGuid)
{
  LayerInfo info;
  info.m_pLayer = nullptr;
  info.m_bVisible = true;
  info.m_objectGuid = layerObjectGuid;
  m_Layers.Insert(layerGuid, info);

  plScene2LayerEvent e;
  e.m_Type = plScene2LayerEvent::Type::LayerAdded;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  //#TODO Decide whether to load a layer or not (persist as meta data? / user preferences?)
  SetLayerLoaded(layerGuid, true).LogFailure();
}

void plScene2Document::LayerRemoved(const plUuid& layerGuid)
{
  // Make sure removed layer is not active
  if (m_ActiveLayerGuid == layerGuid)
  {
    SetActiveLayer(GetGuid()).LogFailure();
  }

  SetLayerLoaded(layerGuid, false).LogFailure();

  plScene2LayerEvent e;
  e.m_Type = plScene2LayerEvent::Type::LayerRemoved;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  m_Layers.Remove(layerGuid);
}

plStatus plScene2Document::CreateLayer(const char* szName, plUuid& out_layerGuid)
{
  // We need to be the active layer in order to make changes to the layers.
  plStatus res = SetActiveLayer(GetGuid());
  if (res.Failed())
    return res;

  const plDocumentTypeDescriptor* pLayerDesc = plDocumentManager::GetDescriptorForDocumentType("Layer");

  plStringBuilder targetDirectory = GetDocumentPath();
  targetDirectory.RemoveFileExtension();
  targetDirectory.Append("_data");
  targetDirectory.AppendPath(szName);
  targetDirectory.Append(".", pLayerDesc->m_sFileExtension.GetData());

  plSceneDocument* pLayerDoc = nullptr;
  if (plOSFile::ExistsFile(targetDirectory))
  {
    plDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    pLayerDoc = plDynamicCast<plSceneDocument*>(plQtEditorApp::GetSingleton()->OpenDocument(targetDirectory, plDocumentFlags::None, pRoot));

    if (m_Layers.Contains(pLayerDoc->GetGuid()))
    {
      return plStatus(plFmt("A layer named '{}' already exists in this scene.", szName));
    }
  }
  else
  {
    plDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    pLayerDoc = plDynamicCast<plSceneDocument*>(plQtEditorApp::GetSingleton()->CreateDocument(targetDirectory, plDocumentFlags::None, pRoot));
    if (!pLayerDoc)
    {
      return plStatus(plFmt("Failed to create new layer '{0}'", targetDirectory));
    }
  }

  plObjectAccessorBase* pAccessor = GetSceneObjectAccessor();
  plStringBuilder sTransactionText;
  pAccessor->StartTransaction(plFmt("Add Layer - '{}'", szName).GetTextCStr(sTransactionText));
  {
    auto pRoot = m_pSceneObjectManager->GetObject(GetSettingsObject()->GetGuid());
    plInt32 uiCount = 0;
    PLASMA_VERIFY(pAccessor->GetCount(pRoot, "Layers", uiCount).Succeeded(), "Failed to get layer count.");
    plUuid sceneLayerGuid;
    PLASMA_VERIFY(pAccessor->AddObject(pRoot, "Layers", uiCount, plGetStaticRTTI<plSceneLayer>(), sceneLayerGuid).Succeeded(), "Failed to add layer to scene.");
    auto pLayer = pAccessor->GetObject(sceneLayerGuid);
    PLASMA_VERIFY(pAccessor->SetValue(pLayer, "Layer", pLayerDoc->GetGuid()).Succeeded(), "Failed to set layer GUID.");
  }
  pAccessor->FinishTransaction();

  LayerInfo* pInfo = nullptr;
  PLASMA_ASSERT_DEV(m_Layers.Contains(pLayerDoc->GetGuid()), "FinishTransaction should have triggered UpdateLayers and filled m_Layers.");
  // We need to manually emit this here as when the layer doc was loaded DocumentManagerEventHandler will not fire as the document was not added as a layer yet.
  if (m_Layers.TryGetValue(pLayerDoc->GetGuid(), pInfo) && pInfo->m_pLayer != pLayerDoc)
  {
    pInfo->m_pLayer = pLayerDoc;

    plScene2LayerEvent e;
    e.m_Type = plScene2LayerEvent::Type::LayerLoaded;
    e.m_layerGuid = pLayerDoc->GetGuid();
    m_LayerEvents.Broadcast(e);
  }
  out_layerGuid = pLayerDoc->GetGuid();
  return plStatus(PLASMA_SUCCESS);
}

plStatus plScene2Document::DeleteLayer(const plUuid& layerGuid)
{
  // We need to be the active layer in order to make changes to the layers.
  plStatus res = SetActiveLayer(GetGuid());
  if (res.Failed())
    return res;

  LayerInfo* pInfo = nullptr;
  if (!m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return plStatus("Unknown layer guid. Layer can't be deleted.");
  }

  if (!pInfo->m_objectGuid.IsValid())
  {
    return plStatus("Layer object guid not set, layer object unknown.");
  }

  const plDocumentObject* pObject = GetSceneObjectManager()->GetObject(pInfo->m_objectGuid);
  if (!pObject)
  {
    return plStatus("Layer object no longer valid.");
  }

  plStringBuilder sName("<Unknown>");
  {
    auto assetInfo = plAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
    if (assetInfo.isValid())
    {
      sName = plPathUtils::GetFileName(assetInfo->m_pAssetInfo->m_sDataDirParentRelativePath);
    }
    else
    {
      return plStatus("Could not resolve layer in plAssetCurator.");
    }
  }

  plObjectAccessorBase* pAccessor = GetSceneObjectAccessor();
  plStringBuilder sTransactionText;
  pAccessor->StartTransaction(plFmt("Remove Layer - '{}'", sName).GetTextCStr(sTransactionText));
  {
    PLASMA_VERIFY(pAccessor->RemoveObject(pObject).Succeeded(), "Failed to remove Layer.");
  }
  pAccessor->FinishTransaction();
  return plStatus(PLASMA_SUCCESS);
}

const plUuid& plScene2Document::GetActiveLayer() const
{
  return m_ActiveLayerGuid;
}

plStatus plScene2Document::SetActiveLayer(const plUuid& layerGuid)
{
  PLASMA_ASSERT_DEV(!m_pCommandHistory->IsInTransaction(), "Active layer must not be changed while an operation is in progress.");
  PLASMA_ASSERT_DEV(!m_pSceneCommandHistory || !m_pSceneCommandHistory->IsInTransaction(), "Active layer must not be changed while an operation is in progress.");

  if (layerGuid == m_ActiveLayerGuid)
    return plStatus(PLASMA_SUCCESS);

  if (layerGuid == GetGuid())
  {
    plDocumentObjectStructureEvent e;
    e.m_pDocument = this;
    e.m_EventType = plDocumentObjectStructureEvent::Type::BeforeReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);

    m_pObjectManager->SwapStorage(m_pSceneObjectManager->GetStorage());
    m_pCommandHistory->SwapStorage(m_pSceneCommandHistory->GetStorage());
    m_pSelectionManager->SwapStorage(m_pSceneSelectionManager->GetStorage());
    m_DocumentObjectMetaData->SwapStorage(m_pSceneDocumentObjectMetaData->GetStorage());
    m_GameObjectMetaData->SwapStorage(m_pSceneGameObjectMetaData->GetStorage());
    // m_pSceneObjectAccessor does not need to be modified

    e.m_EventType = plDocumentObjectStructureEvent::Type::AfterReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);
  }
  else
  {
    plDocument* pDoc = plDocumentManager::GetDocumentByGuid(layerGuid);
    if (!pDoc)
      return plStatus("Unloaded layer can't be made active.");

    plDocumentObjectStructureEvent e;
    e.m_pDocument = this;
    e.m_EventType = plDocumentObjectStructureEvent::Type::BeforeReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);

    m_pObjectManager->SwapStorage(pDoc->GetObjectManager()->GetStorage());
    m_pCommandHistory->SwapStorage(pDoc->GetCommandHistory()->GetStorage());
    m_pSelectionManager->SwapStorage(pDoc->GetSelectionManager()->GetStorage());
    m_DocumentObjectMetaData->SwapStorage(pDoc->m_DocumentObjectMetaData->GetStorage());
    // m_pSceneObjectAccessor does not need to be modified

    e.m_EventType = plDocumentObjectStructureEvent::Type::AfterReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);
  }

  const bool bVisualizers = plVisualizerManager::GetSingleton()->GetVisualizersActive(GetLayerDocument(m_ActiveLayerGuid));

  plVisualizerManager::GetSingleton()->SetVisualizersActive(GetLayerDocument(m_ActiveLayerGuid), false);

  {
    plSelectionManagerEvent se;
    se.m_pDocument = this;
    se.m_pObject = nullptr;
    se.m_Type = plSelectionManagerEvent::Type::SelectionSet;
    m_pSelectionManager->GetStorage()->m_Events.Broadcast(se);
  }
  {
    plCommandHistoryEvent ce;
    ce.m_pDocument = this;
    ce.m_Type = plCommandHistoryEvent::Type::HistoryChanged;
    m_pCommandHistory->GetStorage()->m_Events.Broadcast(ce);
  }

  m_ActiveLayerGuid = layerGuid;
  m_pActiveSubDocument = GetLayerDocument(layerGuid);
  {
    plScene2LayerEvent e;
    e.m_Type = plScene2LayerEvent::Type::ActiveLayerChanged;
    e.m_layerGuid = layerGuid;
    m_LayerEvents.Broadcast(e);
  }
  {
    plDocumentEvent e;
    e.m_pDocument = this;
    e.m_Type = plDocumentEvent::Type::ModifiedChanged;

    m_EventsOne.Broadcast(e);
    s_EventsAny.Broadcast(e);
  }
  {
    plActiveLayerChangedMsgToEngine msg;
    msg.m_ActiveLayer = layerGuid;
    SendMessageToEngine(&msg);
  }

  plVisualizerManager::GetSingleton()->SetVisualizersActive(GetLayerDocument(m_ActiveLayerGuid), bVisualizers);

  // Set selection to object that contains the active layer
  if (const plDocumentObject* pLayerObject = GetLayerObject(layerGuid))
  {
    m_pLayerSelection->SetSelection(pLayerObject);
  }
  return plStatus(PLASMA_SUCCESS);
}

bool plScene2Document::IsLayerLoaded(const plUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_pLayer != nullptr;
  }
  return false;
}

plStatus plScene2Document::SetLayerLoaded(const plUuid& layerGuid, bool bLoaded)
{
  if (GetGameMode() != GameMode::Enum::Off)
    return plStatus("Simulation must be stopped to change a layer's loaded state.");

  if (layerGuid == GetGuid() && !bLoaded)
    return plStatus("Cannot unload the scene itself.");

  // We can't unload the active layer
  if (!bLoaded && m_ActiveLayerGuid == layerGuid)
  {
    plStatus res = SetActiveLayer(GetGuid());
    if (res.Failed())
      return res;
  }

  LayerInfo* pInfo = nullptr;
  if (!m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return plStatus("Unknown layer guid. Layer can't be loaded / unloaded.");
  }

  if ((pInfo->m_pLayer != nullptr) == bLoaded)
    return plStatus(PLASMA_SUCCESS);

  if (bLoaded)
  {
    plStringBuilder sAbsPath;
    if (layerGuid == GetGuid())
    {
      sAbsPath = GetDocumentPath();
    }
    else
    {
      auto assetInfo = plAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
      if (assetInfo.isValid())
      {
        sAbsPath = assetInfo->m_pAssetInfo->m_sAbsolutePath;
      }
      else
      {
        return plStatus("Could not resolve layer in plAssetCurator.");
      }
    }

    plDocument* pDoc = nullptr;
    // Pass our root into it to indicate what the parent context of the layer is.
    plDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    if (plDocument* pLayer = plQtEditorApp::GetSingleton()->OpenDocument(sAbsPath, plDocumentFlags::None, pRoot))
    {
      if (layerGuid != GetGuid() && pLayer->GetMainDocument() != this)
      {
        return plStatus("Layer already open in another window.");
      }

      // In case we are responding to e.g. an redo 'Add Layer' the layer is already loaded in the editor but we still want to enforce that the event is fired every time after adding a layer.
      if (pInfo->m_pLayer != pLayer)
      {
        pInfo->m_pLayer = plDynamicCast<plSceneDocument*>(pLayer);

        plScene2LayerEvent e;
        e.m_Type = plScene2LayerEvent::Type::LayerLoaded;
        e.m_layerGuid = layerGuid;
        m_LayerEvents.Broadcast(e);
      }

      return plStatus(PLASMA_SUCCESS);
    }
    else
    {
      return plStatus("Could not load layer, see log for more information.");
    }
  }
  else
  {
    if (pInfo->m_pLayer == nullptr)
      return plStatus(PLASMA_SUCCESS);

    // Unload document (save and close)
    plDocumentManager* pManager = pInfo->m_pLayer->GetDocumentManager();
    pManager->CloseDocument(pInfo->m_pLayer);
    pInfo->m_pLayer = nullptr;

    // plScene2LayerEvent e;
    // e.m_Type = plScene2LayerEvent::Type::LayerUnloaded;
    // e.m_layerGuid = layerGuid;
    // m_LayerEvents.Broadcast(e);

    return plStatus(PLASMA_SUCCESS);
  }
}

void plScene2Document::GetAllLayers(plDynamicArray<plUuid>& out_LayerGuids)
{
  out_LayerGuids.Clear();
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    out_LayerGuids.PushBack(it.Key());
  }
}

void plScene2Document::GetLoadedLayers(plDynamicArray<plSceneDocument*>& out_Layers) const
{
  out_Layers.Clear();
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pLayer)
    {
      out_Layers.PushBack(it.Value().m_pLayer);
    }
  }
}

bool plScene2Document::IsLayerVisible(const plUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_bVisible;
  }
  return false;
}

plStatus plScene2Document::SetLayerVisible(const plUuid& layerGuid, bool bVisible)
{
  LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    if (pInfo->m_bVisible != bVisible)
    {
      pInfo->m_bVisible = bVisible;
      {
        plScene2LayerEvent e;
        e.m_Type = bVisible ? plScene2LayerEvent::Type::LayerVisible : plScene2LayerEvent::Type::LayerInvisible;
        e.m_layerGuid = layerGuid;
        m_LayerEvents.Broadcast(e);
      }
      SendLayerVisibility();
    }
    return plStatus(PLASMA_SUCCESS);
  }
  return plStatus("Unknown layer.");
}

const plDocumentObject* plScene2Document::GetLayerObject(const plUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return GetSceneObjectManager()->GetObject(pInfo->m_objectGuid);
  }
  return nullptr;
}

plSceneDocument* plScene2Document::GetLayerDocument(const plUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_pLayer;
  }
  return nullptr;
}

bool plScene2Document::IsAnyLayerModified() const
{
  for (auto& layer : m_Layers)
  {
    auto pLayer = layer.Value().m_pLayer;
    if (pLayer && pLayer->IsModified())
      return true;
  }

  return false;
}
