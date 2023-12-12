
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

#include <RendererCore/Shader/ShaderStageBinary.h>
#include <vulkan/vulkan.hpp>

class PLASMA_RENDERERVULKAN_DLL plGALShaderVulkan : public plGALShader
{
public:
  /// \brief Used as input to plResourceCacheVulkan::RequestDescriptorSetLayout to create a vk::DescriptorSetLayout.
  struct DescriptorSetLayoutDesc
  {
    mutable plUInt32 m_uiHash = 0;
    plHybridArray<vk::DescriptorSetLayoutBinding, 6> m_bindings;
    void ComputeHash();
  };

  /// \brief Remaps high level resource binding to the descriptor layout used by this shader.
  struct BindingMapping
  {
    enum Type : plUInt8
    {
      ConstantBuffer,
      ResourceView,
      UAV,
      Sampler,
    };
    vk::DescriptorType m_descriptorType = vk::DescriptorType::eSampler;  ///< Descriptor slot type.
    plShaderResourceType::Enum m_plType = plShaderResourceType::Unknown; ///< PLASMA resource type. We need this to find a compatible fallback resource is a descriptor slot is empty.
    Type m_type = Type::ConstantBuffer;                                  ///< Source resource type in the high level binding model.
    plGALShaderStage::Enum m_stage = plGALShaderStage::ENUM_COUNT;       ///< Source stage in the high level resource binding model.
    plUInt8 m_uiSource = 0;                                              ///< Source binding index in the high level resource binding model.
    plUInt8 m_uiTarget = 0;                                              ///< Target binding index in the descriptor set layout.
    vk::PipelineStageFlags m_targetStages;                               ///< Target stages that this mapping is used in.
    plStringView m_sName;
  };

  struct VertexInputAttribute
  {
    plGALVertexAttributeSemantic::Enum m_eSemantic = plGALVertexAttributeSemantic::Position;
    plUInt8 m_uiLocation = 0;
    plGALResourceFormat::Enum m_eFormat = plGALResourceFormat::XYZFloat;
  };

  void SetDebugName(const char* szName) const override;

  PLASMA_ALWAYS_INLINE vk::ShaderModule GetShader(plGALShaderStage::Enum stage) const;
  PLASMA_ALWAYS_INLINE const DescriptorSetLayoutDesc& GetDescriptorSetLayout() const;
  PLASMA_ALWAYS_INLINE const plArrayPtr<const BindingMapping> GetBindingMapping() const;
  PLASMA_ALWAYS_INLINE const plArrayPtr<const VertexInputAttribute> GetVertexInputAttributes() const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALShaderVulkan(const plGALShaderCreationDescription& description);
  virtual ~plGALShaderVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

private:
  DescriptorSetLayoutDesc m_descriptorSetLayoutDesc;
  plHybridArray<BindingMapping, 16> m_BindingMapping;
  plHybridArray<VertexInputAttribute, 8> m_VertexInputAttributes;
  vk::ShaderModule m_Shaders[plGALShaderStage::ENUM_COUNT];
};

#include <RendererVulkan/Shader/Implementation/ShaderVulkan_inl.h>
