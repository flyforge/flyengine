#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <ShaderCompilerDXC/SpirvMetaData.h>

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
  PLASMA_SUCCEED_OR_RETURN(CreateBindingMapping());

  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);

  m_SetBindings.Clear();

  for (const plShaderResourceBinding& binding : GetBindingMapping())
  {
    plUInt32 iMaxSets = plMath::Max((plUInt32)m_SetBindings.GetCount(), static_cast<plUInt32>(binding.m_iSet + 1));
    m_SetBindings.SetCount(iMaxSets);
    m_SetBindings[binding.m_iSet].PushBack(binding);
  }

//  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; i++)
//  {
//    const vk::ShaderStageFlags vulkanStage = plConversionUtilsVulkan::GetShaderStage((plGALShaderStage::Enum)i);
//    if (m_Description.HasByteCodeForStage((plGALShaderStage::Enum)i))
//    {
//      for (const plShaderResourceBinding& binding : m_Description.m_ByteCodes[i]->m_ShaderResourceBindings)
//      {
//        iMaxSets = plMath::Max(iMaxSets, binding.m_iSet);
//        for (int j = 0; j < m_Bindings.GetCount(); ++j)
//        {
//          auto& binding2 = m_Bindings[j];
//          if (binding.m_iSet == binding2.m_iSet && binding.m_iSlot == binding2.m_iSlot)
//          {
//            PLASMA_ASSERT_DEBUG(binding.m_DescriptorType == binding2.m_DescriptorType, "Shader resource descriptor types do not match between stages");
//            PLASMA_ASSERT_DEBUG(binding.m_TextureType == binding2.m_TextureType, "Shader resource texture types do not match between stages");
//            PLASMA_ASSERT_DEBUG(binding.m_uiArraySize == binding2.m_uiArraySize, "Shader resource array size do not match between stages");
//            PLASMA_ASSERT_DEBUG(binding.m_sName == binding2.m_sName, "Shader resource names do not match between stages");
//            binding2.m_Stages |= binding2.m_Stages;
//            break;
//          }
//        }
//
//        m_Bindings.PushBack(binding);
//      }
//    }
//  }
  // Sort mappings
  m_descriptorSetLayoutDesc.SetCount(m_SetBindings.GetCount());
  for (plUInt32 iSet = 0; iSet < m_SetBindings.GetCount(); ++iSet)
  {
    m_SetBindings[iSet].Sort([](const plShaderResourceBinding& lhs, const plShaderResourceBinding& rhs)
      { return lhs.m_iSlot < rhs.m_iSlot; });

    // Build Vulkan descriptor set layout
    for (plUInt32 i = 0; i < m_SetBindings[iSet].GetCount(); i++)
    {
      const plShaderResourceBinding& plBinding = m_SetBindings[iSet][i];
      vk::DescriptorSetLayoutBinding& binding = m_descriptorSetLayoutDesc[plBinding.m_iSet].m_bindings.ExpandAndGetRef();

      binding.binding = plBinding.m_iSlot;
      binding.descriptorType = plConversionUtilsVulkan::GetDescriptorType(plBinding.m_DescriptorType);
      binding.descriptorCount = plBinding.m_uiArraySize;
      binding.stageFlags = plConversionUtilsVulkan::GetShaderStages(plBinding.m_Stages);
    }

    m_descriptorSetLayoutDesc[iSet].ComputeHash();
  }

  // Build shaders
  vk::ShaderModuleCreateInfo createInfo;
  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((plGALShaderStage::Enum)i))
    {
        createInfo.codeSize = m_Description.m_ByteCodes[i]->m_ByteCode.GetCount();
        PLASMA_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
        createInfo.pCode = reinterpret_cast<const plUInt32*>(m_Description.m_ByteCodes[i]->m_ByteCode.GetData());
        VK_SUCCEED_OR_RETURN_PLASMA_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
    }
  }

  return PLASMA_SUCCESS;
}

plResult plGALShaderVulkan::DeInitPlatform(plGALDevice* pDevice)
{
  DestroyBindingMapping();

  m_descriptorSetLayoutDesc.Clear();
  m_SetBindings.Clear();

  auto* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);
  for (auto & m_Shader : m_Shaders)
  {
    pVulkanDevice->DeleteLater(m_Shader);
  }
  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_ShaderVulkan);
