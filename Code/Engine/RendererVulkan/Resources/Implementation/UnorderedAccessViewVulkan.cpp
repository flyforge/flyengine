#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

bool IsArrayView(const plGALTextureCreationDescription& texDesc, const plGALUnorderedAccessViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

const vk::DescriptorBufferInfo& plGALUnorderedAccessViewVulkan::GetBufferInfo() const
{
  // Vulkan buffers get constantly swapped out for new ones so the vk::Buffer pointer is not persistent.
  // We need to acquire the latest one on every request for rendering.
  m_resourceBufferInfo.buffer = static_cast<const plGALBufferVulkan*>(GetResource())->GetVkBuffer();
  return m_resourceBufferInfo;
}

plGALUnorderedAccessViewVulkan::plGALUnorderedAccessViewVulkan(
  plGALResourceBase* pResource, const plGALUnorderedAccessViewCreationDescription& Description)
  : plGALUnorderedAccessView(pResource, Description)
{
}

plGALUnorderedAccessViewVulkan::~plGALUnorderedAccessViewVulkan() {}

plResult plGALUnorderedAccessViewVulkan::InitPlatform(plGALDevice* pDevice)
{
  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);

  const plGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  const plGALBuffer* pBuffer = nullptr;
  if (!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);

  if (pTexture == nullptr && pBuffer == nullptr)
  {
    plLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return PLASMA_FAILURE;
  }

  if (pTexture)
  {
    auto pParentTexture = static_cast<const plGALTextureVulkan*>(pTexture->GetParentResource());
    auto image = pParentTexture->GetImage();
    const plGALTextureCreationDescription& texDesc = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);

    plGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat == plGALResourceFormat::Invalid ? texDesc.m_Format : m_Description.m_OverrideViewFormat;
    vk::ImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
    viewCreateInfo.image = image;
    viewCreateInfo.subresourceRange = plConversionUtilsVulkan::GetSubresourceRange(texDesc, m_Description);
    viewCreateInfo.viewType = plConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, bIsArrayView);
    if (texDesc.m_Type == plGALTextureType::TextureCube)
      viewCreateInfo.viewType = vk::ImageViewType::e2DArray; // There is no RWTextureCube / RWTextureCubeArray in HLSL

    m_resourceImageInfo.imageLayout = vk::ImageLayout::eGeneral;

    m_range = viewCreateInfo.subresourceRange;
    VK_SUCCEED_OR_RETURN_PLASMA_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
    pVulkanDevice->SetDebugName("UAV-Texture", m_resourceImageInfo.imageView);
  }
  else if (pBuffer)
  {
    if (!pBuffer->GetDescription().m_bAllowRawViews && m_Description.m_bRawView)
    {
      plLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
      return PLASMA_FAILURE;
    }

    auto pParentBuffer = static_cast<const plGALBufferVulkan*>(pBuffer);
    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
    {
      m_resourceBufferInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      m_resourceBufferInfo.range = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
    }
    else if (m_Description.m_bRawView)
    {
      m_resourceBufferInfo.offset = sizeof(plUInt32) * m_Description.m_uiFirstElement;
      m_resourceBufferInfo.range = sizeof(plUInt32) * m_Description.m_uiNumElements;
    }
    else
    {
      m_resourceBufferInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      m_resourceBufferInfo.range = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;

      plGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat;
      if (viewFormat == plGALResourceFormat::Invalid)
        viewFormat = plGALResourceFormat::RUInt;

      vk::BufferViewCreateInfo viewCreateInfo;
      viewCreateInfo.buffer = pParentBuffer->GetVkBuffer();
      viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
      viewCreateInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      viewCreateInfo.range = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;

      VK_SUCCEED_OR_RETURN_PLASMA_FAILURE(pVulkanDevice->GetVulkanDevice().createBufferView(&viewCreateInfo, nullptr, &m_bufferView));
    }
  }

  return PLASMA_SUCCESS;
}

plResult plGALUnorderedAccessViewVulkan::DeInitPlatform(plGALDevice* pDevice)
{
  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_resourceImageInfo.imageView);
  m_resourceImageInfo = vk::DescriptorImageInfo();
  m_resourceBufferInfo = vk::DescriptorBufferInfo();
  pVulkanDevice->DeleteLater(m_bufferView);
  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_UnorderedAccessViewVulkan);
