#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/plTexFormat/plTexFormat.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTexture2DResource, 1, plRTTIDefaultAllocator<plTexture2DResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCVarInt cvar_RenderingOffscreenTargetResolution1("Rendering.Offscreen.TargetResolution1", 256, plCVarFlags::Default, "Configurable render target resolution");
plCVarInt cvar_RenderingOffscreenTargetResolution2("Rendering.Offscreen.TargetResolution2", 512, plCVarFlags::Default, "Configurable render target resolution");

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plTexture2DResource);

plTexture2DResource::plTexture2DResource()
  : plResource(DoUpdate::OnAnyThread, plTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

plTexture2DResource::plTexture2DResource(plResource::DoUpdate ResourceUpdateThread)
  : plResource(ResourceUpdateThread, plTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

plResourceLoadDesc plTexture2DResource::UnloadData(Unload WhatToUnload)
{
  if (m_uiLoadedTextures > 0)
  {
    for (plInt32 r = 0; r < 2; ++r)
    {
      --m_uiLoadedTextures;

      if (!m_hGALTexture[m_uiLoadedTextures].IsInvalidated())
      {
        plGALDevice::GetDefaultDevice()->DestroyTexture(m_hGALTexture[m_uiLoadedTextures]);
        m_hGALTexture[m_uiLoadedTextures].Invalidate();
      }

      m_uiMemoryGPU[m_uiLoadedTextures] = 0;

      if (WhatToUnload == Unload::OneQualityLevel || m_uiLoadedTextures == 0)
        break;
    }
  }

  if (WhatToUnload == Unload::AllQualityLevels)
  {
    if (!m_hSamplerState.IsInvalidated())
    {
      plGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
      m_hSamplerState.Invalidate();
    }
  }

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable = 2 - m_uiLoadedTextures;
  res.m_State = m_uiLoadedTextures == 0 ? plResourceState::Unloaded : plResourceState::Loaded;
  return res;
}

void plTexture2DResource::FillOutDescriptor(plTexture2DResourceDescriptor& ref_td, const plImage* pImage, bool bSRGB, plUInt32 uiNumMipLevels,
  plUInt32& out_uiMemoryUsed, plHybridArray<plGALSystemMemoryDescription, 32>& ref_initData)
{
  const plUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  const plGALResourceFormat::Enum format = plTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), bSRGB);

  ref_td.m_DescGAL.m_Format = format;
  ref_td.m_DescGAL.m_uiWidth = pImage->GetWidth(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiHeight = pImage->GetHeight(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiDepth = pImage->GetDepth(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiMipLevelCount = uiNumMipLevels;
  ref_td.m_DescGAL.m_uiArraySize = pImage->GetNumArrayIndices();

  if (plImageFormat::GetType(pImage->GetImageFormat()) == plImageFormatType::BLOCK_COMPRESSED)
  {
    ref_td.m_DescGAL.m_uiWidth = plMath::RoundUp(ref_td.m_DescGAL.m_uiWidth, 4);
    ref_td.m_DescGAL.m_uiHeight = plMath::RoundUp(ref_td.m_DescGAL.m_uiHeight, 4);
  }

  if (ref_td.m_DescGAL.m_uiDepth > 1)
    ref_td.m_DescGAL.m_Type = plGALTextureType::Texture3D;

  if (pImage->GetNumFaces() == 6)
    ref_td.m_DescGAL.m_Type = plGALTextureType::TextureCube;

  PLASMA_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces");

  out_uiMemoryUsed = 0;

  ref_initData.Clear();

  for (plUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (plUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (plUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        plGALSystemMemoryDescription& id = ref_initData.ExpandAndGetRef();

        id.m_pData = const_cast<plUInt8*>(pImage->GetPixelPointer<plUInt8>(mip, face, array_index));

        if (plImageFormat::GetType(pImage->GetImageFormat()) == plImageFormatType::BLOCK_COMPRESSED)
        {
          const plUInt32 uiMemPitchFactor = plGALResourceFormat::GetBitsPerElement(format) * 4 / 8;

          id.m_uiRowPitch = plMath::RoundUp(pImage->GetWidth(mip), 4) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiRowPitch = static_cast<plUInt32>(pImage->GetRowPitch(mip));
        }

        PLASMA_ASSERT_DEV(pImage->GetDepthPitch(mip) < plMath::MaxValue<plUInt32>(), "Depth pitch exceeds plGAL limits.");
        id.m_uiSlicePitch = static_cast<plUInt32>(pImage->GetDepthPitch(mip));

        out_uiMemoryUsed += id.m_uiSlicePitch;
      }
    }
  }

  const plArrayPtr<plGALSystemMemoryDescription> InitDataPtr(ref_initData);

  ref_td.m_InitialContent = InitDataPtr;
}


plResourceLoadDesc plTexture2DResource::UpdateContent(plStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    plResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = plResourceState::LoadedResourceMissing;

    return res;
  }

  plTexture2DResourceDescriptor td;
  plImage* pImage = nullptr;
  bool bIsFallback = false;
  plTexFormat texFormat;

  // load image data
  {
    Stream->ReadBytes(&pImage, sizeof(plImage*));
    *Stream >> bIsFallback;
    texFormat.ReadHeader(*Stream);

    td.m_SamplerDesc.m_AddressU = texFormat.m_AddressModeU;
    td.m_SamplerDesc.m_AddressV = texFormat.m_AddressModeV;
    td.m_SamplerDesc.m_AddressW = texFormat.m_AddressModeW;
  }

  const bool bIsRenderTarget = texFormat.m_iRenderTargetResolutionX != 0;
  PLASMA_ASSERT_DEV(!bIsRenderTarget, "Render targets are not supported by regular 2D texture resources");

  {

    const plUInt32 uiNumMipmapsLowRes = plTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : plMath::Min(pImage->GetNumMipLevels(), 6U);
    plUInt32 uiUploadNumMipLevels = 0;
    bool bCouldLoadMore = false;

    if (bIsFallback)
    {
      if (m_uiLoadedTextures == 0)
      {
        // only upload fallback textures, if we don't have any texture data at all, yet
        bCouldLoadMore = true;
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        // ignore this texture entirely, if we already have low res data
        // but assume we could load a higher resolution version
        bCouldLoadMore = true;
        plLog::Debug("Ignoring fallback texture data, low-res resource data is already loaded.");
      }
      else
      {
        plLog::Debug("Ignoring fallback texture data, resource is already fully loaded.");
      }
    }
    else
    {
      if (m_uiLoadedTextures == 0)
      {
        bCouldLoadMore = uiNumMipmapsLowRes < pImage->GetNumMipLevels();
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        uiUploadNumMipLevels = pImage->GetNumMipLevels();
      }
      else
      {
        // ignore the texture, if we already have fully loaded data
        plLog::Debug("Ignoring texture data, resource is already fully loaded.");
      }
    }

    if (uiUploadNumMipLevels > 0)
    {
      PLASMA_ASSERT_DEBUG(m_uiLoadedTextures < 2, "Invalid texture upload");

      plHybridArray<plGALSystemMemoryDescription, 32> initData;
      FillOutDescriptor(td, pImage, texFormat.m_bSRGB, uiUploadNumMipLevels, m_uiMemoryGPU[m_uiLoadedTextures], initData);

      plTextureUtils::ConfigureSampler(static_cast<plTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

      // ignore its return value here, we build our own
      CreateResource(std::move(td));
    }

    {
      plResourceLoadDesc res;
      res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
      res.m_uiQualityLevelsLoadable = bCouldLoadMore ? 1 : 0;
      res.m_State = plResourceState::Loaded;

      return res;
    }
  }
}

void plTexture2DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plTexture2DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plTexture2DResource, plTexture2DResourceDescriptor)
{
  plResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State = plResourceState::Loaded;

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  m_Type = descriptor.m_DescGAL.m_Type;
  m_Format = descriptor.m_DescGAL.m_Format;
  m_uiWidth = descriptor.m_DescGAL.m_uiWidth;
  m_uiHeight = descriptor.m_DescGAL.m_uiHeight;

  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descriptor.m_DescGAL, descriptor.m_InitialContent);
  PLASMA_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture Data could not be uploaded to the GPU");

  pDevice->GetTexture(m_hGALTexture[m_uiLoadedTextures])->SetDebugName(GetResourceDescription());

  if (!m_hSamplerState.IsInvalidated())
  {
    pDevice->DestroySamplerState(m_hSamplerState);
  }

  m_hSamplerState = pDevice->CreateSamplerState(descriptor.m_SamplerDesc);
  PLASMA_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// TODO (resources): move into separate file

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderToTexture2DResource, 1, plRTTIDefaultAllocator<plRenderToTexture2DResource>);
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, Texture2D)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    plResourceManager::RegisterResourceOverrideType(plGetStaticRTTI<plRenderToTexture2DResource>(), [](const plStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".plRenderTarget");
      });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plResourceManager::UnregisterResourceOverrideType(plGetStaticRTTI<plRenderToTexture2DResource>());
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plRenderToTexture2DResource);

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plRenderToTexture2DResource, plRenderToTexture2DResourceDescriptor)
{
  plResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = plResourceState::Loaded;

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  m_Type = plGALTextureType::Texture2D;
  m_Format = descriptor.m_Format;
  m_uiWidth = descriptor.m_uiWidth;
  m_uiHeight = descriptor.m_uiHeight;

  plGALTextureCreationDescription descGAL;
  descGAL.SetAsRenderTarget(m_uiWidth, m_uiHeight, m_Format, descriptor.m_SampleCount);

  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descGAL, descriptor.m_InitialContent);
  PLASMA_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture data could not be uploaded to the GPU");

  pDevice->GetTexture(m_hGALTexture[m_uiLoadedTextures])->SetDebugName(GetResourceDescription());

  if (!m_hSamplerState.IsInvalidated())
  {
    pDevice->DestroySamplerState(m_hSamplerState);
  }

  m_hSamplerState = pDevice->CreateSamplerState(descriptor.m_SamplerDesc);
  PLASMA_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

