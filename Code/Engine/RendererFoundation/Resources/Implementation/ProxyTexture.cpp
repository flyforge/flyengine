#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ProxyTexture.h>

namespace
{
  plGALTextureCreationDescription MakeProxyDesc(const plGALTextureCreationDescription& parentDesc)
  {
    plGALTextureCreationDescription desc = parentDesc;
    desc.m_Type = plGALTextureType::Texture2DProxy;
    return desc;
  }
} // namespace

plGALProxyTexture::plGALProxyTexture(const plGALTexture& parentTexture)
  : plGALTexture(MakeProxyDesc(parentTexture.GetDescription()))
  , m_pParentTexture(&parentTexture)
{
}

plGALProxyTexture::~plGALProxyTexture() = default;


const plGALResourceBase* plGALProxyTexture::GetParentResource() const
{
  return m_pParentTexture;
}

plResult plGALProxyTexture::InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData)
{
  return PL_SUCCESS;
}

plResult plGALProxyTexture::DeInitPlatform(plGALDevice* pDevice)
{
  return PL_SUCCESS;
}

void plGALProxyTexture::SetDebugNamePlatform(const char* szName) const {}


