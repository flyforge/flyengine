#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Core/Interfaces/SoundInterface.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginScene/SceneContext/LayerContext.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneContext, 1, plRTTIDefaultAllocator<plSceneContext>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_CONSTANT_PROPERTY("DocumentType", (const char*) "Scene;Prefab;PropertyAnim"),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plSceneContext::ComputeHierarchyBounds(plGameObject* pObj, plBoundingBoxSphere& bounds)
{
  pObj->UpdateGlobalTransformAndBounds();
  const auto& b = pObj->GetGlobalBounds();

  if (b.IsValid())
    bounds.ExpandToInclude(b);

  for (auto it = pObj->GetChildren(); it.IsValid(); ++it)
  {
    ComputeHierarchyBounds(it, bounds);
  }
}

void plSceneContext::DrawSelectionBounds(const plViewHandle& hView)
{
  if (!m_bRenderSelectionBoxes)
    return;

  PL_LOCK(m_pWorld->GetWriteMarker());

  for (const auto& obj : m_Selection)
  {
    plBoundingBoxSphere bounds = plBoundingBoxSphere::MakeInvalid();

    plGameObject* pObj;
    if (!m_pWorld->TryGetObject(obj, pObj))
      continue;

    ComputeHierarchyBounds(pObj, bounds);

    if (bounds.IsValid())
    {
      plDebugRenderer::DrawLineBoxCorners(hView, bounds.GetBox(), 0.25f, plColorScheme::LightUI(plColorScheme::Yellow));
    }
  }
}

void plSceneContext::UpdateInvisibleLayerTags()
{
  if (m_bInvisibleLayersDirty)
  {
    m_bInvisibleLayersDirty = false;

    plMap<plUuid, plUInt32> layerGuidToIndex;
    for (plUInt32 i = 0; i < m_Layers.GetCount(); i++)
    {
      if (m_Layers[i] != nullptr)
      {
        layerGuidToIndex.Insert(m_Layers[i]->GetDocumentGuid(), i);
      }
    }

    plHybridArray<plTag, 1> newInvisibleLayerTags;
    newInvisibleLayerTags.Reserve(m_InvisibleLayers.GetCount());
    for (const plUuid& guid : m_InvisibleLayers)
    {
      plUInt32 uiLayerID = 0;
      if (layerGuidToIndex.TryGetValue(guid, uiLayerID))
      {
        newInvisibleLayerTags.PushBack(m_Layers[uiLayerID]->GetLayerTag());
      }
      else if (guid == GetDocumentGuid())
      {
        newInvisibleLayerTags.PushBack(m_LayerTag);
      }
    }

    for (plEngineProcessViewContext* pView : m_ViewContexts)
    {
      if (pView)
      {
        static_cast<plSceneViewContext*>(pView)->SetInvisibleLayerTags(m_InvisibleLayerTags.GetArrayPtr(), newInvisibleLayerTags.GetArrayPtr());
      }
    }
    m_InvisibleLayerTags.Swap(newInvisibleLayerTags);
  }
}

plSceneContext::plSceneContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
  m_bRenderSelectionOverlay = true;
  m_bRenderSelectionBoxes = true;
  m_bRenderShapeIcons = true;
  m_fGridDensity = 0;
  m_GridTransform.SetIdentity();
  m_pWorld = nullptr;

  plResourceManager::GetManagerEvents().AddEventHandler(plMakeDelegate(&plSceneContext::OnResourceManagerEvent, this));
  plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(plMakeDelegate(&plSceneContext::GameApplicationEventHandler, this));
}

plSceneContext::~plSceneContext()
{
  plResourceManager::GetManagerEvents().RemoveEventHandler(plMakeDelegate(&plSceneContext::OnResourceManagerEvent, this));
  plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(plMakeDelegate(&plSceneContext::GameApplicationEventHandler, this));
}

