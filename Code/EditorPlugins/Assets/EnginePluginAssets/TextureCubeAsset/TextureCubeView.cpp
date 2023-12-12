#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plTextureCubeViewContext::plTextureCubeViewContext(plTextureCubeContext* pContext)
  : PlasmaEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
}

plTextureCubeViewContext::~plTextureCubeViewContext() {}

plViewHandle plTextureCubeViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Texture Cube Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
  pView->SetRenderPassProperty("DepthPrePass", "Active", false);
  pView->SetRenderPassProperty("AOPass", "Active", false);

  PlasmaEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void plTextureCubeViewContext::SetCamera(const plViewRedrawMsgToEngine* pMsg)
{
  // Do not apply render mode here otherwise we would switch to a different pipeline.
  // Also use hard-coded clipping planes so the quad is not culled to early.

  plCameraMode::Enum cameraMode = (plCameraMode::Enum)pMsg->m_iCameraMode;
  m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, 0.0001f, 50.0f);
  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  // Draw some stats
  auto hResource = m_pTextureContext->GetTexture();
  if (hResource.IsValid())
  {
    plResourceLock<plTextureCubeResource> pResource(hResource, plResourceAcquireMode::AllowLoadingFallback);
    plGALResourceFormat::Enum format = pResource->GetFormat();
    plUInt32 uiWidthAndHeight = pResource->GetWidthAndHeight();

    const plUInt32 viewHeight = pMsg->m_uiWindowHeight;

    plStringBuilder sText;
    if (!plReflectionUtils::EnumerationToString(plGetStaticRTTI<plGALResourceFormat>(), format, sText, plReflectionUtils::EnumConversionMode::ValueNameOnly))
    {
      sText = "Unknown format";
    }

    sText.PrependFormat("{0}x{1}x6 - ", uiWidthAndHeight, uiWidthAndHeight);

    plDebugRenderer::DrawInfoText(m_hView, plDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
