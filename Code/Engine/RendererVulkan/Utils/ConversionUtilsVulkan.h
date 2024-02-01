#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <vulkan/vulkan.hpp>

PL_DEFINE_AS_POD_TYPE(vk::PresentModeKHR);

/// \brief Helper functions to convert and extract Vulkan objects from PL objects.
class PL_RENDERERVULKAN_DLL plConversionUtilsVulkan
{
public:
  /// \brief Helper function to hash vk enums.
  template <typename T, typename R = typename std::underlying_type<T>::type>
  static R GetUnderlyingValue(T value)
  {
    return static_cast<typename std::underlying_type<T>::type>(value);
  }

  /// \brief Helper function to hash vk flags.
  template <typename T>
  static auto GetUnderlyingFlagsValue(T value)
  {
    return static_cast<typename T::MaskType>(value);
  }

  static vk::SampleCountFlagBits GetSamples(plEnum<plGALMSAASampleCount> samples);
  static vk::PresentModeKHR GetPresentMode(plEnum<plGALPresentMode> presentMode, const plDynamicArray<vk::PresentModeKHR>& supportedModes);
  static vk::ImageSubresourceRange GetSubresourceRange(const plGALTextureCreationDescription& texDesc, const plGALRenderTargetViewCreationDescription& desc);
  static vk::ImageSubresourceRange GetSubresourceRange(const plGALTextureCreationDescription& texDesc, const plGALResourceViewCreationDescription& viewDesc);
  static vk::ImageSubresourceRange GetSubresourceRange(const plGALTextureCreationDescription& texDesc, const plGALUnorderedAccessViewCreationDescription& viewDesc);
  static vk::ImageSubresourceRange GetSubresourceRange(const vk::ImageSubresourceLayers& layers);
  static vk::ImageViewType GetImageViewType(plEnum<plGALTextureType> texType, bool bIsArray);

  static bool IsDepthFormat(vk::Format format);
  static bool IsStencilFormat(vk::Format format);
  static vk::PrimitiveTopology GetPrimitiveTopology(plEnum<plGALPrimitiveTopology> topology);
  static vk::ShaderStageFlagBits GetShaderStage(plGALShaderStage::Enum stage);
  static vk::ShaderStageFlagBits GetShaderStages(plBitflags<plGALShaderStageFlags> stages);
  static vk::PipelineStageFlags GetPipelineStage(plGALShaderStage::Enum stage);
  static vk::PipelineStageFlags GetPipelineStage(vk::ShaderStageFlags flags);
  static vk::PipelineStageFlags GetPipelineStages(plBitflags<plGALShaderStageFlags> stages);
  static vk::DescriptorType GetDescriptorType(plGALShaderResourceType::Enum type);
};

#include <RendererVulkan/Utils/Implementation/ConversionUtilsVulkan.inl.h>