void plSceneContext::HandleMessage(const plEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plWorldSettingsMsgToEngine>())
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages
    HandleWorldSettingsMsg(static_cast<const plWorldSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plSimulationSettingsMsgToEngine>())
  {
    HandleSimulationSettingsMsg(static_cast<const plSimulationSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plGridSettingsMsgToEngine>())
  {
    HandleGridSettingsMsg(static_cast<const plGridSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plObjectsForDebugVisMsgToEngine>())
  {
    HandleObjectsForDebugVisMsg(static_cast<const plObjectsForDebugVisMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plGameModeMsgToEngine>())
  {
    HandleGameModeMsg(static_cast<const plGameModeMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plObjectSelectionMsgToEngine>())
  {
    HandleSelectionMsg(static_cast<const plObjectSelectionMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plQuerySelectionBBoxMsgToEngine>())
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plExposedDocumentObjectPropertiesMsgToEngine>())
  {
    HandleExposedPropertiesMsg(static_cast<const plExposedDocumentObjectPropertiesMsgToEngine*>(pMsg));
    return;
  }

  if (const plExportSceneGeometryMsgToEngine* msg = plDynamicCast<const plExportSceneGeometryMsgToEngine*>(pMsg))
  {
    HandleSceneGeometryMsg(msg);
    return;
  }

  if (const plPullObjectStateMsgToEngine* msg = plDynamicCast<const plPullObjectStateMsgToEngine*>(pMsg))
  {
    HandlePullObjectStateMsg(msg);
    return;
  }

  if (pMsg->IsInstanceOf<plViewRedrawMsgToEngine>())
  {
    HandleViewRedrawMsg(static_cast<const plViewRedrawMsgToEngine*>(pMsg));
    // fall through
  }

  if (pMsg->IsInstanceOf<plActiveLayerChangedMsgToEngine>())
  {
    HandleActiveLayerChangedMsg(static_cast<const plActiveLayerChangedMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->IsInstanceOf<plObjectTagMsgToEngine>())
  {
    HandleTagMsgToEngineMsg(static_cast<const plObjectTagMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->IsInstanceOf<plLayerVisibilityChangedMsgToEngine>())
  {
    HandleLayerVisibilityChangedMsgToEngineMsg(static_cast<const plLayerVisibilityChangedMsgToEngine*>(pMsg));
    return;
  }

  plEngineProcessDocumentContext::HandleMessage(pMsg);

  if (pMsg->IsInstanceOf<plEntityMsgToEngine>())
  {
    PL_LOCK(m_pWorld->GetWriteMarker());
    AddLayerIndexTag(*static_cast<const plEntityMsgToEngine*>(pMsg), m_Context, m_LayerTag);
  }
}

void plSceneContext::HandleViewRedrawMsg(const plViewRedrawMsgToEngine* pMsg)
{
  if (m_bUpdateAllLocalBounds)
  {
    m_bUpdateAllLocalBounds = false;

    PL_LOCK(m_pWorld->GetWriteMarker());

    for (auto it = m_pWorld->GetObjects(); it.IsValid(); ++it)
    {
      it->UpdateLocalBounds();
    }
  }

  auto pDocView = GetViewContext(pMsg->m_uiViewID);
  if (pDocView)
    DrawSelectionBounds(pDocView->GetViewHandle());

  AnswerObjectStatePullRequest(pMsg);
  UpdateInvisibleLayerTags();
}

void plSceneContext::AnswerObjectStatePullRequest(const plViewRedrawMsgToEngine* pMsg)
{
  if (m_pWorld->GetWorldSimulationEnabled() || m_PushObjectStateMsg.m_ObjectStates.IsEmpty())
    return;

  PL_LOCK(m_pWorld->GetReadMarker());

  for (auto& state : m_PushObjectStateMsg.m_ObjectStates)
  {
    plWorldRttiConverterContext* pContext = GetContextForLayer(state.m_LayerGuid);
    if (!pContext)
      return;

    // if the handle map is currently empty, the scene has not yet been sent over
    // return and try again later
    if (pContext->m_GameObjectMap.GetHandleToGuidMap().IsEmpty())
      return;
  }

  // now we need to adjust the transforms for all objects that were not directly pulled
  // ie. nodes inside instantiated prefabs
  for (auto& state : m_PushObjectStateMsg.m_ObjectStates)
  {
    // ignore the ones that we accessed directly, their transform is correct already
    if (!state.m_bAdjustFromPrefabRootChild)
      continue;

    plWorldRttiConverterContext* pContext = GetContextForLayer(state.m_LayerGuid);
    if (!pContext)
      continue;

    const auto& objectMapper = pContext->m_GameObjectMap;

    plGameObjectHandle hObject = objectMapper.GetHandle(state.m_ObjectGuid);

    // if this object does not exist anymore, this is not considered a problem (user may have deleted it)
    plGameObject* pObject;
    if (!m_pWorld->TryGetObject(hObject, pObject))
      continue;

    // we expect the object to have a child, if none is there yet, we assume the prefab
    // instantiation has not happened yet
    // stop the whole process and try again later
    if (pObject->GetChildCount() == 0)
      return;

    const plGameObject* pChild = pObject->GetChildren();

    const plVec3 localPos = pChild->GetLocalPosition();
    const plQuat localRot = pChild->GetLocalRotation();

    // now adjust the position
    state.m_vPosition -= state.m_qRotation * localRot.GetInverse() * localPos;
    state.m_qRotation = state.m_qRotation * localRot.GetInverse();
  }

  // send a return message with the result
  m_PushObjectStateMsg.m_DocumentGuid = pMsg->m_DocumentGuid;
  SendProcessMessage(&m_PushObjectStateMsg);

  m_PushObjectStateMsg.m_ObjectStates.Clear();
}

void plSceneContext::HandleActiveLayerChangedMsg(const plActiveLayerChangedMsgToEngine* pMsg)
{
  m_ActiveLayer = pMsg->m_ActiveLayer;
}

void plSceneContext::HandleTagMsgToEngineMsg(const plObjectTagMsgToEngine* pMsg)
{
  PL_LOCK(m_pWorld->GetWriteMarker());

  plGameObjectHandle hObject = GetActiveContext().m_GameObjectMap.GetHandle(pMsg->m_ObjectGuid);

  const plTag& tag = plTagRegistry::GetGlobalRegistry().RegisterTag(pMsg->m_sTag);

  plGameObject* pObject;
  if (m_pWorld->TryGetObject(hObject, pObject))
  {
    if (pMsg->m_bApplyOnAllChildren)
    {
      if (pMsg->m_bSetTag)
        SetTagRecursive(pObject, tag);
      else
        ClearTagRecursive(pObject, tag);
    }
    else
    {
      if (pMsg->m_bSetTag)
        pObject->SetTag(tag);
      else
        pObject->RemoveTag(tag);
    }
  }
}

void plSceneContext::HandleLayerVisibilityChangedMsgToEngineMsg(const plLayerVisibilityChangedMsgToEngine* pMsg)
{
  m_InvisibleLayers = pMsg->m_HiddenLayers;
  m_bInvisibleLayersDirty = true;
}

void plSceneContext::HandleGridSettingsMsg(const plGridSettingsMsgToEngine* pMsg)
{
  m_fGridDensity = pMsg->m_fGridDensity;
  if (m_fGridDensity != 0.0f)
  {
    m_GridTransform.m_vPosition = pMsg->m_vGridCenter;

    if (pMsg->m_vGridTangent1.IsZero())
    {
      m_GridTransform.m_vScale.SetZero();
    }
    else
    {
      m_GridTransform.m_vScale.Set(1.0f);

      plMat3 mRot;
      mRot.SetColumn(0, pMsg->m_vGridTangent1);
      mRot.SetColumn(1, pMsg->m_vGridTangent2);
      mRot.SetColumn(2, pMsg->m_vGridTangent1.CrossRH(pMsg->m_vGridTangent2));
      m_GridTransform.m_qRotation = plQuat::MakeFromMat3(mRot);
    }
  }
}

void plSceneContext::HandleSimulationSettingsMsg(const plSimulationSettingsMsgToEngine* pMsg)
{
  const bool bSimulate = pMsg->m_bSimulateWorld;
  plGameStateBase* pState = GetGameState();
  m_pWorld->GetClock().SetSpeed(pMsg->m_fSimulationSpeed);

  if (pState == nullptr && bSimulate != m_pWorld->GetWorldSimulationEnabled())
  {
    m_pWorld->SetWorldSimulationEnabled(bSimulate);

    if (bSimulate)
      OnSimulationEnabled();
    else
      OnSimulationDisabled();
  }
}

void plSceneContext::HandleWorldSettingsMsg(const plWorldSettingsMsgToEngine* pMsg)
{
  m_bRenderSelectionOverlay = pMsg->m_bRenderOverlay;
  m_bRenderShapeIcons = pMsg->m_bRenderShapeIcons;
  m_bRenderSelectionBoxes = pMsg->m_bRenderSelectionBoxes;

  if (pMsg->m_bAddAmbientLight)
    AddAmbientLight(true, false);
  else
    RemoveAmbientLight();
}

void plSceneContext::QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg)
{
  if (m_Selection.IsEmpty())
    return;

  plBoundingBoxSphere bounds = plBoundingBoxSphere::MakeInvalid();

  {
    PL_LOCK(m_pWorld->GetWriteMarker());

    for (const auto& obj : m_Selection)
    {
      plGameObject* pObj;
      if (!m_pWorld->TryGetObject(obj, pObj))
        continue;

      ComputeHierarchyBounds(pObj, bounds);
    }

    // if there are no valid bounds, at all, use dummy bounds for each object
    if (!bounds.IsValid())
    {
      for (const auto& obj : m_Selection)
      {
        plGameObject* pObj;
        if (!m_pWorld->TryGetObject(obj, pObj))
          continue;

        bounds.ExpandToInclude(plBoundingBoxSphere::MakeFromCenterExtents(pObj->GetGlobalPosition(), plVec3(0.0f), 0.0f));
      }
    }
  }

  // PL_ASSERT_DEV(bounds.IsValid() && !bounds.IsNaN(), "Invalid bounds");

  if (!bounds.IsValid() || bounds.IsNaN())
  {
    plLog::Error("Selection has no valid bounding box");
    return;
  }

  const plQuerySelectionBBoxMsgToEngine* msg = static_cast<const plQuerySelectionBBoxMsgToEngine*>(pMsg);

  plQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID = msg->m_uiViewID;
  res.m_iPurpose = msg->m_iPurpose;
  res.m_vCenter = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}

void plSceneContext::OnSimulationEnabled()
{
  plLog::Info("World Simulation enabled");

  plSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentType(), GetDocumentGuid(), false);

  plResourceManager::ReloadAllResources(false);

  plGameApplication::GetGameApplicationInstance()->ReinitializeInputConfig();

  if (plSoundInterface* pSoundInterface = plSingletonRegistry::GetSingletonInstance<plSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(true);
  }
}

void plSceneContext::OnSimulationDisabled()
{
  plLog::Info("World Simulation disabled");

  plResourceManager::ResetAllResources();

  if (plSoundInterface* pSoundInterface = plSingletonRegistry::GetSingletonInstance<plSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(false);
  }
}

plGameStateBase* plSceneContext::GetGameState() const
{
  return plGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameStateLinkedToWorld(m_pWorld);
}

plUInt32 plSceneContext::RegisterLayer(plLayerContext* pLayer)
{
  m_bInvisibleLayersDirty = true;
  m_Contexts.PushBack(&pLayer->m_Context);
  for (plUInt32 i = 0; i < m_Layers.GetCount(); ++i)
  {
    if (m_Layers[i] == nullptr)
    {
      m_Layers[i] = pLayer;
      return i;
    }
  }

  m_Layers.PushBack(pLayer);
  return m_Layers.GetCount() - 1;
}

void plSceneContext::UnregisterLayer(plLayerContext* pLayer)
{
  m_Contexts.RemoveAndSwap(&pLayer->m_Context);
  for (plUInt32 i = 0; i < m_Layers.GetCount(); ++i)
  {
    if (m_Layers[i] == pLayer)
    {
      m_Layers[i] = nullptr;
    }
  }

  while (!m_Layers.IsEmpty() && m_Layers.PeekBack() == nullptr)
    m_Layers.PopBack();
}

void plSceneContext::AddLayerIndexTag(const plEntityMsgToEngine& msg, plWorldRttiConverterContext& ref_context, const plTag& layerTag)
{
  if (msg.m_change.m_Change.m_Operation == plObjectChangeType::NodeAdded)
  {
    if ((msg.m_change.m_Change.m_sProperty == "Children" || msg.m_change.m_Change.m_sProperty.IsEmpty()) && msg.m_change.m_Change.m_Value.IsA<plUuid>())
    {
      const plUuid& object = msg.m_change.m_Change.m_Value.Get<plUuid>();
      plRttiConverterObject target = ref_context.GetObjectByGUID(object);
      if (target.m_pType == plGetStaticRTTI<plGameObject>() && target.m_pObject != nullptr)
      {
        // We do postpone tagging until after the first frame so that prefab references are instantiated and affected as well.
        plGameObject* pObject = static_cast<plGameObject*>(target.m_pObject);
        m_ObjectsToTag.PushBack({pObject->GetHandle(), layerTag});
      }
    }
  }
}

const plArrayPtr<const plTag> plSceneContext::GetInvisibleLayerTags() const
{
  return m_InvisibleLayerTags.GetArrayPtr();
}

void plSceneContext::OnInitialize()
{
  PL_LOCK(m_pWorld->GetWriteMarker());
  if (!m_ActiveLayer.IsValid())
    m_ActiveLayer = m_DocumentGuid;
  m_Contexts.PushBack(&m_Context);

  m_LayerTag = plTagRegistry::GetGlobalRegistry().RegisterTag("Layer_Scene");

  plShadowPool::AddExcludeTagToWhiteList(m_LayerTag);
}

void plSceneContext::OnDeinitialize()
{
  m_Selection.Clear();
  m_SelectionWithChildren.Clear();
  m_SelectionWithChildrenSet.Clear();
  m_hSkyLight.Invalidate();
  m_hDirectionalLight.Invalidate();
  m_LayerTag = plTag();
  for (plLayerContext* pLayer : m_Layers)
  {
    if (pLayer != nullptr)
      pLayer->SceneDeinitialized();
  }
}

plEngineProcessViewContext* plSceneContext::CreateViewContext()
{
  return PL_DEFAULT_NEW(plSceneViewContext, this);
}

void plSceneContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PL_DEFAULT_DELETE(pContext);
}

void plSceneContext::HandleSelectionMsg(const plObjectSelectionMsgToEngine* pMsg)
{
  m_Selection.Clear();
  m_SelectionWithChildrenSet.Clear();
  m_SelectionWithChildren.Clear();

  plStringBuilder sSel = pMsg->m_sSelection;
  plStringBuilder sGuid;

  auto pWorld = m_pWorld;
  PL_LOCK(pWorld->GetReadMarker());

  while (!sSel.IsEmpty())
  {
    sGuid.SetSubString_ElementCount(sSel.GetData() + 1, 40);
    sSel.Shrink(41, 0);

    const plUuid guid = plConversionUtils::ConvertStringToUuid(sGuid);

    auto hObject = GetActiveContext().m_GameObjectMap.GetHandle(guid);

    if (!hObject.IsInvalidated())
    {
      m_Selection.PushBack(hObject);

      plGameObject* pObject;
      if (pWorld->TryGetObject(hObject, pObject))
        InsertSelectedChildren(pObject);
    }
  }

  for (auto it = m_SelectionWithChildrenSet.GetIterator(); it.IsValid(); ++it)
  {
    m_SelectionWithChildren.PushBack(it.Key());
  }
}

void plSceneContext::OnPlayTheGameModeStarted(const plTransform* pStartPosition)
{
  if (plGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState() != nullptr)
  {
    plLog::Warning("A Play-the-Game instance is already running, cannot launch a second in parallel.");
    return;
  }

  plLog::Info("Starting Play-the-Game mode");

  plSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentType(), GetDocumentGuid(), false);

  plResourceManager::ReloadAllResources(false);

  m_pWorld->GetClock().SetSpeed(1.0f);
  m_pWorld->SetWorldSimulationEnabled(true);

  plGameApplication::GetGameApplicationInstance()->ReinitializeInputConfig();

  plGameApplicationBase::GetGameApplicationBaseInstance()->ActivateGameState(m_pWorld, pStartPosition).IgnoreResult();

  plGameModeMsgToEditor msgRet;
  msgRet.m_DocumentGuid = GetDocumentGuid();
  msgRet.m_bRunningPTG = true;

  SendProcessMessage(&msgRet);

  if (plSoundInterface* pSoundInterface = plSingletonRegistry::GetSingletonInstance<plSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(false);
  }
}

void plSceneContext::OnResourceManagerEvent(const plResourceManagerEvent& e)
{
  if (e.m_Type == plResourceManagerEvent::Type::ReloadAllResources)
  {
    // when resources get reloaded, make sure to update all object bounds
    // this is to prevent culling errors after meshes got transformed etc.
    m_bUpdateAllLocalBounds = true;
  }
}

void plSceneContext::GameApplicationEventHandler(const plGameApplicationExecutionEvent& e)
{
  if (e.m_Type == plGameApplicationExecutionEvent::Type::AfterUpdatePlugins && !m_ObjectsToTag.IsEmpty())
  {
    // At this point the world was ticked once and prefab instances are instantiated and will be affected by SetTagRecursive.
    PL_LOCK(m_pWorld->GetWriteMarker());
    for (const TagGameObject& tagObject : m_ObjectsToTag)
    {
      plGameObject* pObject = nullptr;
      if (m_pWorld->TryGetObject(tagObject.m_hObject, pObject))
      {
        SetTagRecursive(pObject, tagObject.m_Tag);
      }
    }
    m_ObjectsToTag.Clear();
  }
}

void plSceneContext::HandleObjectsForDebugVisMsg(const plObjectsForDebugVisMsgToEngine* pMsg)
{
  PL_LOCK(GetWorld()->GetWriteMarker());

  const plArrayPtr<const plUuid> guids(reinterpret_cast<const plUuid*>(pMsg->m_Objects.GetData()), pMsg->m_Objects.GetCount() / sizeof(plUuid));

  for (auto guid : guids)
  {
    auto hComp = GetActiveContext().m_ComponentMap.GetHandle(guid);

    if (hComp.IsInvalidated())
      continue;

    plEventMessageHandlerComponent* pComp = nullptr;
    if (!m_pWorld->TryGetComponent(hComp, pComp))
      continue;

    pComp->SetDebugOutput(true);
  }
}

void plSceneContext::HandleGameModeMsg(const plGameModeMsgToEngine* pMsg)
{
  plGameStateBase* pState = GetGameState();

  if (pMsg->m_bEnablePTG)
  {
    if (pState != nullptr)
    {
      plLog::Error("Cannot start Play-the-Game, there is already a game state active for this world");
      return;
    }

    if (pMsg->m_bUseStartPosition)
    {
      plQuat qRot = plQuat::MakeShortestRotation(plVec3(1, 0, 0), pMsg->m_vStartDirection);

      plTransform tStart(pMsg->m_vStartPosition, qRot);

      OnPlayTheGameModeStarted(&tStart);
    }
    else
    {
      OnPlayTheGameModeStarted(nullptr);
    }
  }
  else
  {
    if (pState == nullptr)
      return;

    plLog::Info("Attempting to stop Play-the-Game mode");
    pState->RequestQuit();
  }
}

void plSceneContext::InsertSelectedChildren(const plGameObject* pObject)
{
  m_SelectionWithChildrenSet.Insert(pObject->GetHandle());

  auto it = pObject->GetChildren();

  while (it.IsValid())
  {
    InsertSelectedChildren(it);

    it.Next();
  }
}

plStatus plSceneContext::ExportDocument(const plExportDocumentMsgToEngine* pMsg)
{
  if (!m_Context.m_UnknownTypes.IsEmpty())
  {
    plStringBuilder s;

    s.Append("Scene / prefab export failed: ");

    for (const plString& sType : m_Context.m_UnknownTypes)
    {
      s.AppendFormat("'{}' is unknown. ", sType);
    }

    return plStatus(s.GetView());
  }

  // make sure the world has been updated at least once, otherwise components aren't initialized
  // and messages for geometry extraction won't be delivered
  // this is necessary for the scene export modifiers to work
  {
    PL_LOCK(m_pWorld->GetWriteMarker());
    m_pWorld->SetWorldSimulationEnabled(false);
    m_pWorld->Update();
  }

  // #TODO layers
  plSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentType(), GetDocumentGuid(), true);

  plDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  // export
  {
    // File Header
    {
      plAssetFileHeader header;
      header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
      header.Write(file).IgnoreResult();

      const char* szSceneTag = "[plBinaryScene]";
      file.WriteBytes(szSceneTag, sizeof(char) * 16).IgnoreResult();
    }

    const plTag& tagEditor = plTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
    const plTag& tagNoExport = plTagRegistry::GetGlobalRegistry().RegisterTag("Exclude From Export");

    plTagSet tags;
    tags.Set(tagEditor);
    tags.Set(tagNoExport);

    plWorldWriter ww;
    ww.WriteWorld(file, *m_pWorld, &tags);

    ExportExposedParameters(ww, file);
  }

  // do the actual file writing
  if (file.Close().Failed())
    return plStatus(plFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return plStatus(PL_SUCCESS);
}

void plSceneContext::ExportExposedParameters(const plWorldWriter& ww, plDeferredFileWriter& file) const
{
  plHybridArray<plExposedPrefabParameterDesc, 16> exposedParams;

  for (const auto& esp : m_ExposedSceneProperties)
  {
    plGameObject* pTargetObject = nullptr;
    const plRTTI* pComponenType = nullptr;

    plRttiConverterObject obj = m_Context.GetObjectByGUID(esp.m_Object);

    if (obj.m_pType == nullptr)
      continue;

    if (obj.m_pType->IsDerivedFrom<plGameObject>())
    {
      pTargetObject = reinterpret_cast<plGameObject*>(obj.m_pObject);
    }
    else if (obj.m_pType->IsDerivedFrom<plComponent>())
    {
      plComponent* pComponent = reinterpret_cast<plComponent*>(obj.m_pObject);

      pTargetObject = pComponent->GetOwner();
      pComponenType = obj.m_pType;
    }

    if (pTargetObject == nullptr)
      continue;

    plInt32 iFoundObjRoot = -1;
    plInt32 iFoundObjChild = -1;

    // search for the target object in the exported objects
    {
      const auto& objects = ww.GetAllWrittenRootObjects();
      for (plUInt32 i = 0; i < objects.GetCount(); ++i)
      {
        if (objects[i] == pTargetObject)
        {
          iFoundObjRoot = i;
          break;
        }
      }

      if (iFoundObjRoot < 0)
      {
        const auto& objects = ww.GetAllWrittenChildObjects();
        for (plUInt32 i = 0; i < objects.GetCount(); ++i)
        {
          if (objects[i] == pTargetObject)
          {
            iFoundObjChild = i;
            break;
          }
        }
      }
    }

    // if exposed object not found, ignore parameter
    if (iFoundObjRoot < 0 && iFoundObjChild < 0)
      continue;

    // store the exposed parameter information
    plExposedPrefabParameterDesc& paramdesc = exposedParams.ExpandAndGetRef();
    paramdesc.m_sExposeName.Assign(esp.m_sName.GetData());
    paramdesc.m_uiWorldReaderChildObject = (iFoundObjChild >= 0) ? 1 : 0;
    paramdesc.m_uiWorldReaderObjectIndex = (iFoundObjChild >= 0) ? iFoundObjChild : iFoundObjRoot;
    paramdesc.m_sComponentType.Clear();

    if (pComponenType)
    {
      paramdesc.m_sComponentType.Assign(pComponenType->GetTypeName());
    }

    paramdesc.m_sProperty.Assign(esp.m_sPropertyPath.GetData());
  }

  exposedParams.Sort([](const plExposedPrefabParameterDesc& lhs, const plExposedPrefabParameterDesc& rhs) -> bool { return lhs.m_sExposeName.GetHash() < rhs.m_sExposeName.GetHash(); });

  file << exposedParams.GetCount();

  for (const auto& ep : exposedParams)
  {
    ep.Save(file);
  }
}

void plSceneContext::OnThumbnailViewContextCreated()
{
  // make sure there is ambient light in the thumbnails
  // TODO: should check whether this is a prefab (info currently not available in plSceneContext)
  RemoveAmbientLight();
  AddAmbientLight(false, true);
}

void plSceneContext::OnDestroyThumbnailViewContext()
{
  RemoveAmbientLight();
}

void plSceneContext::UpdateDocumentContext()
{
  SUPER::UpdateDocumentContext();
  plGameStateBase* pState = GetGameState();
  if (pState && pState->WasQuitRequested())
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->DeactivateGameState();

    plGameModeMsgToEditor msgToEd;
    msgToEd.m_DocumentGuid = GetDocumentGuid();
    msgToEd.m_bRunningPTG = false;

    SendProcessMessage(&msgToEd);
  }
}

