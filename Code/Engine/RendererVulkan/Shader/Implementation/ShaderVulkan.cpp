#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <ShaderCompilerDXC/SpirvMetaData.h>

PLASMA_CHECK_AT_COMPILETIME(plVulkanDescriptorSetLayoutBinding::ConstantBuffer == plGALShaderVulkan::BindingMapping::ConstantBuffer);
PLASMA_CHECK_AT_COMPILETIME(plVulkanDescriptorSetLayoutBinding::ResourceView == plGALShaderVulkan::BindingMapping::ResourceView);
PLASMA_CHECK_AT_COMPILETIME(plVulkanDescriptorSetLayoutBinding::UAV == plGALShaderVulkan::BindingMapping::UAV);
PLASMA_CHECK_AT_COMPILETIME(plVulkanDescriptorSetLayoutBinding::Sampler == plGALShaderVulkan::BindingMapping::Sampler);

void plGALShaderVulkan::DescriptorSetLayoutDesc::ComputeHash()
{
  plHashStreamWriter32 writer;
  const plUInt32 uiSize = m_bindings.GetCount();
  for (plUInt32 i = 0; i < uiSize; i++)
  {
    const auto& binding = m_bindings[i];
    writer << binding.binding;
    writer << plConversionUtilsVulkan::GetUnderlyingValue(binding.descriptorType);
    writer << binding.descriptorCount;
    writer << plConversionUtilsVulkan::GetUnderlyingFlagsValue(binding.stageFlags);
    writer << binding.pImmutableSamplers;
  }
  m_uiHash = writer.GetHashValue();
}

plGALShaderVulkan::plGALShaderVulkan(const plGALShaderCreationDescription& Description)
  : plGALShader(Description)
{
}

plGALShaderVulkan::~plGALShaderVulkan() {}

void plGALShaderVulkan::SetDebugName(const char* szName) const
{
  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(plGALDevice::GetDefaultDevice());
  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; i++)
  {
    pVulkanDevice->SetDebugName(szName, m_Shaders[i]);
  }
}

