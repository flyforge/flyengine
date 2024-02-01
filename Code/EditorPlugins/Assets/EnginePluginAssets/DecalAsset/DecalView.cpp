#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plDecalViewContext::plDecalViewContext(plDecalContext* pDecalContext)
  : plEngineProcessViewContext(pDecalContext)
{
  m_pDecalContext = pDecalContext;
}

plDecalViewContext::~plDecalViewContext() = default;

plViewHandle plDecalViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Decal Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  plEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