plGameObjectHandle plSceneContext::ResolveStringToGameObjectHandle(const void* pString, plComponentHandle hThis, plStringView sProperty) const
{
  const char* szTargetGuid = reinterpret_cast<const char*>(pString);

  if (hThis.IsInvalidated() && sProperty.IsEmpty())
  {
    // This code path is used by plPrefabReferenceComponent::SerializeComponent() to check whether an arbitrary string may
    // represent a game object reference. References will always be stringyfied GUIDs.

    if (!plConversionUtils::IsStringUuid(szTargetGuid))
      return plGameObjectHandle();

    // convert string to GUID and check if references a known object
    return m_Context.m_GameObjectMap.GetHandle(plConversionUtils::ConvertStringToUuid(szTargetGuid));
  }

  // Test if the component is a direct part of this scene or one of its layers.
  if (m_Context.m_ComponentMap.GetGuid(hThis).IsValid())
  {
    return SUPER::ResolveStringToGameObjectHandle(pString, hThis, sProperty);
  }
  for (const plLayerContext* pLayer : m_Layers)
  {
    if (pLayer)
    {
      if (pLayer->m_Context.m_ComponentMap.GetGuid(hThis).IsValid())
      {
        return pLayer->ResolveStringToGameObjectHandle(pString, hThis, sProperty);
      }
    }
  }

  // Component not found - it is probably an engine prefab instance part.
  // Walk up the hierarchy and find a game object that belongs to the scene or layer.
  plComponent* pComponent = nullptr;
  if (!GetWorld()->TryGetComponent<plComponent>(hThis, pComponent))
    return {};

  const plGameObject* pParent = pComponent->GetOwner();
  while (pParent)
  {
    if (m_Context.m_GameObjectMap.GetGuid(pParent->GetHandle()).IsValid())
    {
      return SUPER::ResolveStringToGameObjectHandle(pString, hThis, sProperty);
    }
    for (const plLayerContext* pLayer : m_Layers)
    {
      if (pLayer)
      {
        if (pLayer->m_Context.m_GameObjectMap.GetGuid(pParent->GetHandle()).IsValid())
        {
          return pLayer->ResolveStringToGameObjectHandle(pString, hThis, sProperty);
        }
      }
    }
    pParent = pParent->GetParent();
  }

  plLog::Error("Game object reference could not be resolved. Component source was not found.");
  return plGameObjectHandle();
}

