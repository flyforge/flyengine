#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/RenderTargetActivatorComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plRenderTargetActivatorComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("RenderTarget", GetRenderTargetFile, SetRenderTargetFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_Target")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plRenderTargetActivatorComponent::plRenderTargetActivatorComponent() = default;
plRenderTargetActivatorComponent::~plRenderTargetActivatorComponent() = default;

void plRenderTargetActivatorComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_hRenderTarget;
}

void plRenderTargetActivatorComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();

  s >> m_hRenderTarget;
}

plResult plRenderTargetActivatorComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  if (m_hRenderTarget.IsValid())
  {
    ref_bounds = plBoundingSphere(plVec3::ZeroVector(), 0.1f);
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

void plRenderTargetActivatorComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // only add render target views from main views
  // otherwise every shadow casting light source would activate a render target
  if (msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::EditorView)
    return;

  if (!m_hRenderTarget.IsValid())
    return;

  plResourceLock<plRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, plResourceAcquireMode::BlockTillLoaded);

  for (auto hView : pRenderTarget->GetAllRenderViews())
  {
    plRenderWorld::AddViewToRender(hView);
  }
}

void plRenderTargetActivatorComponent::SetRenderTarget(const plRenderToTexture2DResourceHandle& hResource)
{
  m_hRenderTarget = hResource;

  TriggerLocalBoundsUpdate();
}

void plRenderTargetActivatorComponent::SetRenderTargetFile(const char* szFile)
{
  plRenderToTexture2DResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plRenderToTexture2DResource>(szFile);
  }

  SetRenderTarget(hResource);
}

const char* plRenderTargetActivatorComponent::GetRenderTargetFile() const
{
  if (!m_hRenderTarget.IsValid())
    return "";

  return m_hRenderTarget.GetResourceID();
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderTargetActivatorComponent);
