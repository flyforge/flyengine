#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>

#include <GameEngine/Animation/Skeletal/SimpleAnimationComponent.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimationClipContext, 1, plRTTIDefaultAllocator<plAnimationClipContext>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_CONSTANT_PROPERTY("DocumentType", (const char*) "Animation Clip"),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAnimationClipContext::plAnimationClipContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
}

void plAnimationClipContext::HandleMessage(const plEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = plDynamicCast<const plQuerySelectionBBoxMsgToEngine*>(pMsg0))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = plDynamicCast<const plSimpleDocumentConfigMsgToEngine*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState")
    {
      if (pMsg->m_sPayload == "Grid")
      {
        m_bDisplayGrid = pMsg->m_fPayload > 0;
      }
    }
    else if (pMsg->m_sWhatToDo == "PreviewMesh" && m_sAnimatedMeshToUse != pMsg->m_sPayload)
    {
      m_sAnimatedMeshToUse = pMsg->m_sPayload;

      auto pWorld = m_pWorld;
      PLASMA_LOCK(pWorld->GetWriteMarker());

      plStringBuilder sAnimClipGuid;
      plConversionUtils::ToString(GetDocumentGuid(), sAnimClipGuid);

      plAnimatedMeshComponent* pAnimMesh;
      if (pWorld->TryGetComponent(m_hAnimMeshComponent, pAnimMesh))
      {
        pAnimMesh->DeleteComponent();
        m_hAnimMeshComponent.Invalidate();
      }

      plSimpleAnimationComponent* pAnimController;
      if (pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
      {
        pAnimController->DeleteComponent();
        m_hAnimControllerComponent.Invalidate();
      }

      if (!m_sAnimatedMeshToUse.IsEmpty())
      {
        m_hAnimMeshComponent = plAnimatedMeshComponent::CreateComponent(m_pGameObject, pAnimMesh);
        m_hAnimControllerComponent = plSimpleAnimationComponent::CreateComponent(m_pGameObject, pAnimController);

        pAnimMesh->SetMeshFile(m_sAnimatedMeshToUse);
        pAnimController->SetAnimationClipFile(sAnimClipGuid);
      }
    }
    else if (pMsg->m_sWhatToDo == "PlaybackPos")
    {
      SetPlaybackPosition(pMsg->m_fPayload);
    }

    return;
  }

  if (auto pMsg = plDynamicCast<const plViewRedrawMsgToEngine*>(pMsg0))
  {
    auto pWorld = m_pWorld;
    PLASMA_LOCK(pWorld->GetWriteMarker());

    plSimpleAnimationComponent* pAnimController;
    if (pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
    {
      if (pAnimController->GetAnimationClip().IsValid())
      {
        plResourceLock<plAnimationClipResource> pResource(pAnimController->GetAnimationClip(), plResourceAcquireMode::AllowLoadingFallback_NeverFail);

        if (pResource.GetAcquireResult() == plResourceAcquireResult::Final)
        {
          plSimpleDocumentConfigMsgToEditor msg;
          msg.m_DocumentGuid = pMsg->m_DocumentGuid;
          msg.m_sName = "ClipDuration";
          msg.m_fPayload = pResource->GetDescriptor().GetDuration().GetSeconds();

          SendProcessMessage(&msg);
        }
      }
    }
  }

  plEngineProcessDocumentContext::HandleMessage(pMsg0);
}

void plAnimationClipContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  PLASMA_LOCK(pWorld->GetWriteMarker());

  plGameObjectDesc obj;

  // Preview
  {
    obj.m_bDynamic = true;
    obj.m_sName.Assign("SkeletonPreview");
    pWorld->CreateObject(obj, m_pGameObject);
  }
}

plEngineProcessViewContext* plAnimationClipContext::CreateViewContext()
{
  return PLASMA_DEFAULT_NEW(plAnimationClipViewContext, this);
}

void plAnimationClipContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PLASMA_DEFAULT_DELETE(pContext);
}

bool plAnimationClipContext::UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext)
{
  plBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  if (!m_hAnimControllerComponent.IsInvalidated())
  {
    PLASMA_LOCK(m_pWorld->GetWriteMarker());

    plSimpleAnimationComponent* pAnimController;
    if (m_pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
    {
      pAnimController->SetNormalizedPlaybackPosition(0.5f);
      pAnimController->m_fSpeed = 0.0f;

      m_pWorld->SetWorldSimulationEnabled(true);
      m_pWorld->Update();
      m_pWorld->SetWorldSimulationEnabled(false);
    }
  }

  plAnimationClipViewContext* pMeshViewContext = static_cast<plAnimationClipViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void plAnimationClipContext::QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg)
{
  if (m_pGameObject == nullptr)
    return;

  plBoundingBoxSphere bounds = plBoundingBoxSphere::MakeInvalid();

  {
    PLASMA_LOCK(m_pWorld->GetWriteMarker());

    m_pGameObject->UpdateLocalBounds();
    m_pGameObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pGameObject->GetGlobalBounds();

    if (b.IsValid())
      bounds.ExpandToInclude(b);
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

void plAnimationClipContext::SetPlaybackPosition(double pos)
{
  PLASMA_LOCK(m_pWorld->GetWriteMarker());

  plSimpleAnimationComponent* pAnimController;
  if (m_pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
  {
    pAnimController->SetNormalizedPlaybackPosition(static_cast<float>(pos));
    pAnimController->m_fSpeed = 0.0f;
  }
}
