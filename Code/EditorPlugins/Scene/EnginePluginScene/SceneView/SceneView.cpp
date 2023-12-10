#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <Core/Interfaces/SoundInterface.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EnginePluginScene/SceneView/SceneView.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plSceneViewContext::plSceneViewContext(plSceneContext* pSceneContext)
  : plEngineProcessViewContext(pSceneContext)
{
  m_pSceneContext = pSceneContext;
  m_bUpdatePickingData = true;

  // Start with something valid.
  m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(plVec3(1, 1, 1), plVec3::MakeZero(), plVec3(0.0f, 0.0f, 1.0f));

  m_CullingCamera = m_Camera;
}

plSceneViewContext::~plSceneViewContext() {}

void plSceneViewContext::HandleViewMessage(const plEditorEngineViewMsg* pMsg)
{
  plEngineProcessViewContext::HandleViewMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plViewRedrawMsgToEngine>())
  {
    const plViewRedrawMsgToEngine* pMsg2 = static_cast<const plViewRedrawMsgToEngine*>(pMsg);

    plView* pView = nullptr;
    if (plRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetRenderPassProperty("EditorPickingPass", "Active", pMsg2->m_bUpdatePickingData);
      pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", pMsg2->m_bEnablePickingSelected);
      pView->SetRenderPassProperty("EditorPickingPass", "PickTransparent", pMsg2->m_bEnablePickTransparent);
    }

    if (pMsg2->m_iCameraMode == plCameraMode::PerspectiveFixedFovX || pMsg2->m_iCameraMode == plCameraMode::PerspectiveFixedFovY)
    {
      if (!m_pSceneContext->IsPlayTheGameActive())
      {
        if (plSoundInterface* pSoundInterface = plSingletonRegistry::GetSingletonInstance<plSoundInterface>())
        {
          pSoundInterface->SetListener(-1, pMsg2->m_vPosition, pMsg2->m_vDirForwards, pMsg2->m_vDirUp, plVec3::MakeZero());
        }
      }
    }
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plViewPickingMsgToEngine>())
  {
    const plViewPickingMsgToEngine* pMsg2 = static_cast<const plViewPickingMsgToEngine*>(pMsg);

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plViewMarqueePickingMsgToEngine>())
  {
    const plViewMarqueePickingMsgToEngine* pMsg2 = static_cast<const plViewMarqueePickingMsgToEngine*>(pMsg);

    MarqueePickObjects(pMsg2);
  }
}

void plSceneViewContext::SetupRenderTarget(plGALSwapChainHandle hSwapChain, const plGALRenderTargets* renderTargets, plUInt16 uiWidth, plUInt16 uiHeight)
{
  plEngineProcessViewContext::SetupRenderTarget(hSwapChain, renderTargets, uiWidth, uiHeight);
  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView))
  {
    plTagSet& excludeTags = pView->m_ExcludeTags;
    const plArrayPtr<const plTag> addTags = m_pSceneContext->GetInvisibleLayerTags();
    for (const plTag& addTag : addTags)
    {
      excludeTags.Set(addTag);
    }
  }
}

bool plSceneViewContext::UpdateThumbnailCamera(const plBoundingBoxSphere& bounds)
{
  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetViewRenderMode(plViewRenderMode::Default);
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", false);
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", false);
    pView->SetExtractorProperty("EditorGridExtractor", "Active", false);
    pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", true);
  }

  PLASMA_LOCK(m_pSceneContext->GetWorld()->GetWriteMarker());
  const plCameraComponentManager* pCamMan = m_pSceneContext->GetWorld()->GetComponentManager<plCameraComponentManager>();
  if (pCamMan)
  {
    for (auto it = pCamMan->GetComponents(); it.IsValid(); ++it)
    {
      const plCameraComponent* pCamComp = it;

      if (pCamComp->GetUsageHint() == plCameraUsageHint::Thumbnail)
      {
        m_Camera.LookAt(pCamComp->GetOwner()->GetGlobalPosition(), pCamComp->GetOwner()->GetGlobalPosition() + pCamComp->GetOwner()->GetGlobalDirForwards(), pCamComp->GetOwner()->GetGlobalDirUp());

        m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovY, 70.0f, 0.1f, 100.0f);

        m_CullingCamera = m_Camera;
        return true;
      }
    }
  }

  bool bResult = !FocusCameraOnObject(m_Camera, bounds, 70.0f, -plVec3(5, -2, 3));
  m_CullingCamera = m_Camera;
  return bResult;
}

