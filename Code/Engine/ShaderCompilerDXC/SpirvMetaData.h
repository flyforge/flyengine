#pragma once
#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

struct plVulkanDescriptorSetLayoutBinding
{
  enum ResourceType : plUInt8
  {
    ConstantBuffer,
    ResourceView,
    UAV,
    Sampler,
  };

  PLASMA_DECLARE_POD_TYPE();
  plStringView m_sName;                                                ///< Used to match the same descriptor use across multiple stages.
  plUInt8 m_uiBinding = 0;                                             ///< Target descriptor binding slot.
  plUInt8 m_uiVirtualBinding = 0;                                      ///< Virtual binding slot in the high level renderer interface.
  plShaderResourceType::Enum m_plType = plShaderResourceType::Unknown; ///< PLASMA shader resource type, needed to find compatible fallback resources.
  ResourceType m_Type = ResourceType::ConstantBuffer;                  ///< Resource type, used to map to the correct PLASMA resource type.
  plUInt16 m_uiDescriptorType = 0;                                     ///< Maps to vk::DescriptorType
  plUInt32 m_uiDescriptorCount = 1;                                    ///< For now, this must be 1 as PLASMA does not support descriptor arrays right now.
  plUInt32 m_uiWordOffset = 0;                                         ///< Offset of the location in the spirv code where the binding index is located to allow changing it at runtime.
};

struct plVulkanDescriptorSetLayout
{
  plUInt32 m_uiSet = 0;
  plHybridArray<plVulkanDescriptorSetLayoutBinding, 6> bindings;
};

struct plVulkanVertexInputAttribute
{
  plGALVertexAttributeSemantic::Enum m_eSemantic = plGALVertexAttributeSemantic::Position;
  plUInt8 m_uiLocation = 0;
  plGALResourceFormat::Enum m_eFormat = plGALResourceFormat::XYZFloat;
};

namespace plSpirvMetaData
{
  constexpr plUInt32 s_uiSpirvMetaDataMagicNumber = 0x4B565A45; //PLASMAVK

  enum MetaDataVersion
  {
    Version1 = 1,
    Version2 = 2, ///< m_uiVirtualBinding, m_plType added
    Version3 = 3, ///< Vertex input binding
  };

  void Write(plStreamWriter& stream, const plArrayPtr<plUInt8>& shaderCode, const plDynamicArray<plVulkanDescriptorSetLayout>& sets, const plDynamicArray<plVulkanVertexInputAttribute>& vertexInputAttributes)
  {
    stream << s_uiSpirvMetaDataMagicNumber;
    stream.WriteVersion(MetaDataVersion::Version3);
    const plUInt32 uiSize = shaderCode.GetCount();
    stream << uiSize;
    stream.WriteBytes(shaderCode.GetPtr(), uiSize).AssertSuccess();

    const plUInt8 uiSets = sets.GetCount();
    stream << uiSets;
    for (plUInt8 i = 0; i < uiSets; i++)
    {
      const plVulkanDescriptorSetLayout& set = sets[i];
      stream << set.m_uiSet;
      const plUInt8 uiBindings = set.bindings.GetCount();
      stream << uiBindings;
      for (plUInt8 j = 0; j < uiBindings; j++)
      {
        const plVulkanDescriptorSetLayoutBinding& binding = set.bindings[j];
        stream.WriteString(binding.m_sName).AssertSuccess();
        stream << binding.m_uiBinding;
        stream << binding.m_uiVirtualBinding;
        stream << static_cast<plUInt8>(binding.m_plType);
        stream << static_cast<plUInt8>(binding.m_Type);
        stream << binding.m_uiDescriptorType;
        stream << binding.m_uiDescriptorCount;
        stream << binding.m_uiWordOffset;
      }
    }

    const plUInt8 uiVIA = vertexInputAttributes.GetCount();
    stream << uiVIA;
    for (plUInt8 i = 0; i < uiVIA; i++)
    {
      const plVulkanVertexInputAttribute& via = vertexInputAttributes[i];
      stream << static_cast<plUInt8>(via.m_eSemantic);
      stream << via.m_uiLocation;
      stream << static_cast<plUInt8>(via.m_eFormat);
    }
  }

