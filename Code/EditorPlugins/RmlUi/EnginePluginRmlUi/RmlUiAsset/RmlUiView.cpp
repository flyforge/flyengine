#include <EnginePluginRmlUi/EnginePluginRmlUiPCH.h>

#include <EnginePluginRmlUi/RmlUiAsset/RmlUiContext.h>
#include <EnginePluginRmlUi/RmlUiAsset/RmlUiView.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plRmlUiViewContext::plRmlUiViewContext(plRmlUiDocumentContext* pRmlUiContext)
  : plEngineProcessViewContext(pRmlUiContext)
{
  m_pRmlUiContext = pRmlUiContext;

  // Start with something valid.
  m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 45.0f, 0.01f, 1000.0f);
  m_Camera.LookAt(plVec3(1, 1, 1), plVec3::MakeZero(), plVec3(0.0f, 0.0f, 1.0f));
}

plRmlUiViewContext::~plRmlUiViewContext() = default;

bool plRmlUiViewContext::UpdateThumbnailCamera(const plBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -plVec3(5, -2, 3));
}

plViewHandle plRmlUiViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Rml Ui Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  plEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);
  return pView->GetHandle();
}

void plRmlUiViewContext::SetCamera(const plViewRedrawMsgToEngine* pMsg)
{
  plEngineProcessViewContext::SetCamera(pMsg);

  /*const plUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hResource = m_pRmlUiContext->GetResource();
  if (hResource.IsValid())
  {
    plResourceLock<plRmlUiResource> pResource(hResource, plResourceAcquireMode::AllowLoadingFallback);

    if (pResource->GetDetails().m_Bounds.IsValid())
    {
      const plBoundingBox& bbox = pResource->GetDetails().m_Bounds.GetBox();

      plStringBuilder sText;
      sText.PrependFormat("Bounding Box: width={0}, depth={1}, height={2}", plArgF(bbox.GetHalfExtents().x * 2, 2),
                          plArgF(bbox.GetHalfExtents().y * 2, 2), plArgF(bbox.GetHalfExtents().z * 2, 2));

      plDebugRenderer::DrawInfoText(m_hView, sText, plVec2I32(10, viewHeight - 26), plColor::White);
    }
  }*/
}