plResult plGALShaderVulkan::InitPlatform(plGALDevice* pDevice)
{
  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);

  // Extract meta data and shader code.
  plArrayPtr<const plUInt8> shaderCode[plGALShaderStage::ENUM_COUNT];
  plDynamicArray<plVulkanDescriptorSetLayout> sets[plGALShaderStage::ENUM_COUNT];
  plHybridArray<plVulkanVertexInputAttribute, 8> vertexInputAttributes;

  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((plGALShaderStage::Enum)i))
    {
      plArrayPtr<const plUInt8> metaData(reinterpret_cast<const plUInt8*>(m_Description.m_ByteCodes[i]->GetByteCode()), m_Description.m_ByteCodes[i]->GetSize());
      // Only the vertex shader stores vertexInputAttributes, so passing in the array into other shaders is just a no op.
      plSpirvMetaData::Read(metaData, shaderCode[i], sets[i], vertexInputAttributes);
    }
  }

  // For now the meta data and what the shader exposes is the exact same data but this might change so different types are used.
  for (plVulkanVertexInputAttribute& via : vertexInputAttributes)
  {
    m_VertexInputAttributes.PushBack({via.m_eSemantic, via.m_uiLocation, via.m_eFormat});
  }

  // Compute remapping.
  // Each shader stage is compiled individually and has its own binding indices.
  // In Vulkan we need to map all stages into one descriptor layout which requires us to remap some shader stages so no binding index conflicts appear.
  struct ShaderRemapping
  {
    const plVulkanDescriptorSetLayoutBinding* pBinding = 0;
    plUInt16 m_uiTarget = 0; ///< The new binding target that pBinding needs to be remapped to.
  };
  struct LayoutBinding
  {
    const plVulkanDescriptorSetLayoutBinding* m_binding = nullptr; ///< The first binding under which this resource was encountered.
    vk::ShaderStageFlags m_stages = {};                            ///< Bitflags of all stages that share this binding. Matching is done by name.
  };
  plHybridArray<ShaderRemapping, 6> remappings[plGALShaderStage::ENUM_COUNT]; ///< Remappings for each shader stage.
  plHybridArray<LayoutBinding, 6> sourceBindings;                             ///< Bindings across all stages. Can have gaps. Array index is the binding index.
  plMap<plStringView, plUInt32> bindingMap;                                   ///< Maps binding name to index in sourceBindings.

  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; i++)
  {
    const vk::ShaderStageFlags vulkanStage = plConversionUtilsVulkan::GetShaderStage((plGALShaderStage::Enum)i);
    if (m_Description.HasByteCodeForStage((plGALShaderStage::Enum)i))
    {
      PLASMA_ASSERT_DEV(sets[i].GetCount() <= 1, "Only a single descriptor set is currently supported.");

      for (plUInt32 j = 0; j < sets[i].GetCount(); j++)
      {
        const plVulkanDescriptorSetLayout& set = sets[i][j];
        PLASMA_ASSERT_DEV(set.m_uiSet == 0, "Only a single descriptor set is currently supported.");
        for (plUInt32 k = 0; k < set.bindings.GetCount(); k++)
        {
          const plVulkanDescriptorSetLayoutBinding& binding = set.bindings[k];
          // Does a binding already exist for the resource with the same name?
          if (plUInt32* pBindingIdx = bindingMap.GetValue(binding.m_sName))
          {
            LayoutBinding& layoutBinding = sourceBindings[*pBindingIdx];
            layoutBinding.m_stages |= vulkanStage;
            const plVulkanDescriptorSetLayoutBinding* pCurrentBinding = layoutBinding.m_binding;
            PLASMA_ASSERT_DEBUG(pCurrentBinding->m_Type == binding.m_Type, "The descriptor {} was found with different resource type {} and {}", binding.m_sName, pCurrentBinding->m_Type, binding.m_Type);
            PLASMA_ASSERT_DEBUG(pCurrentBinding->m_uiDescriptorType == binding.m_uiDescriptorType, "The descriptor {} was found with different type {} and {}", binding.m_sName, pCurrentBinding->m_uiDescriptorType, binding.m_uiDescriptorType);
            PLASMA_ASSERT_DEBUG(pCurrentBinding->m_uiDescriptorCount == binding.m_uiDescriptorCount, "The descriptor {} was found with different count {} and {}", binding.m_sName, pCurrentBinding->m_uiDescriptorCount, binding.m_uiDescriptorCount);
            // The binding index differs from the one already in the set, remapping is necessary.
            if (binding.m_uiBinding != *pBindingIdx)
            {
              remappings[i].PushBack({&binding, pCurrentBinding->m_uiBinding});
            }
          }
          else
          {
            plUInt8 uiTargetBinding = binding.m_uiBinding;
            // Doesn't exist yet, find a good place for it.
            if (binding.m_uiBinding >= sourceBindings.GetCount())
              sourceBindings.SetCount(binding.m_uiBinding + 1);

            // If the original binding index doesn't exist yet, use it (No remapping necessary).
            if (sourceBindings[binding.m_uiBinding].m_binding == nullptr)
            {
              sourceBindings[binding.m_uiBinding] = {&binding, vulkanStage};
              bindingMap[binding.m_sName] = uiTargetBinding;
            }
            else
            {
              // Binding index already in use, remapping necessary.
              uiTargetBinding = (plUInt8)sourceBindings.GetCount();
              sourceBindings.PushBack({&binding, vulkanStage});
              bindingMap[binding.m_sName] = uiTargetBinding;
              remappings[i].PushBack({&binding, uiTargetBinding});
            }

            // The shader reflection used by the high level renderer is per stage and assumes it can map resources to stages.
            // We build this remapping table to map our descriptor binding to the original per-stage resource binding model.
            BindingMapping& bindingMapping = m_BindingMapping.ExpandAndGetRef();
            bindingMapping.m_descriptorType = (vk::DescriptorType)binding.m_uiDescriptorType;
            bindingMapping.m_plType = binding.m_plType;
            bindingMapping.m_type = (BindingMapping::Type)binding.m_Type;
            bindingMapping.m_stage = (plGALShaderStage::Enum)i;
            bindingMapping.m_uiSource = binding.m_uiVirtualBinding;
            bindingMapping.m_uiTarget = uiTargetBinding;
            bindingMapping.m_sName = binding.m_sName;
          }
        }
      }
    }
  }
  m_BindingMapping.Sort([](const BindingMapping& lhs, const BindingMapping& rhs) { return lhs.m_uiTarget < rhs.m_uiTarget; });
  for (plUInt32 i = 0; i < m_BindingMapping.GetCount(); i++)
  {
    m_BindingMapping[i].m_targetStages = plConversionUtilsVulkan::GetPipelineStage(sourceBindings[m_BindingMapping[i].m_uiTarget].m_stages);
  }

  // Build Vulkan descriptor set layout
  for (plUInt32 i = 0; i < sourceBindings.GetCount(); i++)
  {
    const LayoutBinding& sourceBinding = sourceBindings[i];
    if (sourceBinding.m_binding != nullptr)
    {
      vk::DescriptorSetLayoutBinding& binding = m_descriptorSetLayoutDesc.m_bindings.ExpandAndGetRef();
      binding.binding = i;
      binding.descriptorType = (vk::DescriptorType)sourceBinding.m_binding->m_uiDescriptorType;
      binding.descriptorCount = sourceBinding.m_binding->m_uiDescriptorCount;
      binding.stageFlags = sourceBinding.m_stages;
    }
  }
  m_descriptorSetLayoutDesc.m_bindings.Sort([](const vk::DescriptorSetLayoutBinding& lhs, const vk::DescriptorSetLayoutBinding& rhs) { return lhs.binding < rhs.binding; });
  m_descriptorSetLayoutDesc.ComputeHash();

  // Remap and build shaders
  plUInt32 uiMaxShaderSize = 0;
  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; i++)
  {
    if (!remappings[i].IsEmpty())
    {
      uiMaxShaderSize = plMath::Max(uiMaxShaderSize, shaderCode[i].GetCount());
    }
  }

  vk::ShaderModuleCreateInfo createInfo;
  plDynamicArray<plUInt8> tempBuffer;
  tempBuffer.Reserve(uiMaxShaderSize);
  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((plGALShaderStage::Enum)i))
    {
      if (remappings[i].IsEmpty())
      {
        createInfo.codeSize = shaderCode[i].GetCount();
        PLASMA_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
        createInfo.pCode = reinterpret_cast<const plUInt32*>(shaderCode[i].GetPtr());
        VK_SUCCEED_OR_RETURN_PLASMA_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
      }
      else
      {
        tempBuffer = shaderCode[i];
        plUInt32* pData = reinterpret_cast<plUInt32*>(tempBuffer.GetData());
        for (const auto& remap : remappings[i])
        {
          PLASMA_ASSERT_DEBUG(pData[remap.pBinding->m_uiWordOffset] == remap.pBinding->m_uiBinding, "Spirv descriptor word offset does not point to descriptor index.");
          pData[remap.pBinding->m_uiWordOffset] = remap.m_uiTarget;
        }
        createInfo.codeSize = tempBuffer.GetCount();
        PLASMA_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
        createInfo.pCode = pData;
        VK_SUCCEED_OR_RETURN_PLASMA_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
      }
    }
  }

  return PLASMA_SUCCESS;
}

plResult plGALShaderVulkan::DeInitPlatform(plGALDevice* pDevice)
{
  m_descriptorSetLayoutDesc = {};
  m_BindingMapping.Clear();

  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);
  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; i++)
  {
    pVulkanDevice->DeleteLater(m_Shaders[i]);
  }
  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_ShaderVulkan);
