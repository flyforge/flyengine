
#pragma once

#include <RendererFoundation/State/State.h>

#include <vulkan/vulkan.hpp>

class PLASMA_RENDERERVULKAN_DLL plGALBlendStateVulkan : public plGALBlendState
{
public:
  PLASMA_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* GetBlendState() const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALBlendStateVulkan(const plGALBlendStateCreationDescription& Description);

  ~plGALBlendStateVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  vk::PipelineColorBlendStateCreateInfo m_blendState = {};
  vk::PipelineColorBlendAttachmentState m_blendAttachmentState[8] = {};
};

class PLASMA_RENDERERVULKAN_DLL plGALDepthStencilStateVulkan : public plGALDepthStencilState
{
public:
  PLASMA_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* GetDepthStencilState() const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALDepthStencilStateVulkan(const plGALDepthStencilStateCreationDescription& Description);

  ~plGALDepthStencilStateVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  vk::PipelineDepthStencilStateCreateInfo m_depthStencilState = {};
};

class PLASMA_RENDERERVULKAN_DLL plGALRasterizerStateVulkan : public plGALRasterizerState
{
public:
  PLASMA_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* GetRasterizerState() const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALRasterizerStateVulkan(const plGALRasterizerStateCreationDescription& Description);

  ~plGALRasterizerStateVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  vk::PipelineRasterizationStateCreateInfo m_rasterizerState = {};
};

class PLASMA_RENDERERVULKAN_DLL plGALSamplerStateVulkan : public plGALSamplerState
{
public:
  PLASMA_ALWAYS_INLINE const vk::DescriptorImageInfo& GetImageInfo() const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALSamplerStateVulkan(const plGALSamplerStateCreationDescription& Description);
  ~plGALSamplerStateVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  vk::DescriptorImageInfo m_resourceImageInfo;
};


#include <RendererVulkan/State/Implementation/StateVulkan_inl.h>
