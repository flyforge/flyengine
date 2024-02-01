#include <EnginePluginKraut/EnginePluginKrautPCH.h>

#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plKrautTreeViewContext::plKrautTreeViewContext(plKrautTreeContext* pKrautTreeContext)
  : plEngineProcessViewContext(pKrautTreeContext)
{
  m_pKrautTreeContext = pKrautTreeContext;

  // Start with something valid.
  m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(plVec3(1, 1, 1), plVec3::MakeZero(), plVec3(0.0f, 0.0f, 1.0f));
}

plKrautTreeViewContext::~plKrautTreeViewContext() = default;

bool plKrautTreeViewContext::UpdateThumbnailCamera(const plBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -plVec3(5, -2, 3));
}

plViewHandle plKrautTreeViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Kraut Tree Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  plEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  pView->SetCameraUsageHint(plCameraUsageHint::Thumbnail);
  return pView->GetHandle();
}

void plKrautTreeViewContext::SetCamera(const plViewRedrawMsgToEngine* pMsg)
{
  plEngineProcessViewContext::SetCamera(pMsg);

  // const plUInt32 viewHeight = pMsg->m_uiWindowHeight;

  plBoundingBox bbox = plBoundingBox::MakeFromCenterAndHalfExtents(plVec3::MakeZero(), plVec3::MakeZero());

  auto hResource = m_pKrautTreeContext->GetResource();
  if (hResource.IsValid())
  {
    // plResourceLock<plKrautGeneratorResource> pResource(hResource, plResourceAcquireMode::AllowLoadingFallback);

    // TODO

    // if (pResource->GetDetails().m_Bounds.IsValid())
    //{
    //   bbox = pResource->GetDetails().m_Bounds.GetBox();

    //  plStringBuilder sText;
    //  sText.PrependFormat("Bounding Box: width={0}, depth={1}, height={2}", plArgF(bbox.GetHalfExtents().x * 2, 2),
    //    plArgF(bbox.GetHalfExtents().y * 2, 2), plArgF(bbox.GetHalfExtents().z * 2, 2));

    //  plDebugRenderer::Draw2DText(m_hView, sText, plVec2I32(10, viewHeight - 26), plColor::White);
    //}
  }
}