plResourceLoadDesc plRenderToTexture2DResource::UnloadData(Unload WhatToUnload)
{
  for (plInt32 r = 0; r < 2; ++r)
  {
    if (!m_hGALTexture[r].IsInvalidated())
    {
      plGALDevice::GetDefaultDevice()->DestroyTexture(m_hGALTexture[r]);
      m_hGALTexture[r].Invalidate();
    }

    m_uiMemoryGPU[r] = 0;
  }

  m_uiLoadedTextures = 0;

  if (!m_hSamplerState.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
    m_hSamplerState.Invalidate();
  }

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable = 2 - m_uiLoadedTextures;
  res.m_State = plResourceState::Unloaded;
  return res;
}

plGALRenderTargetViewHandle plRenderToTexture2DResource::GetRenderTargetView() const
{
  return plGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_hGALTexture[0]);
}

void plRenderToTexture2DResource::AddRenderView(plViewHandle hView)
{
  m_RenderViews.PushBack(hView);
}

void plRenderToTexture2DResource::RemoveRenderView(plViewHandle hView)
{
  m_RenderViews.RemoveAndSwap(hView);
}

const plDynamicArray<plViewHandle>& plRenderToTexture2DResource::GetAllRenderViews() const
{
  return m_RenderViews;
}

static plUInt16 GetNextBestResolution(float fRes)
{
  fRes = plMath::Clamp(fRes, 8.0f, 4096.0f);

  int mulEight = (int)plMath::Floor((fRes + 7.9f) / 8.0f);

  return static_cast<plUInt16>(mulEight * 8);
}

