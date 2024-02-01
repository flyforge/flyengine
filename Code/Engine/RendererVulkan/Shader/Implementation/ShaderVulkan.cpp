#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererFoundation/Device/ImmutableSamplers.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

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
  PL_SUCCEED_OR_RETURN(CreateBindingMapping());

  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);

  m_SetBindings.Clear();

  for (const plShaderResourceBinding& binding : GetBindingMapping())
  {
    if (binding.m_ResourceType == plGALShaderResourceType::PushConstants)
      continue;

    plUInt32 iMaxSets = plMath::Max((plUInt32)m_SetBindings.GetCount(), static_cast<plUInt32>(binding.m_iSet + 1));
    m_SetBindings.SetCount(iMaxSets);
    m_SetBindings[binding.m_iSet].PushBack(binding);
  }

  const plGALImmutableSamplers::ImmutableSamplers& immutableSamplers = plGALImmutableSamplers::GetImmutableSamplers();
  auto GetImmutableSampler = [&](const plHashedString& sName) -> const vk::Sampler*
  {
    if (const plGALSamplerStateHandle* hSampler = immutableSamplers.GetValue(sName))
    {
      const auto* pSampler = static_cast<const plGALSamplerStateVulkan*>(pVulkanDevice->GetSamplerState(*hSampler));
      return &pSampler->GetImageInfo().sampler;
    }
    return nullptr;
  };

  // Sort mappings and build descriptor set layout
  plHybridArray<DescriptorSetLayoutDesc, 4> descriptorSetLayoutDesc;
  descriptorSetLayoutDesc.SetCount(m_SetBindings.GetCount());
  m_descriptorSetLayout.SetCount(m_SetBindings.GetCount());
  for (plUInt32 iSet = 0; iSet < m_SetBindings.GetCount(); ++iSet)
  {
    m_SetBindings[iSet].Sort([](const plShaderResourceBinding& lhs, const plShaderResourceBinding& rhs)
      { return lhs.m_iSlot < rhs.m_iSlot; });

    // Build Vulkan descriptor set layout
    for (plUInt32 i = 0; i < m_SetBindings[iSet].GetCount(); i++)
    {
      const plShaderResourceBinding& plBinding = m_SetBindings[iSet][i];
      vk::DescriptorSetLayoutBinding& binding = descriptorSetLayoutDesc[plBinding.m_iSet].m_bindings.ExpandAndGetRef();

      binding.binding = plBinding.m_iSlot;
      binding.descriptorType = plConversionUtilsVulkan::GetDescriptorType(plBinding.m_ResourceType);
      binding.descriptorCount = plBinding.m_uiArraySize;
      binding.stageFlags = plConversionUtilsVulkan::GetShaderStages(plBinding.m_Stages);
      binding.pImmutableSamplers = plBinding.m_ResourceType == plGALShaderResourceType::Sampler ? GetImmutableSampler(plBinding.m_sName) : nullptr;
    }

    descriptorSetLayoutDesc[iSet].ComputeHash();
    m_descriptorSetLayout[iSet] = plResourceCacheVulkan::RequestDescriptorSetLayout(descriptorSetLayoutDesc[iSet]);
  }

  // Remove immutable samplers and push constants from binding info
  {
    for (plUInt32 uiSet = 0; uiSet < m_SetBindings.GetCount(); ++uiSet)
    {
      for (plInt32 iIndex = (plInt32)m_SetBindings[uiSet].GetCount() - 1; iIndex >= 0; --iIndex)
      {
        const bool bIsImmutableSample = m_SetBindings[uiSet][iIndex].m_ResourceType == plGALShaderResourceType::Sampler && immutableSamplers.Contains(m_SetBindings[uiSet][iIndex].m_sName);

        if (bIsImmutableSample)
        {
          m_SetBindings[uiSet].RemoveAtAndCopy(iIndex);
        }
      }
    }
    for (plInt32 iIndex = (plInt32)m_BindingMapping.GetCount() - 1; iIndex >= 0; --iIndex)
    {
      const bool bIsImmutableSample = m_BindingMapping[iIndex].m_ResourceType == plGALShaderResourceType::Sampler && immutableSamplers.Contains(m_BindingMapping[iIndex].m_sName);
      const bool bIsPushConstant = m_BindingMapping[iIndex].m_ResourceType == plGALShaderResourceType::PushConstants;

      if (bIsPushConstant)
      {
        const auto& pushConstant = m_BindingMapping[iIndex];
        m_pushConstants.size = pushConstant.m_pLayout->m_uiTotalSize;
        m_pushConstants.offset = 0;
        m_pushConstants.stageFlags = plConversionUtilsVulkan::GetShaderStages(pushConstant.m_Stages);
      }

      if (bIsImmutableSample || bIsPushConstant)
      {
        m_BindingMapping.RemoveAtAndCopy(iIndex);
      }
    }
  }

  // Build shaders
  vk::ShaderModuleCreateInfo createInfo;
  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((plGALShaderStage::Enum)i))
    {
      createInfo.codeSize = m_Description.m_ByteCodes[i]->m_ByteCode.GetCount();
      PL_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
      createInfo.pCode = reinterpret_cast<const plUInt32*>(m_Description.m_ByteCodes[i]->m_ByteCode.GetData());
      VK_SUCCEED_OR_RETURN_PL_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
    }
  }

  return PL_SUCCESS;
}

plResult plGALShaderVulkan::DeInitPlatform(plGALDevice* pDevice)
{
  DestroyBindingMapping();

  // Right now, we do not destroy descriptor set layouts as they are shared among many shaders.
  m_descriptorSetLayout.Clear();
  m_SetBindings.Clear();

  auto* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);
  for (auto& m_Shader : m_Shaders)
  {
    pVulkanDevice->DeleteLater(m_Shader);
  }
  return PL_SUCCESS;
}

PL_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_ShaderVulkan);
