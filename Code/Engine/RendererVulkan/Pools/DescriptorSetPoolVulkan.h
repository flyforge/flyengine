#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

PLASMA_DEFINE_AS_POD_TYPE(vk::DescriptorType);

template <>
struct plHashHelper<vk::DescriptorType>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(vk::DescriptorType value) { return plHashHelper<plUInt32>::Hash(plUInt32(value)); }
  PLASMA_ALWAYS_INLINE static bool Equal(vk::DescriptorType a, vk::DescriptorType b) { return a == b; }
};

class PLASMA_RENDERERVULKAN_DLL plDescriptorSetPoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();
  static plHashTable<vk::DescriptorType, float>& AccessDescriptorPoolWeights();

  static vk::DescriptorSet CreateDescriptorSet(vk::DescriptorSetLayout layout);
  static void UpdateDescriptorSet(vk::DescriptorSet descriptorSet, plArrayPtr<vk::WriteDescriptorSet> update);
  static void ReclaimPool(vk::DescriptorPool& descriptorPool);

private:
  static constexpr plUInt32 s_uiPoolBaseSize = 1024;

  static vk::DescriptorPool GetNewPool();

  static vk::DescriptorPool s_currentPool;
  static plHybridArray<vk::DescriptorPool, 4> s_freePools;
  static vk::Device s_device;
  static plHashTable<vk::DescriptorType, float> s_descriptorWeights;
};