plResourceLoadDesc plRenderToTexture2DResource::UpdateContent(plStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    plResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = plResourceState::LoadedResourceMissing;

    return res;
  }

  plRenderToTexture2DResourceDescriptor td;
  plImage* pImage = nullptr;
  bool bIsFallback = false;
  plTexFormat texFormat;

  // load image data
  {
    Stream->ReadBytes(&pImage, sizeof(plImage*));
    *Stream >> bIsFallback;
    texFormat.ReadHeader(*Stream);

    td.m_SamplerDesc.m_AddressU = texFormat.m_AddressModeU;
    td.m_SamplerDesc.m_AddressV = texFormat.m_AddressModeV;
    td.m_SamplerDesc.m_AddressW = texFormat.m_AddressModeW;
  }

  const bool bIsRenderTarget = texFormat.m_iRenderTargetResolutionX != 0;

  PLASMA_ASSERT_DEV(bIsRenderTarget, "Trying to create a RenderToTexture resource from data that is not set up as a render-target");

  {
    PLASMA_ASSERT_DEV(m_uiLoadedTextures == 0, "not implemented");

    if (texFormat.m_iRenderTargetResolutionX == -1)
    {
      if (texFormat.m_iRenderTargetResolutionY == 1)
      {
        texFormat.m_iRenderTargetResolutionX = GetNextBestResolution(cvar_RenderingOffscreenTargetResolution1 * texFormat.m_fResolutionScale);
        texFormat.m_iRenderTargetResolutionY = texFormat.m_iRenderTargetResolutionX;
      }
      else if (texFormat.m_iRenderTargetResolutionY == 2)
      {
        texFormat.m_iRenderTargetResolutionX = GetNextBestResolution(cvar_RenderingOffscreenTargetResolution2 * texFormat.m_fResolutionScale);
        texFormat.m_iRenderTargetResolutionY = texFormat.m_iRenderTargetResolutionX;
      }
      else
      {
        PLASMA_REPORT_FAILURE(
          "Invalid render target configuration: {0} x {1}", texFormat.m_iRenderTargetResolutionX, texFormat.m_iRenderTargetResolutionY);
      }
    }

    td.m_Format = static_cast<plGALResourceFormat::Enum>(texFormat.m_GalRenderTargetFormat);
    td.m_uiWidth = texFormat.m_iRenderTargetResolutionX;
    td.m_uiHeight = texFormat.m_iRenderTargetResolutionY;

    plTextureUtils::ConfigureSampler(static_cast<plTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

    m_uiLoadedTextures = 0;

    CreateResource(std::move(td));
  }

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;
  return res;
}

void plRenderToTexture2DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plRenderToTexture2DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Textures_Texture2DResource);
