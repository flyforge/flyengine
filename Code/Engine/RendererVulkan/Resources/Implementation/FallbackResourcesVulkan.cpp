#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Resources/FallbackResourcesVulkan.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <Foundation/Configuration/Startup.h>

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(RendererVulkan, FallbackResourcesVulkan)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plFallbackResourcesVulkan::Initialize();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plFallbackResourcesVulkan::DeInitialize();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plGALDevice* plFallbackResourcesVulkan::s_pDevice = nullptr;
plEventSubscriptionID plFallbackResourcesVulkan::s_EventID = 0;

plHashTable<plFallbackResourcesVulkan::Key, plGALResourceViewHandle, plFallbackResourcesVulkan::KeyHash> plFallbackResourcesVulkan::m_ResourceViews;
plHashTable<plFallbackResourcesVulkan::Key, plGALUnorderedAccessViewHandle, plFallbackResourcesVulkan::KeyHash> plFallbackResourcesVulkan::m_UAVs;
plDynamicArray<plGALBufferHandle> plFallbackResourcesVulkan::m_Buffers;
plDynamicArray<plGALTextureHandle> plFallbackResourcesVulkan::m_Textures;

void plFallbackResourcesVulkan::Initialize()
{
  s_EventID = plGALDevice::s_Events.AddEventHandler(plMakeDelegate(&plFallbackResourcesVulkan::GALDeviceEventHandler));
}

