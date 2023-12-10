#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Threading/Mutex.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

struct plGALDeviceEvent;

/// \brief This class serves as a pool for GPU related resources (e.g. buffers and textures required for rendering).
/// Note that the functions creating and returning render targets are thread safe (by using a mutex).
class PLASMA_RENDERERCORE_DLL plGPUResourcePool
{
public:
  plGPUResourcePool();
  ~plGPUResourcePool();

  /// \brief Returns a render target handle for the given texture description
  /// Note that you should return the handle to the pool and never destroy it directly with the device.
  plGALTextureHandle GetRenderTarget(const plGALTextureCreationDescription& textureDesc);

  /// \brief Convenience functions which creates a texture description fit for a 2d render target without a mip chains.
  plGALTextureHandle GetRenderTarget(plUInt32 uiWidth, plUInt32 uiHeight, plGALResourceFormat::Enum format,
    plGALMSAASampleCount::Enum sampleCount = plGALMSAASampleCount::None, plUInt32 uiSliceColunt = 1);

  /// \brief Returns a render target to the pool so other consumers can use it.
  /// Note that targets which are returned to the pool are susceptible to destruction due to garbage collection.
  void ReturnRenderTarget(plGALTextureHandle hRenderTarget);


  /// \brief Returns a buffer handle for the given buffer description
  plGALBufferHandle GetBuffer(const plGALBufferCreationDescription& bufferDesc);

  /// \brief Returns a buffer to the pool so other consumers can use it.
  void ReturnBuffer(plGALBufferHandle hBuffer);


  /// \brief Tries to free resources which are currently in the pool.
  /// Triggered automatically due to allocation number / size thresholds but can be triggered manually (e.g. after editor window resize)
  ///
  /// \param uiMinimumAge How many frames at least the resource needs to have been unused before it will be GCed.
  void RunGC(plUInt32 uiMinimumAge);


  static plGPUResourcePool* GetDefaultInstance();
  static void SetDefaultInstance(plGPUResourcePool* pDefaultInstance);

protected:
  void CheckAndPotentiallyRunGC();
  void UpdateMemoryStats() const;
  void GALDeviceEventHandler(const plGALDeviceEvent& e);

  struct TextureHandleWithAge
  {
    plGALTextureHandle m_hTexture;
    plUInt64 m_uiLastUsed = 0;
  };

  struct BufferHandleWithAge
  {
    plGALBufferHandle m_hBuffer;
    plUInt64 m_uiLastUsed = 0;
  };

  plEventSubscriptionID m_GALDeviceEventSubscriptionID = 0;
  plUInt64 m_uiMemoryThresholdForGC = 256 * 1024 * 1024;
  plUInt64 m_uiCurrentlyAllocatedMemory = 0;
  plUInt16 m_uiNumAllocationsThresholdForGC = 128;
  plUInt16 m_uiNumAllocationsSinceLastGC = 0;
  plUInt16 m_uiFramesThresholdSinceLastGC = 60; ///< Every 60 frames resources unused for more than 10 frames in a row are GCed.
  plUInt16 m_uiFramesSinceLastGC = 0;

  plMap<plUInt32, plDynamicArray<TextureHandleWithAge>> m_AvailableTextures;
  plSet<plGALTextureHandle> m_TexturesInUse;

  plMap<plUInt32, plDynamicArray<BufferHandleWithAge>> m_AvailableBuffers;
  plSet<plGALBufferHandle> m_BuffersInUse;

  plMutex m_Lock;

  plGALDevice* m_pDevice;

private:
  static plGPUResourcePool* s_pDefaultInstance;
};
