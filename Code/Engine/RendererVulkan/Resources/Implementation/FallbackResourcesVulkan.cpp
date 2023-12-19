#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Resources/FallbackResourcesVulkan.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

plGALDeviceVulkan* plFallbackResourcesVulkan::s_pDevice = nullptr;
plEventSubscriptionID plFallbackResourcesVulkan::s_EventID = 0;

plHashTable<plFallbackResourcesVulkan::Key, plGALResourceViewHandle, plFallbackResourcesVulkan::KeyHash> plFallbackResourcesVulkan::m_ResourceViews;
plHashTable<plFallbackResourcesVulkan::Key, plGALUnorderedAccessViewHandle, plFallbackResourcesVulkan::KeyHash> plFallbackResourcesVulkan::m_UAVs;
plDynamicArray<plGALBufferHandle> plFallbackResourcesVulkan::m_Buffers;
plDynamicArray<plGALTextureHandle> plFallbackResourcesVulkan::m_Textures;

void plFallbackResourcesVulkan::Initialize(plGALDeviceVulkan* pDevice)
{
  s_pDevice = pDevice;
  s_EventID = pDevice->m_Events.AddEventHandler(plMakeDelegate(&plFallbackResourcesVulkan::GALDeviceEventHandler));
}

void plFallbackResourcesVulkan::DeInitialize()
{
  s_pDevice->m_Events.RemoveEventHandler(s_EventID);
  s_pDevice = nullptr;
}
void plFallbackResourcesVulkan::GALDeviceEventHandler(const plGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case plGALDeviceEvent::AfterInit:
    {
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
        PLASMA_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackResourceVulkan");
        m_Textures.PushBack(hTexture);
        return s_pDevice->GetDefaultResourceView(hTexture);
      };
      {
        plGALResourceViewHandle hView = CreateTexture(plGALTextureType::Texture2D, plGALMSAASampleCount::None, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, plShaderResourceType::Texture2D, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, plShaderResourceType::Texture2DArray, false}] = hView;
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
        PLASMA_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackResourceVulkan");
        m_Textures.PushBack(hTexture);
        plGALUnorderedAccessViewCreationDescription descUAV;
        descUAV.m_hTexture = hTexture;
        auto hUAV = s_pDevice->CreateUnorderedAccessView(descUAV);
        m_UAVs[{vk::DescriptorType::eStorageImage, plShaderResourceType::UAV, false}] = hUAV;
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
        PLASMA_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackResourceVulkan");
        m_Textures.PushBack(hTexture);
        plGALUnorderedAccessViewCreationDescription descUAV;
        descUAV.m_hTexture = hTexture;
        auto hUAV = s_pDevice->CreateUnorderedAccessView(descUAV);
        m_UAVs[{vk::DescriptorType::eStorageImage, plShaderResourceType::ConstantBuffer, false}] = hUAV;
      }
      {
        plGALResourceViewHandle hView = CreateTexture(plGALTextureType::Texture2D, plGALMSAASampleCount::None, true);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, plShaderResourceType::Texture2D, true}] = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, plShaderResourceType::Texture2DArray, true}] = hView;
      }

      // Swift shader can only do 4x MSAA. Add a check anyways.
      vk::ImageFormatProperties props;
      vk::Result res = s_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(vk::Format::eB8G8R8A8Srgb, vk::ImageType::e2D, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled, {}, &props);
      if (res == vk::Result::eSuccess && props.sampleCounts & vk::SampleCountFlagBits::e4)
      {
        plGALResourceViewHandle hView = CreateTexture(plGALTextureType::Texture2D, plGALMSAASampleCount::FourSamples, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, plShaderResourceType::Texture2DMS, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, plShaderResourceType::Texture2DMSArray, false}] = hView;
      }
      {
        plGALResourceViewHandle hView = CreateTexture(plGALTextureType::TextureCube, plGALMSAASampleCount::None, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, plShaderResourceType::TextureCube, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, plShaderResourceType::TextureCubeArray, false}] = hView;
      }
      {
        plGALResourceViewHandle hView = CreateTexture(plGALTextureType::Texture3D, plGALMSAASampleCount::None, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, plShaderResourceType::Texture3D, false}] = hView;
      }
      {
        plGALBufferCreationDescription desc;
        desc.m_bUseForIndirectArguments = false;
        desc.m_bUseAsStructuredBuffer = true;
        desc.m_bAllowRawViews = true;
        desc.m_bStreamOutputTarget = false;
        desc.m_bAllowShaderResourceView = true;
        desc.m_bAllowUAV = true;
        desc.m_uiStructSize = 128;
        desc.m_uiTotalSize = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        plGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackStructuredBufferVulkan");
        m_Buffers.PushBack(hBuffer);
        plGALResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_ResourceViews[{vk::DescriptorType::eUniformBuffer, plShaderResourceType::ConstantBuffer, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eUniformBuffer, plShaderResourceType::ConstantBuffer, true}] = hView;
        m_ResourceViews[{vk::DescriptorType::eStorageBuffer, plShaderResourceType::GenericBuffer, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eStorageBuffer, plShaderResourceType::GenericBuffer, true}] = hView;
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
        m_ResourceViews[{vk::DescriptorType::eUniformTexelBuffer, plShaderResourceType::GenericBuffer, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eUniformTexelBuffer, plShaderResourceType::GenericBuffer, true}] = hView;
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
    }
    break;
    default:
      break;
  }
}

const plGALResourceViewVulkan* plFallbackResourcesVulkan::GetFallbackResourceView(vk::DescriptorType descriptorType, plShaderResourceType::Enum plType, bool bDepth)
{
  if (plGALResourceViewHandle* pView = m_ResourceViews.GetValue(Key{descriptorType, plType, bDepth}))
  {
    return static_cast<const plGALResourceViewVulkan*>(s_pDevice->GetResourceView(*pView));
  }
  PLASMA_REPORT_FAILURE("No fallback resource set, update plFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

const plGALUnorderedAccessViewVulkan* plFallbackResourcesVulkan::GetFallbackUnorderedAccessView(vk::DescriptorType descriptorType, plShaderResourceType::Enum plType)
{
  if (plGALUnorderedAccessViewHandle* pView = m_UAVs.GetValue(Key{descriptorType, plType, false}))
  {
    return static_cast<const plGALUnorderedAccessViewVulkan*>(s_pDevice->GetUnorderedAccessView(*pView));
  }
  PLASMA_REPORT_FAILURE("No fallback resource set, update plFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

plUInt32 plFallbackResourcesVulkan::KeyHash::Hash(const Key& a)
{
  plHashStreamWriter32 writer;
  writer << plConversionUtilsVulkan::GetUnderlyingValue(a.m_descriptorType);
  writer << plConversionUtilsVulkan::GetUnderlyingValue(a.m_plType);
  writer << a.m_bDepth;
  return writer.GetHashValue();
}

bool plFallbackResourcesVulkan::KeyHash::Equal(const Key& a, const Key& b)
{
  return a.m_descriptorType == b.m_descriptorType && a.m_plType == b.m_plType && a.m_bDepth == b.m_bDepth;
}
