
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class plGALRenderTargetViewVulkan : public plGALRenderTargetView
{
public:
  vk::ImageView GetImageView() const;
  bool IsFullRange() const;
  vk::ImageSubresourceRange GetRange() const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALRenderTargetViewVulkan(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& Description);
  virtual ~plGALRenderTargetViewVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  vk::ImageView m_imageView;
  bool m_bfullRange = false;
  vk::ImageSubresourceRange m_range;
};

#include <RendererVulkan/Resources/Implementation/RenderTargetViewVulkan_inl.h>