bool plSceneContext::UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext)
{
  const plBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  plSceneViewContext* pMaterialViewContext = static_cast<plSceneViewContext*>(pThumbnailViewContext);
  const bool result = pMaterialViewContext->UpdateThumbnailCamera(bounds);

  return result;
}

void plSceneContext::AddAmbientLight(bool bSetEditorTag, bool bForce)
{
  if (!m_hSkyLight.IsInvalidated() || !m_hDirectionalLight.IsInvalidated())
    return;

  PL_LOCK(GetWorld()->GetWriteMarker());

  // delay adding ambient light until the scene isn't empty, to prevent adding two skylights
  if (!bForce && GetWorld()->GetObjectCount() == 0)
    return;

  plSkyLightComponentManager* pSkyMan = GetWorld()->GetComponentManager<plSkyLightComponentManager>();
  if (pSkyMan == nullptr || pSkyMan->GetSingletonComponent() == nullptr)
  {
    // only create a skylight, if there is none yet

    plGameObjectDesc obj;
    obj.m_sName.Assign("Sky Light");

    if (bSetEditorTag)
    {
      const plTag& tagEditor = plTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
      obj.m_Tags.Set(tagEditor); // to prevent it from being exported
    }

    plGameObject* pObj;
    m_hSkyLight = GetWorld()->CreateObject(obj, pObj);


    plSkyLightComponent* pSkyLight = nullptr;
    plSkyLightComponent::CreateComponent(pObj, pSkyLight);
    pSkyLight->SetCubeMapFile("{ 0b202e08-a64f-465d-b38e-15b81d161822 }");
    pSkyLight->SetReflectionProbeMode(plReflectionProbeMode::Static);
  }

  {
    plGameObjectDesc obj;
    obj.m_sName.Assign("Ambient Light");

    obj.m_LocalRotation = plQuat::MakeFromEulerAngles(plAngle::MakeFromDegree(-14.510815f), plAngle::MakeFromDegree(43.07951f), plAngle::MakeFromDegree(93.223808f));

    if (bSetEditorTag)
    {
      const plTag& tagEditor = plTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
      obj.m_Tags.Set(tagEditor); // to prevent it from being exported
    }

    plGameObject* pLight;
    m_hDirectionalLight = GetWorld()->CreateObject(obj, pLight);

    plDirectionalLightComponent* pDirLight = nullptr;
    plDirectionalLightComponent::CreateComponent(pLight, pDirLight);
    pDirLight->SetIntensity(10.0f);
  }
}

