#pragma once

#include <RendererFoundation/Resources/Texture.h>

#include <vulkan/vulkan.hpp>

class plGALBufferVulkan;
class plGALDeviceVulkan;

class plGALTextureVulkan : public plGALTexture
{
public:
  enum class StagingMode : plUInt8
  {
    None,
    Buffer,          ///< We can use vkCopyImageToBuffer to a CPU buffer.
    Texture,         ///< Formats differ and we need to render to a linear CPU texture to do the conversion.
    TextureAndBuffer ///< Formats differ and linear texture can't be rendered to. Render to optimal layout GPU texture and then use vkCopyImageToBuffer to CPU buffer.
  };
  struct SubResourceOffset
  {
    PLASMA_DECLARE_POD_TYPE();
    plUInt32 m_uiOffset;
    plUInt32 m_uiSize;
    plUInt32 m_uiRowLength;
    plUInt32 m_uiImageHeight;
  };

  PLASMA_ALWAYS_INLINE vk::Image GetImage() const;
  PLASMA_ALWAYS_INLINE vk::Format GetImageFormat() const { return m_imageFormat; }
  PLASMA_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout() const;
  PLASMA_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout(vk::ImageLayout targetLayout) const;
  PLASMA_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  PLASMA_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;

  PLASMA_ALWAYS_INLINE plVulkanAllocation GetAllocation() const;
  PLASMA_ALWAYS_INLINE const plVulkanAllocationInfo& GetAllocationInfo() const;

  PLASMA_ALWAYS_INLINE bool GetFormatOverrideEnabled() const;
  PLASMA_ALWAYS_INLINE bool IsLinearLayout() const;

  vk::Extent3D GetMipLevelSize(plUInt32 uiMipLevel) const;
  vk::ImageSubresourceRange GetFullRange() const;
  vk::ImageAspectFlags GetAspectMask() const;

  // Read-back staging resources
  PLASMA_ALWAYS_INLINE StagingMode GetStagingMode() const;
  PLASMA_ALWAYS_INLINE plGALTextureHandle GetStagingTexture() const;
  PLASMA_ALWAYS_INLINE plGALBufferHandle GetStagingBuffer() const;
  plUInt32 ComputeSubResourceOffsets(plDynamicArray<SubResourceOffset>& out_subResourceOffsets) const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALTextureVulkan(const plGALTextureCreationDescription& Description);
  plGALTextureVulkan(const plGALTextureCreationDescription& Description, vk::Format OverrideFormat, bool bLinearCPU);

  ~plGALTextureVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  StagingMode ComputeStagingMode(const vk::ImageCreateInfo& createInfo) const;
  plResult CreateStagingBuffer(const vk::ImageCreateInfo& createInfo);

  vk::Image m_image;
  vk::Format m_imageFormat = vk::Format::eUndefined;
  vk::ImageLayout m_preferredLayout = vk::ImageLayout::eUndefined;
  vk::PipelineStageFlags m_stages = {};
  vk::AccessFlags m_access = {};

  plVulkanAllocation m_alloc = nullptr;
  plVulkanAllocationInfo m_allocInfo;

  plGALDeviceVulkan* m_pDevice = nullptr;
  void* m_pExisitingNativeObject = nullptr;

  bool m_formatOverride = false;
  bool m_bLinearCPU = false;

  StagingMode m_stagingMode = StagingMode::None;
  plGALTextureHandle m_hStagingTexture;
  plGALBufferHandle m_hStagingBuffer;
};

#include <RendererVulkan/Resources/Implementation/TextureVulkan_inl.h>