void plFallbackResourcesVulkan::DeInitialize()
{
  plGALDevice::s_Events.RemoveEventHandler(s_EventID);
}
void plFallbackResourcesVulkan::GALDeviceEventHandler(const plGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case plGALDeviceEvent::AfterInit:
    {
      s_pDevice = e.m_pDevice;
      auto CreateTexture = [](plGALTextureType::Enum type, plGALMSAASampleCount::Enum samples, bool bDepth) -> plGALResourceViewHandle {
        plGALTextureCreationDescription desc;
        desc.m_uiWidth = 4;
        desc.m_uiHeight = 4;
        if (type == plGALTextureType::Texture3D)
          desc.m_uiDepth = 4;
        desc.m_uiMipLevelCount = 1;
        desc.m_Format = bDepth ? plGALResourceFormat::D16 : plGALResourceFormat::BGRAUByteNormalizedsRGB;
        desc.m_Type = type;
        desc.m_SampleCount = samples;
        desc.m_ResourceAccess.m_bImmutable = false;
        desc.m_bCreateRenderTarget = bDepth;
        plGALTextureHandle hTexture = s_pDevice->CreateTexture(desc);
        PL_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackResourceVulkan");
        m_Textures.PushBack(hTexture);
        return s_pDevice->GetDefaultResourceView(hTexture);
      };
      {
        plGALResourceViewHandle hView = CreateTexture(plGALTextureType::Texture2D, plGALMSAASampleCount::None, false);
        m_ResourceViews[{plGALShaderResourceType::Texture, plGALShaderTextureType::Texture2D, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::Texture, plGALShaderTextureType::Texture2DArray, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::TextureAndSampler, plGALShaderTextureType::Texture2D, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::TextureAndSampler, plGALShaderTextureType::Texture2DArray, false}] = hView;
      }
      {
        plGALResourceViewHandle hView = CreateTexture(plGALTextureType::Texture2D, plGALMSAASampleCount::None, true);
        m_ResourceViews[{plGALShaderResourceType::Texture, plGALShaderTextureType::Texture2D, true}] = hView;
        m_ResourceViews[{plGALShaderResourceType::Texture, plGALShaderTextureType::Texture2DArray, true}] = hView;
        m_ResourceViews[{plGALShaderResourceType::TextureAndSampler, plGALShaderTextureType::Texture2D, true}] = hView;
        m_ResourceViews[{plGALShaderResourceType::TextureAndSampler, plGALShaderTextureType::Texture2DArray, true}] = hView;
      }

      // Swift shader can only do 4x MSAA. Add a check anyways.
      const bool bSupported = s_pDevice->GetCapabilities().m_FormatSupport[plGALResourceFormat::BGRAUByteNormalizedsRGB].AreAllSet(plGALResourceFormatSupport::Sample | plGALResourceFormatSupport::MSAA4x);

      if (bSupported)
      {
        plGALResourceViewHandle hView = CreateTexture(plGALTextureType::Texture2D, plGALMSAASampleCount::FourSamples, false);
        m_ResourceViews[{plGALShaderResourceType::Texture, plGALShaderTextureType::Texture2DMS, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::Texture, plGALShaderTextureType::Texture2DMSArray, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::TextureAndSampler, plGALShaderTextureType::Texture2DMS, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::TextureAndSampler, plGALShaderTextureType::Texture2DMSArray, false}] = hView;
      }
      {
        plGALResourceViewHandle hView = CreateTexture(plGALTextureType::TextureCube, plGALMSAASampleCount::None, false);
        m_ResourceViews[{plGALShaderResourceType::Texture, plGALShaderTextureType::TextureCube, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::Texture, plGALShaderTextureType::TextureCubeArray, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::TextureAndSampler, plGALShaderTextureType::TextureCube, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::TextureAndSampler, plGALShaderTextureType::TextureCubeArray, false}] = hView;
      }
      {
        plGALResourceViewHandle hView = CreateTexture(plGALTextureType::Texture3D, plGALMSAASampleCount::None, false);
        m_ResourceViews[{plGALShaderResourceType::Texture, plGALShaderTextureType::Texture3D, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::TextureAndSampler, plGALShaderTextureType::Texture3D, false}] = hView;
      }
      {
        plGALBufferCreationDescription desc;
        desc.m_bUseForIndirectArguments = false;
        desc.m_bUseAsStructuredBuffer = true;
        desc.m_bAllowRawViews = true;
        desc.m_bAllowShaderResourceView = true;
        desc.m_bAllowUAV = true;
        desc.m_uiStructSize = 128;
        desc.m_uiTotalSize = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        plGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackStructuredBufferVulkan");
        m_Buffers.PushBack(hBuffer);
        plGALResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_ResourceViews[{plGALShaderResourceType::ConstantBuffer, plGALShaderTextureType::Unknown, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::ConstantBuffer, plGALShaderTextureType::Unknown, true}] = hView;
        m_ResourceViews[{plGALShaderResourceType::StructuredBuffer, plGALShaderTextureType::Unknown, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::StructuredBuffer, plGALShaderTextureType::Unknown, true}] = hView;
      }
      {
        plGALBufferCreationDescription desc;
        desc.m_uiStructSize = sizeof(plUInt32);
        desc.m_uiTotalSize = 1024;
        desc.m_bAllowShaderResourceView = true;
        desc.m_ResourceAccess.m_bImmutable = false;
        plGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBufferVulkan");
        m_Buffers.PushBack(hBuffer);
        plGALResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_ResourceViews[{plGALShaderResourceType::TexelBuffer, plGALShaderTextureType::Unknown, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::TexelBuffer, plGALShaderTextureType::Unknown, true}] = hView;
      }
      {
        plGALTextureCreationDescription desc;
        desc.m_uiWidth = 4;
        desc.m_uiHeight = 4;
        desc.m_uiMipLevelCount = 1;
        desc.m_Format = plGALResourceFormat::RGBAHalf;
        desc.m_Type = plGALTextureType::Texture2D;
        desc.m_SampleCount = plGALMSAASampleCount::None;
        desc.m_ResourceAccess.m_bImmutable = false;
        desc.m_bCreateRenderTarget = false;
        desc.m_bAllowUAV = true;
        plGALTextureHandle hTexture = s_pDevice->CreateTexture(desc);
        PL_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackTextureRWVulkan");
        m_Textures.PushBack(hTexture);

        plGALUnorderedAccessViewCreationDescription descUAV;
        descUAV.m_hTexture = hTexture;
        auto hUAV = s_pDevice->CreateUnorderedAccessView(descUAV);
        m_UAVs[{plGALShaderResourceType::TextureRW, plGALShaderTextureType::Unknown, false}] = hUAV;
      }
      {
        plGALBufferCreationDescription desc;
        desc.m_uiStructSize = sizeof(plUInt32);
        desc.m_uiTotalSize = 1024;
        desc.m_bAllowShaderResourceView = true;
        desc.m_bAllowUAV = true;
        desc.m_ResourceAccess.m_bImmutable = false;
        plGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBufferRWVulkan");
        m_Buffers.PushBack(hBuffer);
        plGALResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_ResourceViews[{plGALShaderResourceType::TexelBufferRW, plGALShaderTextureType::Unknown, false}] = hView;
        m_ResourceViews[{plGALShaderResourceType::TexelBufferRW, plGALShaderTextureType::Unknown, true}] = hView;
      }
      {
        plGALBufferCreationDescription desc;
        desc.m_bUseForIndirectArguments = false;
        desc.m_bUseAsStructuredBuffer = true;
        desc.m_bAllowRawViews = true;
        desc.m_bAllowShaderResourceView = true;
        desc.m_bAllowUAV = true;
        desc.m_uiStructSize = 128;
        desc.m_uiTotalSize = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        plGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackStructuredBufferRWVulkan");
        m_Buffers.PushBack(hBuffer);
        plGALResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_ResourceViews[{plGALShaderResourceType::StructuredBufferRW, plGALShaderTextureType::Unknown, false}] = hView;
      }
    }
    break;
    case plGALDeviceEvent::BeforeShutdown:
    {
      m_ResourceViews.Clear();
      m_ResourceViews.Compact();

      m_UAVs.Clear();
      m_UAVs.Compact();

      for (plGALBufferHandle hBuffer : m_Buffers)
      {
        s_pDevice->DestroyBuffer(hBuffer);
      }
      m_Buffers.Clear();
      m_Buffers.Compact();

      for (plGALTextureHandle hTexture : m_Textures)
      {
        s_pDevice->DestroyTexture(hTexture);
      }
      m_Textures.Clear();
      m_Textures.Compact();
      s_pDevice = nullptr;
    }
    break;
    default:
      break;
  }
}