void plSceneViewContext::SetInvisibleLayerTags(const plArrayPtr<plTag> removeTags, const plArrayPtr<plTag> addTags)
{
  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView))
  {
    plTagSet& excludeTags = pView->m_ExcludeTags;
    for (const plTag& removeTag : removeTags)
    {
      excludeTags.Remove(removeTag);
    }
    for (const plTag& addTag : addTags)
    {
      excludeTags.Set(addTag);
    }
  }
}

void plSceneViewContext::Redraw(bool bRenderEditorGizmos)
{
  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView))
  {
    const plTag& tagNoOrtho = plTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    if (pView->GetCamera()->IsOrthographic())
    {
      pView->m_ExcludeTags.Set(tagNoOrtho);
    }
    else
    {
      pView->m_ExcludeTags.Remove(tagNoOrtho);
    }

    PLASMA_LOCK(pView->GetWorld()->GetWriteMarker());
    if (auto pGizmoManager = pView->GetWorld()->GetComponentManager<plGizmoComponentManager>())
    {
      pGizmoManager->m_uiHighlightID = GetDocumentContext()->m_Context.m_uiHighlightID;
    }
  }

  plEngineProcessViewContext::Redraw(bRenderEditorGizmos);
}

void plSceneViewContext::SetCamera(const plViewRedrawMsgToEngine* pMsg)
{
  plEngineProcessViewContext::SetCamera(pMsg);

  plView* pView = nullptr;
  plRenderWorld::TryGetView(m_hView, pView);

  bool bDebugCulling = false;
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  bDebugCulling = plRenderPipeline::cvar_SpatialCullingVis;
#endif

  if (bDebugCulling && pView != nullptr)
  {
    if (const plCameraComponentManager* pCameraManager = pView->GetWorld()->GetComponentManager<plCameraComponentManager>())
    {
      if (const plCameraComponent* pCameraComponent = pCameraManager->GetCameraByUsageHint(plCameraUsageHint::Culling))
      {
        const plGameObject* pOwner = pCameraComponent->GetOwner();
        plVec3 vPosition = pOwner->GetGlobalPosition();
        plVec3 vForward = pOwner->GetGlobalDirForwards();
        plVec3 vUp = pOwner->GetGlobalDirUp();

        m_CullingCamera.LookAt(vPosition, vPosition + vForward, vUp);
      }
    }
  }
  else
  {
    m_CullingCamera = m_Camera;
  }

  if (pView != nullptr)
  {
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", m_pSceneContext->GetRenderSelectionOverlay());
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", m_pSceneContext->GetRenderShapeIcons());
  }
}

plViewHandle plSceneViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  plVariant sceneContextVariant(m_pSceneContext);
  pView->SetExtractorProperty("EditorSelectedObjectsExtractor", "SceneContext", sceneContextVariant);
  pView->SetExtractorProperty("EditorShapeIconsExtractor", "SceneContext", sceneContextVariant);
  pView->SetExtractorProperty("EditorGridExtractor", "SceneContext", sceneContextVariant);

  plEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  pView->SetCullingCamera(&m_CullingCamera);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  const plTag& tagHidden = plTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden");

  pView->m_ExcludeTags.Set(tagHidden);
  return pView->GetHandle();
}

