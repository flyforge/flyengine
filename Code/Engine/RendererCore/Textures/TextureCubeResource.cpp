#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/plTexFormat/plTexFormat.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureCubeResource, 1, plRTTIDefaultAllocator<plTextureCubeResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plTextureCubeResource);
// clang-format on

plTextureCubeResource::plTextureCubeResource()
  : plResource(DoUpdate::OnAnyThread, plTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
  m_uiLoadedTextures = 0;
  m_uiMemoryGPU[0] = 0;
  m_uiMemoryGPU[1] = 0;
  m_Format = plGALResourceFormat::Invalid;
  m_uiWidthAndHeight = 0;
}

plResourceLoadDesc plTextureCubeResource::UnloadData(Unload WhatToUnload)
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

plResourceLoadDesc plTextureCubeResource::UpdateContent(plStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    plResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = plResourceState::LoadedResourceMissing;

    return res;
  }

  plImage* pImage = nullptr;
  Stream->ReadBytes(&pImage, sizeof(plImage*));

  bool bIsFallback = false;
  *Stream >> bIsFallback;

  plTexFormat texFormat;
  texFormat.ReadHeader(*Stream);

  const plUInt32 uiNumMipmapsLowRes = plTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : 6;

  const plUInt32 uiNumMipLevels = plMath::Min(m_uiLoadedTextures == 0 ? uiNumMipmapsLowRes : pImage->GetNumMipLevels(), pImage->GetNumMipLevels());
  const plUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  if (pImage->GetWidth(uiHighestMipLevel) != pImage->GetHeight(uiHighestMipLevel))
  {
    plLog::Error("Cubemap width '{0}' is not identical to height '{1}'", pImage->GetWidth(uiHighestMipLevel), pImage->GetHeight(uiHighestMipLevel));

    plResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = plResourceState::LoadedResourceMissing;

    return res;
  }

  m_Format = plTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), texFormat.m_bSRGB);
  m_uiWidthAndHeight = pImage->GetWidth(uiHighestMipLevel);

  plGALTextureCreationDescription texDesc;
  texDesc.m_Format = m_Format;
  texDesc.m_uiWidth = m_uiWidthAndHeight;
  texDesc.m_uiHeight = m_uiWidthAndHeight;
  texDesc.m_uiDepth = pImage->GetDepth(uiHighestMipLevel);
  texDesc.m_uiMipLevelCount = uiNumMipLevels;
  texDesc.m_uiArraySize = pImage->GetNumArrayIndices();

  if (texDesc.m_uiDepth > 1)
    texDesc.m_Type = plGALTextureType::Texture3D;

  if (pImage->GetNumFaces() == 6)
    texDesc.m_Type = plGALTextureType::TextureCube;

  PLASMA_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces (resource: '{0}')", GetResourceID());

  m_uiMemoryGPU[m_uiLoadedTextures] = 0;

  plHybridArray<plGALSystemMemoryDescription, 32> InitData;

  for (plUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (plUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (plUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        plGALSystemMemoryDescription& id = InitData.ExpandAndGetRef();

        id.m_pData = pImage->GetPixelPointer<plUInt8>(mip, face, array_index);

        PLASMA_ASSERT_DEV(pImage->GetDepthPitch(mip) < plMath::MaxValue<plUInt32>(), "Depth pitch exceeds plGAL limits.");

        if (plImageFormat::GetType(pImage->GetImageFormat()) == plImageFormatType::BLOCK_COMPRESSED)
        {
          const plUInt32 uiMemPitchFactor = plGALResourceFormat::GetBitsPerElement(m_Format) * 4 / 8;

          id.m_uiRowPitch = plMath::Max<plUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiRowPitch = static_cast<plUInt32>(pImage->GetRowPitch(mip));
        }

        id.m_uiSlicePitch = static_cast<plUInt32>(pImage->GetDepthPitch(mip));

        m_uiMemoryGPU[m_uiLoadedTextures] += id.m_uiSlicePitch;
      }
    }
  }

  const plArrayPtr<plGALSystemMemoryDescription> InitDataPtr(InitData);

  plTextureCubeResourceDescriptor td;
  td.m_DescGAL = texDesc;
  td.m_SamplerDesc.m_AddressU = texFormat.m_AddressModeU;
  td.m_SamplerDesc.m_AddressV = texFormat.m_AddressModeV;
  td.m_SamplerDesc.m_AddressW = texFormat.m_AddressModeW;
  td.m_InitialContent = InitDataPtr;

  plTextureUtils::ConfigureSampler(static_cast<plTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

  // ignore its return value here, we build our own
  CreateResource(std::move(td));

  {
    plResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;

    if (uiHighestMipLevel == 0)
      res.m_uiQualityLevelsLoadable = 0;
    else
      res.m_uiQualityLevelsLoadable = 1;

    res.m_State = plResourceState::Loaded;

    return res;
  }
}

void plTextureCubeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plTextureCubeResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plTextureCubeResource, plTextureCubeResourceDescriptor)
{
  plResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State = plResourceState::Loaded;

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  PLASMA_ASSERT_DEV(descriptor.m_DescGAL.m_uiWidth == descriptor.m_DescGAL.m_uiHeight, "Cubemap width and height must be identical");

  m_Format = descriptor.m_DescGAL.m_Format;
  m_uiWidthAndHeight = descriptor.m_DescGAL.m_uiWidth;

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



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureCubeResource);
