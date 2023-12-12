#include <EnginePluginParticle/EnginePluginParticlePCH.h>

#include <EnginePluginParticle/ParticleAsset/ParticleContext.h>
#include <EnginePluginParticle/ParticleAsset/ParticleView.h>
#include <RendererCore/Pipeline/View.h>

plParticleViewContext::plParticleViewContext(plParticleContext* pParticleContext)
  : PlasmaEngineProcessViewContext(pParticleContext)
{
  m_pParticleContext = pParticleContext;
}

plParticleViewContext::~plParticleViewContext() {}

void plParticleViewContext::PositionThumbnailCamera(const plBoundingBoxSphere& bounds)
{
  m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);

  FocusCameraOnObject(m_Camera, bounds, 45.0f, -plVec3(-1.8f, 1.8f, 1.0f));
}

plViewHandle plParticleViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Particle Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  PlasmaEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
