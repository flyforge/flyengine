#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/RenderPipeline/EditorSelectedObjectsExtractor.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/SwapChain.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorSelectedObjectsExtractor, 1, plRTTIDefaultAllocator<plEditorSelectedObjectsExtractor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plEditorSelectedObjectsExtractor::plEditorSelectedObjectsExtractor()
{
  m_pSceneContext = nullptr;
}

plEditorSelectedObjectsExtractor::~plEditorSelectedObjectsExtractor()
{
  plRenderWorld::DeleteView(m_hRenderTargetView);
}

const plDeque<plGameObjectHandle>* plEditorSelectedObjectsExtractor::GetSelection()
{
  if (m_pSceneContext == nullptr)
    return nullptr;

  return &m_pSceneContext->GetSelectionWithChildren();
}

void plEditorSelectedObjectsExtractor::Extract(
  const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& extractedRenderData)
{
  const bool bShowCameraOverlays = view.GetCameraUsageHint() == plCameraUsageHint::EditorView;

  if (bShowCameraOverlays && m_pSceneContext && m_pSceneContext->GetRenderSelectionBoxes())
  {
    const plDeque<plGameObjectHandle>* pSelection = GetSelection();
    if (pSelection == nullptr)
      return;

    const plCameraComponent* pCamComp = nullptr;

    CreateRenderTargetTexture(view);

    PLASMA_LOCK(view.GetWorld()->GetReadMarker());

    for (const auto& hObj : *pSelection)
    {
      const plGameObject* pObject = nullptr;
      if (!view.GetWorld()->TryGetObject(hObj, pObject))
        continue;

      if (FilterByViewTags(view, pObject))
        continue;

      if (pObject->TryGetComponentOfBaseType(pCamComp))
      {
        UpdateRenderTargetCamera(pCamComp);

        const float fAspect = 9.0f / 16.0f;

        // TODO: use aspect ratio of camera render target, if available
        plDebugRenderer::Draw2DRectangle(view.GetHandle(), plRectFloat(20, 20, 256, 256 * fAspect), 0, plColor::White, m_hRenderTarget);

        // TODO: if the camera renders to a texture anyway, use its view + render target instead

        plRenderWorld::AddViewToRender(m_hRenderTargetView);

        break;
      }
    }
  }

  plSelectedObjectsExtractorBase::Extract(view, visibleObjects, extractedRenderData);
}

void plEditorSelectedObjectsExtractor::CreateRenderTargetTexture(const plView& view)
{
  if (m_hRenderTarget.IsValid())
    return;

  m_hRenderTarget = plResourceManager::GetExistingResource<plRenderToTexture2DResource>("EditorCameraRT");

  if (!m_hRenderTarget.IsValid())
  {
    const float fAspect = 9.0f / 16.0f;
    const plUInt32 uiWidth = 256;

    plRenderToTexture2DResourceDescriptor d;
    d.m_Format = plGALResourceFormat::RGBAUByteNormalizedsRGB;
    d.m_uiWidth = uiWidth;
    d.m_uiHeight = (plUInt32)(uiWidth * fAspect);

    m_hRenderTarget = plResourceManager::GetOrCreateResource<plRenderToTexture2DResource>("EditorCameraRT", std::move(d));
  }

  CreateRenderTargetView(view);
}

void plEditorSelectedObjectsExtractor::CreateRenderTargetView(const plView& view)
{
  PLASMA_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  plResourceLock<plRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, plResourceAcquireMode::BlockTillLoaded);

  plStringBuilder name("EditorCameraRT");

  plView* pRenderTargetView = nullptr;
  m_hRenderTargetView = plRenderWorld::CreateView(name, pRenderTargetView);

  // MainRenderPipeline.plRenderPipelineAsset
  auto hRenderPipeline = plResourceManager::LoadResource<plRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
  pRenderTargetView->SetRenderPipelineResource(hRenderPipeline);

  // TODO: get rid of const cast ?
  pRenderTargetView->SetWorld(const_cast<plWorld*>(view.GetWorld()));
  pRenderTargetView->SetCamera(&m_RenderTargetCamera);

  m_RenderTargetCamera.SetCameraMode(plCameraMode::PerspectiveFixedFovY, 45, 0.1f, 100.0f);

  plGALRenderTargets renderTargets;
  renderTargets.m_hRTs[0] = pRenderTarget->GetGALTexture();
  pRenderTargetView->SetRenderTargets(renderTargets);

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  pRenderTargetView->SetViewport(plRectFloat(0, 0, resX, resY));
}

void plEditorSelectedObjectsExtractor::UpdateRenderTargetCamera(const plCameraComponent* pCamComp)
{
  float fFarPlane = plMath::Max(pCamComp->GetNearPlane() + 0.00001f, pCamComp->GetFarPlane());
  switch (pCamComp->GetCameraMode())
  {
    case plCameraMode::OrthoFixedHeight:
    case plCameraMode::OrthoFixedWidth:
      m_RenderTargetCamera.SetCameraMode(pCamComp->GetCameraMode(), pCamComp->GetOrthoDimension(), pCamComp->GetNearPlane(), fFarPlane);
      break;
    case plCameraMode::PerspectiveFixedFovX:
    case plCameraMode::PerspectiveFixedFovY:
      m_RenderTargetCamera.SetCameraMode(pCamComp->GetCameraMode(), pCamComp->GetFieldOfView(), pCamComp->GetNearPlane(), fFarPlane);
      break;
    case plCameraMode::Stereo:
      m_RenderTargetCamera.SetCameraMode(plCameraMode::PerspectiveFixedFovY, 45, pCamComp->GetNearPlane(), fFarPlane);
      break;
    default:
      break;
  }


  plView* pRenderTargetView = nullptr;
  if (!plRenderWorld::TryGetView(m_hRenderTargetView, pRenderTargetView))
    return;

  pRenderTargetView->m_IncludeTags = pCamComp->m_IncludeTags;
  pRenderTargetView->m_ExcludeTags = pCamComp->m_ExcludeTags;
  pRenderTargetView->m_ExcludeTags.SetByName("Editor");

  if (pCamComp->GetRenderPipeline().IsValid())
  {
    pRenderTargetView->SetRenderPipelineResource(pCamComp->GetRenderPipeline());
  }
  else
  {
    // MainRenderPipeline.plRenderPipelineAsset
    auto hRenderPipeline = plResourceManager::LoadResource<plRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
    pRenderTargetView->SetRenderPipelineResource(hRenderPipeline);
  }

  const plVec3 pos = pCamComp->GetOwner()->GetGlobalPosition();
  const plVec3 dir = pCamComp->GetOwner()->GetGlobalDirForwards();
  const plVec3 up = pCamComp->GetOwner()->GetGlobalDirUp();

  m_RenderTargetCamera.LookAt(pos, pos + dir, up);
  m_RenderTargetCamera.SetShutterSpeed(pCamComp->GetShutterTime().AsFloatInSeconds());
  m_RenderTargetCamera.SetExposure(pCamComp->GetExposure());
  m_RenderTargetCamera.SetAperture(pCamComp->GetAperture());
  m_RenderTargetCamera.SetISO(pCamComp->GetISO());
  //m_RenderTargetCamera.SetFocusDistance(pCamComp->GetFocusDistance());
}