void plSceneContext::RemoveAmbientLight()
{
  PL_LOCK(GetWorld()->GetWriteMarker());

  if (!m_hSkyLight.IsInvalidated())
  {
    // make sure to remove the object RIGHT NOW, otherwise it may still exist during scene export (without the "Editor" tag)
    GetWorld()->DeleteObjectNow(m_hSkyLight);
    m_hSkyLight.Invalidate();
  }

  if (!m_hDirectionalLight.IsInvalidated())
  {
    // make sure to remove the object RIGHT NOW, otherwise it may still exist during scene export (without the "Editor" tag)
    GetWorld()->DeleteObjectNow(m_hDirectionalLight);
    m_hDirectionalLight.Invalidate();
  }
}

const plEngineProcessDocumentContext* plSceneContext::GetActiveDocumentContext() const
{
  if (m_ActiveLayer == GetDocumentGuid())
  {
    return this;
  }

  for (const plLayerContext* pLayer : m_Layers)
  {
    if (pLayer && m_ActiveLayer == pLayer->GetDocumentGuid())
    {
      return pLayer;
    }
  }

  PL_REPORT_FAILURE("Active layer does not exist.");
  return this;
}

plEngineProcessDocumentContext* plSceneContext::GetActiveDocumentContext()
{
  return const_cast<plEngineProcessDocumentContext*>(const_cast<const plSceneContext*>(this)->GetActiveDocumentContext());
}