void plSceneViewContext::PickObjectAt(plUInt16 x, plUInt16 y)
{
  // remote processes do not support picking, just ignore this
  if (plEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  plViewPickingResultMsgToEditor res;

  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView) && pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "PickingMatrix"))
  {
    pView->SetRenderPassProperty("EditorPickingPass", "PickingPosition", plVec2(x, y));
    plVariant varMat = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingMatrix");

    if (varMat.IsA<plMat4>())
    {
      const plUInt32 uiPickingID = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingID").ConvertTo<plUInt32>();
      // const float fPickingDepth = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingDepth").ConvertTo<float>();
      res.m_vPickedNormal = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingNormal").ConvertTo<plVec3>();
      res.m_vPickingRayStartPosition = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingRayStartPosition").ConvertTo<plVec3>();
      res.m_vPickedPosition = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingPosition").ConvertTo<plVec3>();

      PLASMA_ASSERT_DEBUG(!res.m_vPickedPosition.IsNaN(), "");

      const plUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);
      const plUInt32 uiPartIndex = (uiPickingID >> 24) & 0x7F; // highest bit indicates whether the object is dynamic, ignore this

      plArrayPtr<plWorldRttiConverterContext*> contexts = m_pSceneContext->GetAllContexts();
      for (plWorldRttiConverterContext* pContext : contexts)
      {
        res.m_ComponentGuid = pContext->m_ComponentPickingMap.GetGuid(uiComponentID);
        if (res.m_ComponentGuid.IsValid())
        {
          plComponentHandle hComponent = pContext->m_ComponentMap.GetHandle(res.m_ComponentGuid);

          plEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

          // check whether the component is still valid
          plComponent* pComponent = nullptr;
          if (pDocumentContext->GetWorld()->TryGetComponent<plComponent>(hComponent, pComponent))
          {
            // if yes, fill out the parent game object guid
            res.m_ObjectGuid = pContext->m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle());
            res.m_uiPartIndex = uiPartIndex;
          }
          else
          {
            res.m_ComponentGuid = plUuid();
          }
          break;
        }
      }
      // Always take the other picking ID from the scene itself as gizmos are handled by the window and only the scene itself has one.
      res.m_OtherGuid = m_pSceneContext->m_Context.m_OtherPickingMap.GetGuid(uiComponentID);
    }
  }

  SendViewMessage(&res);
}

void plSceneViewContext::MarqueePickObjects(const plViewMarqueePickingMsgToEngine* pMsg)
{
  // remote processes do not support picking, just ignore this
  if (plEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  plViewMarqueePickingResultMsgToEditor res;
  res.m_uiWhatToDo = pMsg->m_uiWhatToDo;
  res.m_uiActionIdentifier = 0;

  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetRenderPassProperty("EditorPickingPass", "MarqueePickPos0", plVec2(pMsg->m_uiPickPosX0, pMsg->m_uiPickPosY0));
    pView->SetRenderPassProperty("EditorPickingPass", "MarqueePickPos1", plVec2(pMsg->m_uiPickPosX1, pMsg->m_uiPickPosY1));
    pView->SetRenderPassProperty("EditorPickingPass", "MarqueeActionID", pMsg->m_uiActionIdentifier);

    if (pMsg->m_uiWhatToDo == 0xFF)
      return;

    if (!pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "MarqueeActionID") || pView->GetRenderPassReadBackProperty("EditorPickingPass", "MarqueeActionID").ConvertTo<plUInt32>() != pMsg->m_uiActionIdentifier)
      return;

    res.m_uiActionIdentifier = pMsg->m_uiActionIdentifier;

    plVariant varMarquee = pView->GetRenderPassReadBackProperty("EditorPickingPass", "MarqueeResult");

    if (varMarquee.IsA<plVariantArray>())
    {
      plEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

      const plVariantArray resArray = varMarquee.Get<plVariantArray>();

      for (plUInt32 i = 0; i < resArray.GetCount(); ++i)
      {
        const plVariant& singleRes = resArray[i];

        const plUInt32 uiPickingID = singleRes.ConvertTo<plUInt32>();
        const plUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);

        const plUuid componentGuid = m_pSceneContext->GetActiveContext().m_ComponentPickingMap.GetGuid(uiComponentID);

        if (componentGuid.IsValid())
        {
          plComponentHandle hComponent = m_pSceneContext->GetActiveContext().m_ComponentMap.GetHandle(componentGuid);

          // check whether the component is still valid
          plComponent* pComponent = nullptr;
          if (pDocumentContext->GetWorld()->TryGetComponent<plComponent>(hComponent, pComponent))
          {
            // if yes, fill out the parent game object guid
            res.m_ObjectGuids.PushBack(m_pSceneContext->GetActiveContext().m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle()));
          }
        }
      }
    }
  }

  if (res.m_uiActionIdentifier == 0)
    return;

  SendViewMessage(&res);
}
