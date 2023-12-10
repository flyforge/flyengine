#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

bool IsArrayView(const plGALTextureCreationDescription& texDesc, const plGALRenderTargetViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstSlice > 0;
}

plGALRenderTargetViewVulkan::plGALRenderTargetViewVulkan(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& Description)
  : plGALRenderTargetView(pTexture, Description)
{
}

plGALRenderTargetViewVulkan::~plGALRenderTargetViewVulkan() {}

plResult plGALRenderTargetViewVulkan::InitPlatform(plGALDevice* pDevice)
{
  const plGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    plLog::Error("No valid texture handle given for render target view creation!");
    return PLASMA_FAILURE;
  }

  const plGALTextureCreationDescription& texDesc = pTexture->GetDescription();
  plGALResourceFormat::Enum viewFormat = texDesc.m_Format;

  if (m_Description.m_OverrideViewFormat != plGALResourceFormat::Invalid)
    viewFormat = m_Description.m_OverrideViewFormat;

  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);
  auto pTextureVulkan = static_cast<const plGALTextureVulkan*>(pTexture->GetParentResource());
  vk::Format vkViewFormat = pTextureVulkan->GetImageFormat();

  const bool bIsDepthFormat = plConversionUtilsVulkan::IsDepthFormat(vkViewFormat);

  if (vkViewFormat == vk::Format::eUndefined)
  {
    plLog::Error("Couldn't get Vulkan format for view!");
    return PLASMA_FAILURE;
  }


  vk::Image vkImage = pTextureVulkan->GetImage();
  const bool bIsArrayView = IsArrayView(texDesc, m_Description);

  vk::ImageViewCreateInfo imageViewCreationInfo;
  if (bIsDepthFormat)
  {
    imageViewCreationInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    if (texDesc.m_Format == plGALResourceFormat::D24S8)
    {
      imageViewCreationInfo.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
  }
  else
  {
    imageViewCreationInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  imageViewCreationInfo.image = vkImage;
  imageViewCreationInfo.format = vkViewFormat;

  if (!bIsArrayView)
  {
    imageViewCreationInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreationInfo.subresourceRange.baseMipLevel = m_Description.m_uiMipLevel;
    imageViewCreationInfo.subresourceRange.levelCount = 1;
    imageViewCreationInfo.subresourceRange.layerCount = 1;
  }
  else
  {
    imageViewCreationInfo.viewType = vk::ImageViewType::e2DArray;
    imageViewCreationInfo.subresourceRange.baseMipLevel = m_Description.m_uiMipLevel;
    imageViewCreationInfo.subresourceRange.levelCount = 1;
    imageViewCreationInfo.subresourceRange.baseArrayLayer = m_Description.m_uiFirstSlice;
    imageViewCreationInfo.subresourceRange.layerCount = m_Description.m_uiSliceCount;
  }
  m_range = imageViewCreationInfo.subresourceRange;
  m_bfullRange = m_range == pTextureVulkan->GetFullRange();

  VK_SUCCEED_OR_RETURN_PLASMA_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&imageViewCreationInfo, nullptr, &m_imageView));
  pVulkanDevice->SetDebugName("RTV", m_imageView);
  return PLASMA_SUCCESS;
}

plResult plGALRenderTargetViewVulkan::DeInitPlatform(plGALDevice* pDevice)
{
  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_imageView);
  return PLASMA_SUCCESS;
}



PLASMA_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_RenderTargetViewVulkan);