const plWorldRttiConverterContext& plSceneContext::GetActiveContext() const
{
  return GetActiveDocumentContext()->m_Context;
}

plWorldRttiConverterContext& plSceneContext::GetActiveContext()
{
  return const_cast<plWorldRttiConverterContext&>(const_cast<const plSceneContext*>(this)->GetActiveContext());
}

plWorldRttiConverterContext* plSceneContext::GetContextForLayer(const plUuid& layerGuid)
{
  if (layerGuid == GetDocumentGuid())
    return &m_Context;

  for (plLayerContext* pLayer : m_Layers)
  {
    if (pLayer && layerGuid == pLayer->GetDocumentGuid())
    {
      return &pLayer->m_Context;
    }
  }
  return nullptr;
}

plArrayPtr<plWorldRttiConverterContext*> plSceneContext::GetAllContexts()
{
  return m_Contexts;
}

void plSceneContext::HandleExposedPropertiesMsg(const plExposedDocumentObjectPropertiesMsgToEngine* pMsg)
{
  m_ExposedSceneProperties = pMsg->m_Properties;
}

void plSceneContext::HandleSceneGeometryMsg(const plExportSceneGeometryMsgToEngine* pMsg)
{
  plWorldGeoExtractionUtil::MeshObjectList objects;

  plTagSet excludeTags;
  excludeTags.SetByName("Editor");

  if (pMsg->m_bSelectionOnly)
    plWorldGeoExtractionUtil::ExtractWorldGeometry(objects, *m_pWorld, static_cast<plWorldGeoExtractionUtil::ExtractionMode>(pMsg->m_iExtractionMode), m_SelectionWithChildren);
  else
    plWorldGeoExtractionUtil::ExtractWorldGeometry(objects, *m_pWorld, static_cast<plWorldGeoExtractionUtil::ExtractionMode>(pMsg->m_iExtractionMode), &excludeTags);

  plWorldGeoExtractionUtil::WriteWorldGeometryToOBJ(pMsg->m_sOutputFile, objects, pMsg->m_Transform);
}