  /// \brief Reads Vulkan shader code and meta data from a data buffer. Note that 'data' must be kept alive for the lifetime of the shader as this functions stores views into this memory in its out parameters.
  /// \param data Raw data buffer to read the shader code and meta data from.
  /// \param out_shaderCode Will be filled with a view into data that contains the shader byte code.
  /// \param out_sets Will be filled with shader meta data. Note that this array contains string views into 'data'.
  void Read(const plArrayPtr<const plUInt8> data, plArrayPtr<const plUInt8>& out_shaderCode, plDynamicArray<plVulkanDescriptorSetLayout>& out_sets, plDynamicArray<plVulkanVertexInputAttribute>& out_vertexInputAttributes)
  {
    plRawMemoryStreamReader stream(data.GetPtr(), data.GetCount());

    plUInt32 uiMagicNumber;
    stream >> uiMagicNumber;
    PLASMA_ASSERT_DEV(uiMagicNumber == s_uiSpirvMetaDataMagicNumber, "Vulkan shader does not start with s_uiSpirvMetaDataMagicNumber");
    plTypeVersion uiVersion = stream.ReadVersion(MetaDataVersion::Version3);

    plUInt32 uiSize = 0;
    stream >> uiSize;
    out_shaderCode = plArrayPtr<const plUInt8>(&data[(plUInt32)stream.GetReadPosition()], uiSize);
    stream.SkipBytes(uiSize);

    plUInt8 uiSets = 0;
    stream >> uiSets;
    out_sets.Reserve(uiSets);

    for (plUInt8 i = 0; i < uiSets; i++)
    {
      plVulkanDescriptorSetLayout& set = out_sets.ExpandAndGetRef();
      stream >> set.m_uiSet;
      plUInt8 uiBindings = 0;
      stream >> uiBindings;
      set.bindings.Reserve(uiBindings);

      for (plUInt8 j = 0; j < uiBindings; j++)
      {
        plVulkanDescriptorSetLayoutBinding& binding = set.bindings.ExpandAndGetRef();

        plUInt32 uiStringElements = 0;
        stream >> uiStringElements;
        binding.m_sName = plStringView(reinterpret_cast<const char*>(&data[(plUInt32)stream.GetReadPosition()]), uiStringElements);
        stream.SkipBytes(uiStringElements);
        stream >> binding.m_uiBinding;
        if (uiVersion >= MetaDataVersion::Version2)
        {
          stream >> binding.m_uiVirtualBinding;
          stream >> reinterpret_cast<plUInt8&>(binding.m_plType);
        }
        else
        {
          binding.m_uiVirtualBinding = binding.m_uiBinding;
          binding.m_plType = plShaderResourceType::Texture2D;
        }
        stream >> reinterpret_cast<plUInt8&>(binding.m_Type);
        stream >> binding.m_uiDescriptorType;
        stream >> binding.m_uiDescriptorCount;
        stream >> binding.m_uiWordOffset;
      }
    }

    if (uiVersion >= MetaDataVersion::Version3)
    {
      plUInt8 uiVIA = 0;
      stream >> uiVIA;
      out_vertexInputAttributes.Reserve(uiVIA);
      for (plUInt8 i = 0; i < uiVIA; i++)
      {
        plVulkanVertexInputAttribute& via = out_vertexInputAttributes.ExpandAndGetRef();
        stream >> reinterpret_cast<plUInt8&>(via.m_eSemantic);
        stream >> via.m_uiLocation;
        stream >> reinterpret_cast<plUInt8&>(via.m_eFormat);
      }
    }
  }
} // namespace plSpirvMetaData