const plGALResourceViewVulkan* plFallbackResourcesVulkan::GetFallbackResourceView(plGALShaderResourceType::Enum descriptorType, plGALShaderTextureType::Enum textureType, bool bDepth)
{
  if (plGALResourceViewHandle* pView = m_ResourceViews.GetValue(Key{descriptorType, textureType, bDepth}))
  {
    return static_cast<const plGALResourceViewVulkan*>(s_pDevice->GetResourceView(*pView));
  }
  PL_REPORT_FAILURE("No fallback resource set, update plFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

const plGALUnorderedAccessViewVulkan* plFallbackResourcesVulkan::GetFallbackUnorderedAccessView(plGALShaderResourceType::Enum descriptorType, plGALShaderTextureType::Enum textureType)
{
  if (plGALUnorderedAccessViewHandle* pView = m_UAVs.GetValue(Key{descriptorType, textureType, false}))
  {
    return static_cast<const plGALUnorderedAccessViewVulkan*>(s_pDevice->GetUnorderedAccessView(*pView));
  }
  PL_REPORT_FAILURE("No fallback resource set, update plFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

plUInt32 plFallbackResourcesVulkan::KeyHash::Hash(const Key& a)
{
  plHashStreamWriter32 writer;
  writer << a.m_ResourceType.GetValue();
  writer << a.m_plType.GetValue();
  writer << a.m_bDepth;
  return writer.GetHashValue();
}

bool plFallbackResourcesVulkan::KeyHash::Equal(const Key& a, const Key& b)
{
  return a.m_ResourceType == b.m_ResourceType && a.m_plType == b.m_plType && a.m_bDepth == b.m_bDepth;
}
