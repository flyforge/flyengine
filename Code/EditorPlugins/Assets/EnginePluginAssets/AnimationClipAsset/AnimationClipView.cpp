#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plAnimationClipViewContext::plAnimationClipViewContext(plAnimationClipContext* pContext)
  : PlasmaEngineProcessViewContext(pContext)
{
  m_pContext = pContext;

  // Start with something valid.
  m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(plVec3(1, 1, 1), plVec3::ZeroVector(), plVec3(0.0f, 0.0f, 1.0f));
}

plAnimationClipViewContext::~plAnimationClipViewContext() {}

bool plAnimationClipViewContext::UpdateThumbnailCamera(const plBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -plVec3(5, -2, 3));
}


plViewHandle plAnimationClipViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Animation Clip Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  PlasmaEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void plAnimationClipViewContext::SetCamera(const plViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    PlasmaEngineProcessViewContext::DrawSimpleGrid();
  }

  PlasmaEngineProcessViewContext::SetCamera(pMsg);
}
