#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plMaterialViewContext::plMaterialViewContext(plMaterialContext* pMaterialContext)
  : plEngineProcessViewContext(pMaterialContext)
{
  m_pMaterialContext = pMaterialContext;
}

plMaterialViewContext::~plMaterialViewContext() = default;

void plMaterialViewContext::PositionThumbnailCamera()
{
  m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(plVec3(+0.23f, -0.23f, 0.1f), plVec3::MakeZero(), plVec3(0.0f, 0.0f, 1.0f));
}

plViewHandle plMaterialViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Material Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());
  pView->SetShaderPermutationVariable("MATERIAL_PREVIEW", "TRUE");

  plEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