void plSceneContext::HandlePullObjectStateMsg(const plPullObjectStateMsgToEngine* pMsg)
{
  if (!m_pWorld->GetWorldSimulationEnabled())
    return;

  const plWorld* pWorld = GetWorld();
  PL_LOCK(pWorld->GetReadMarker());

  const auto& objectMapper = GetActiveContext().m_GameObjectMap;

  m_PushObjectStateMsg.m_ObjectStates.Reserve(m_PushObjectStateMsg.m_ObjectStates.GetCount() + m_SelectionWithChildren.GetCount());

  for (plGameObjectHandle hObject : m_SelectionWithChildren)
  {
    const plGameObject* pObject = nullptr;
    if (!pWorld->TryGetObject(hObject, pObject))
      continue;

    plUuid objectGuid = objectMapper.GetGuid(hObject);
    bool bAdjust = false;

    if (!objectGuid.IsValid())
    {
      // this must be an object created on the runtime side, try to match it to some editor object
      // we only try the direct parent, more steps than that are not allowed

      const plGameObject* pParentObject = pObject->GetParent();
      if (pParentObject == nullptr)
        continue;

      // if the parent has more than one child, remapping the position from the child to the parent is not possible, so skip those
      if (pParentObject->GetChildCount() > 1)
        continue;

      auto parentGuid = objectMapper.GetGuid(pParentObject->GetHandle());

      if (!parentGuid.IsValid())
        continue;

      objectGuid = parentGuid;
      bAdjust = true;

      for (plUInt32 i = 0; i < m_PushObjectStateMsg.m_ObjectStates.GetCount(); ++i)
      {
        if (m_PushObjectStateMsg.m_ObjectStates[i].m_ObjectGuid == objectGuid)
        {
          m_PushObjectStateMsg.m_ObjectStates.RemoveAtAndCopy(i);
          break;
        }
      }
    }

    {
      auto& state = m_PushObjectStateMsg.m_ObjectStates.ExpandAndGetRef();

      state.m_LayerGuid = m_ActiveLayer;
      state.m_ObjectGuid = objectGuid;
      state.m_bAdjustFromPrefabRootChild = bAdjust;
      state.m_vPosition = pObject->GetGlobalPosition();
      state.m_qRotation = pObject->GetGlobalRotation();

      plMsgRetrieveBoneState msg;
      pObject->SendMessage(msg);

      state.m_BoneTransforms = msg.m_BoneTransforms;
    }
  }

  // the return message is sent after the simulation has stopped
}
