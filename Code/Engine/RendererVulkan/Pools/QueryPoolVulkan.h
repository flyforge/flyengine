#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class plGALDeviceVulkan;

/// \brief Pool for GPU queries.
class PL_RENDERERVULKAN_DLL plQueryPoolVulkan
{
public:
  /// \brief Initializes the pool.
  /// \param pDevice Parent Vulkan device.
  /// \param uiValidBits The number of valid bits in the query result. Each queue has different query characteristics and a separate pool is needed for each queue.
  void Initialize(plGALDeviceVulkan* pDevice, plUInt32 uiValidBits);
  void DeInitialize();

  /// \brief Needs to be called every frame so the pool can figure out which queries have finished and reuse old data.
  void BeginFrame(vk::CommandBuffer commandBuffer);

  /// \brief Create a new timestamp. This needs to be consumed in the same frame it was acquired using InsertTimestamp.
  plGALTimestampHandle GetTimestamp();

  /// \brief Inserts a timestamp into the given command buffer.
  /// \param commandBuffer Target command buffer to insert the timestamp into.
  /// \param hTimestamp Timestamp to insert. After insertion the only valid option is to call GetTimestampResult.
  /// \param pipelineStage The value of the timestamp will be the point in time in which all previously committed commands have finished this stage.
  void InsertTimestamp(vk::CommandBuffer commandBuffer, plGALTimestampHandle hTimestamp, vk::PipelineStageFlagBits pipelineStage = vk::PipelineStageFlagBits::eBottomOfPipe);

  /// \brief Retrieves the timestamp value if it is available.
  /// \param hTimestamp The target timestamp to resolve.
  /// \param result The time of the timestamp. If this is empty on success the timestamp has expired.
  /// \param bForce Wait for the timestamp to become available.
  /// \return Returns false if the result is not available yet.
  plResult GetTimestampResult(plGALTimestampHandle hTimestamp, plTime& result, bool bForce = false);

private:
  /// \brief GPU and CPU timestamps have no relation in Vulkan. To establish it we need to measure the same timestamp in both and compute the difference.
  void Calibrate();

  static constexpr plUInt32 s_uiRetainFrames = 4;
  static constexpr plUInt32 s_uiPoolSize = 256;

  struct TimestampPool
  {
    vk::QueryPool m_pool;
    bool m_bReady = false;
    plDynamicArray<uint64_t> m_queryResults;
  };

  struct FramePool
  {
    plUInt64 m_uiNextIndex = 0;
    plUInt64 m_uiFrameCounter = 0;
    plHybridArray<TimestampPool*, 2> m_pools;
  };

  TimestampPool* GetFreePool();

  plGALDeviceVulkan* m_pDevice = nullptr;
  vk::Device m_device;

  // Conversion and calibration data.
  double m_fNanoSecondsPerTick = 0;
  plUInt64 m_uiValidBitsMask = 0;
  plTime m_gpuToCpuDelta;

  // Current active data.
  FramePool* m_pCurrentFrame = nullptr;
  plUInt64 m_uiFirstFrameIndex = 0;
  plDeque<FramePool> m_pendingFrames;

  // Pools.
  plHybridArray<vk::QueryPool, 8> m_resetPools;
  plHybridArray<TimestampPool*, 8> m_freePools;
};
