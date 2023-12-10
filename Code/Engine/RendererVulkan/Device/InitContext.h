
#pragma once

#include <Foundation/Types/UniquePtr.h>

class plGALDeviceVulkan;
class plPipelineBarrierVulkan;
class plCommandBufferPoolVulkan;
class plStagingBufferPoolVulkan;

/// \brief Thread-safe context for initializing resources. Records a command buffer that transitions all newly created resources into their initial state.
class plInitContextVulkan
{
public:
  plInitContextVulkan(plGALDeviceVulkan* pDevice);
  ~plInitContextVulkan();

  /// \brief Returns a finished command buffer of all background loading up to this point.
  ///    The command buffer is already ended and marked to be reclaimed so the only thing done on it should be to submit it.
  vk::CommandBuffer GetFinishedCommandBuffer();

  /// \brief Initializes a texture and moves it into its default state.
  /// \param pTexture The texture to initialize.
  /// \param createInfo The image creation info for the texture. Needed for initial state information.
  /// \param pInitialData The initial data of the texture. If not set, the initial content will be undefined.
  void InitTexture(const plGALTextureVulkan* pTexture, vk::ImageCreateInfo& createInfo, plArrayPtr<plGALSystemMemoryDescription> pInitialData);

  /// \brief Needs to be called by the plGALDeviceVulkan just before a texture is destroyed to clean up stale barriers.
  void TextureDestroyed(const plGALTextureVulkan* pTexture);

private:
  void EnsureCommandBufferExists();

  plGALDeviceVulkan* m_pDevice = nullptr;

  plMutex m_Lock;
  vk::CommandBuffer m_currentCommandBuffer;
  plUniquePtr<plPipelineBarrierVulkan> m_pPipelineBarrier;
  plUniquePtr<plCommandBufferPoolVulkan> m_pCommandBufferPool;
  plUniquePtr<plStagingBufferPoolVulkan> m_pStagingBufferPool;
};
