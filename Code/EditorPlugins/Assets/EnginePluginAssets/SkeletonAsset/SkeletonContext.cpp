#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>

#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/SkeletonComponent.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSkeletonContext, 1, plRTTIDefaultAllocator<plSkeletonContext>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_CONSTANT_PROPERTY("DocumentType", (const char*) "Skeleton"),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSkeletonContext::plSkeletonContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
}

void plSkeletonContext::HandleMessage(const plEditorEngineDocumentMsg* pDocMsg)
{
  if (auto pMsg = plDynamicCast<const plQuerySelectionBBoxMsgToEngine*>(pDocMsg))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = plDynamicCast<const plSimpleDocumentConfigMsgToEngine*>(pDocMsg))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState")
    {
      if (pMsg->m_sPayload == "Grid")
      {
        m_bDisplayGrid = pMsg->m_fPayload > 0;
        return;
      }
    }
    else if (pMsg->m_sWhatToDo == "HighlightBones")
    {
      PL_LOCK(m_pWorld->GetWriteMarker());

      plSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->SetBonesToHighlight(pMsg->m_sPayload);
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderBones")
    {
      PL_LOCK(m_pWorld->GetWriteMarker());

      plSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeBones = (pMsg->m_fPayload != 0);
      }

      // resend the pose every frame (this config message is send every frame)
      // this ensures that changing any of the visualization states in the skeleton component displays correctly
      // a bit hacky and should be cleaned up, but this way the skeleton component doesn't need to keep a copy of the last pose (maybe it should)
      plSkeletonPoseComponent* pPoseSkeleton;
      if (m_pWorld->TryGetComponent(m_hPoseComponent, pPoseSkeleton))
      {
        pPoseSkeleton->ResendPose();
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderColliders")
    {
      PL_LOCK(m_pWorld->GetWriteMarker());

      plSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeColliders = (pMsg->m_fPayload != 0);
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderJoints")
    {
      PL_LOCK(m_pWorld->GetWriteMarker());

      plSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeJoints = (pMsg->m_fPayload != 0);
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderSwingLimits")
    {
      PL_LOCK(m_pWorld->GetWriteMarker());

      plSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeSwingLimits = (pMsg->m_fPayload != 0);
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderTwistLimits")
    {
      PL_LOCK(m_pWorld->GetWriteMarker());

      plSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeTwistLimits = (pMsg->m_fPayload != 0);
      }
    }
    else if (pMsg->m_sWhatToDo == "PreviewMesh" && m_sAnimatedMeshToUse != pMsg->m_sPayload)
    {
      m_sAnimatedMeshToUse = pMsg->m_sPayload;

      auto pWorld = m_pWorld;
      PL_LOCK(pWorld->GetWriteMarker());

      plAnimatedMeshComponent* pAnimMesh;
      if (pWorld->TryGetComponent(m_hAnimMeshComponent, pAnimMesh))
      {
        m_hAnimMeshComponent.Invalidate();
        pAnimMesh->DeleteComponent();
      }

      if (!m_sAnimatedMeshToUse.IsEmpty())
      {
        plMaterialResourceHandle hMat = plResourceManager::LoadResource<plMaterialResource>("Editor/Materials/SkeletonPreviewMesh.plMaterial");

        m_hAnimMeshComponent = plAnimatedMeshComponent::CreateComponent(m_pGameObject, pAnimMesh);
        pAnimMesh->SetMeshFile(m_sAnimatedMeshToUse);

        for (int i = 0; i < 10; ++i)
        {
          pAnimMesh->SetMaterial(i, hMat);
        }
      }
    }
  }

  plEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void plSkeletonContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  PL_LOCK(pWorld->GetWriteMarker());

  plGameObjectDesc obj;
  plSkeletonComponent* pVisSkeleton;
  plSkeletonPoseComponent* pPoseSkeleton;

  // Preview Mesh
  {
    obj.m_sName.Assign("SkeletonPreview");
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pGameObject);

    m_hSkeletonComponent = plSkeletonComponent::CreateComponent(m_pGameObject, pVisSkeleton);
    plStringBuilder sSkeletonGuid;
    plConversionUtils::ToString(GetDocumentGuid(), sSkeletonGuid);
    m_hSkeleton = plResourceManager::LoadResource<plSkeletonResource>(sSkeletonGuid);
    pVisSkeleton->SetSkeleton(m_hSkeleton);
    pVisSkeleton->m_bVisualizeColliders = true;

    m_hPoseComponent = plSkeletonPoseComponent::CreateComponent(m_pGameObject, pPoseSkeleton);
    pPoseSkeleton->SetSkeleton(m_hSkeleton);
    pPoseSkeleton->SetPoseMode(plSkeletonPoseMode::RestPose);
  }
}

plEngineProcessViewContext* plSkeletonContext::CreateViewContext()
{
  return PL_DEFAULT_NEW(plSkeletonViewContext, this);
}

void plSkeletonContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PL_DEFAULT_DELETE(pContext);
}

bool plSkeletonContext::UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext)
{
  plBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  plSkeletonViewContext* pMeshViewContext = static_cast<plSkeletonViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void plSkeletonContext::QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg)
{
  if (m_pGameObject == nullptr)
    return;

  plBoundingBoxSphere bounds = plBoundingBoxSphere::MakeInvalid();

  {
    PL_LOCK(m_pWorld->GetWriteMarker());

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
