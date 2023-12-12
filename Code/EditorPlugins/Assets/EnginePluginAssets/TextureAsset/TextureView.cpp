#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plTextureViewContext::plTextureViewContext(plTextureContext* pContext)
  : PlasmaEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
}

plTextureViewContext::~plTextureViewContext() {}

plViewHandle plTextureViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Texture Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
  pView->SetRenderPassProperty("DepthPrePass", "Active", false);
  pView->SetRenderPassProperty("AOPass", "Active", false);

  PlasmaEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void plTextureViewContext::SetCamera(const plViewRedrawMsgToEngine* pMsg)
{
  // Do not apply render mode here otherwise we would switch to a different pipeline.
  // Also use hard-coded clipping planes so the quad is not culled too early.

  plCameraMode::Enum cameraMode = (plCameraMode::Enum)pMsg->m_iCameraMode;
  m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, 0.0001f, 50.0f);
  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  // Draw some stats
  auto hResource = m_pTextureContext->GetTexture();
  if (hResource.IsValid())
  {
    plResourceLock<plTexture2DResource> pResource(hResource, plResourceAcquireMode::AllowLoadingFallback);
    plGALResourceFormat::Enum format = pResource->GetFormat();
    plUInt32 uiWidth = pResource->GetWidth();
    plUInt32 uiHeight = pResource->GetHeight();


    auto memoryUsage = pResource->GetMemoryUsage();



    const plUInt32 viewHeight = pMsg->m_uiWindowHeight;

    plStringBuilder sGPUMemoryText;
    sGPUMemoryText.Format("GPU Memory - {0} MB", plMath::RoundUp((float)memoryUsage.m_uiMemoryGPU / 1024.0f / 1024.0f,0.01f));
    plDebugRenderer::DrawInfoText(m_hView, plDebugTextPlacement::BottomLeft, "AssetStats", sGPUMemoryText);

    plStringBuilder sText;
    if (!plReflectionUtils::EnumerationToString(plGetStaticRTTI<plGALResourceFormat>(), format, sText, plReflectionUtils::EnumConversionMode::ValueNameOnly))
    {
      sText = "Unknown format";
    }

    sText.PrependFormat("{0}x{1} - ", uiWidth, uiHeight);
    plDebugRenderer::DrawInfoText(m_hView, plDebugTextPlacement::BottomLeft, "AssetStats", sText);

  }
}
