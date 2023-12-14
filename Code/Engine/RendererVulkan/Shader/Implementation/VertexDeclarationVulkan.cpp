#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>

plGALVertexDeclarationVulkan::plGALVertexDeclarationVulkan(const plGALVertexDeclarationCreationDescription& Description)
  : plGALVertexDeclaration(Description)
{
}

plGALVertexDeclarationVulkan::~plGALVertexDeclarationVulkan() = default;

plResult plGALVertexDeclarationVulkan::InitPlatform(plGALDevice* pDevice)
{
  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);

  const plGALShaderVulkan* pShader = static_cast<const plGALShaderVulkan*>(pDevice->GetShader(m_Description.m_hShader));

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(plGALShaderStage::VertexShader))
  {
    return PLASMA_FAILURE;
  }

  plHybridArray<plShaderVertexInputAttribute, 8> vias(pShader->GetVertexInputAttributes());
  auto FindLocation = [&](plGALVertexAttributeSemantic::Enum sematic, plGALResourceFormat::Enum format) -> plUInt32 {
    for (plUInt32 i = 0; i < vias.GetCount(); i++)
    {
      if (vias[i].m_eSemantic == sematic)
      {
        //PLASMA_ASSERT_DEBUG(vias[i].m_eFormat == format, "Found matching sematic {} but format differs: {} : {}", sematic, format, vias[i].m_eFormat);
        plUInt32 uiLocation = vias[i].m_uiLocation;
        vias.RemoveAtAndSwap(i);
        return uiLocation;
      }
    }
    return plMath::MaxValue<plUInt32>();
  };

  // Copy attribute descriptions
  plUInt32 usedBindings = 0;
  for (plUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const plGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    const plUInt32 uiLocation = FindLocation(Current.m_eSemantic, Current.m_eFormat);
    if (uiLocation == plMath::MaxValue<plUInt32>())
    {
      plLog::Warning("Vertex buffer semantic {} not used by shader", Current.m_eSemantic);
      continue;
    }
    vk::VertexInputAttributeDescription& attrib = m_attributes.ExpandAndGetRef();
    attrib.binding = Current.m_uiVertexBufferSlot;
    attrib.location = uiLocation;
    attrib.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType;
    attrib.offset = Current.m_uiOffset;

    if (attrib.format == vk::Format::eUndefined)
    {
      plLog::Error("Vertex attribute format {0} of attribute at index {1} is undefined!", Current.m_eFormat, i);
      return PLASMA_FAILURE;
    }

    usedBindings |= PLASMA_BIT(Current.m_uiVertexBufferSlot);
    if (Current.m_uiVertexBufferSlot >= m_bindings.GetCount())
    {
      m_bindings.SetCount(Current.m_uiVertexBufferSlot + 1);
    }
    vk::VertexInputBindingDescription& binding = m_bindings[Current.m_uiVertexBufferSlot];
    binding.binding = Current.m_uiVertexBufferSlot;
    binding.stride = 0;
    binding.inputRate = Current.m_bInstanceData ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex;
  }
  for (plInt32 i = (plInt32)m_bindings.GetCount() - 1; i >= 0; --i)
  {
    if ((usedBindings & PLASMA_BIT(i)) == 0)
    {
      m_bindings.RemoveAtAndCopy(i);
    }
  }

  if (!vias.IsEmpty())
  {
    plLog::Error("Vertex buffers do not cover all vertex attributes defined in the shader!");
    return PLASMA_FAILURE;
  }
  return PLASMA_SUCCESS;
}

plResult plGALVertexDeclarationVulkan::DeInitPlatform(plGALDevice* pDevice)
{
  return PLASMA_SUCCESS;
}



PLASMA_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_VertexDeclarationVulkan);
