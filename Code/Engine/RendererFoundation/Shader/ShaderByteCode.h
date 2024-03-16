
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/RefCounted.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief The reflection data of a constant in a shader constant buffer.
/// \sa plShaderConstantBufferLayout
struct PL_RENDERERFOUNDATION_DLL plShaderConstant
{
  PL_DECLARE_MEM_RELOCATABLE_TYPE();

  struct Type
  {
    using StorageType = plUInt8;

    enum Enum
    {
      Default,
      Float1,
      Float2,
      Float3,
      Float4,
      Int1,
      Int2,
      Int3,
      Int4,
      UInt1,
      UInt2,
      UInt3,
      UInt4,
      Mat3x3,
      Mat4x4,
      Transform,
      Bool,
      Struct,
      ENUM_COUNT
    };
  };

  static plUInt32 s_TypeSize[Type::ENUM_COUNT];

  void CopyDataFormVariant(plUInt8* pDest, plVariant* pValue) const;

  plHashedString m_sName;
  plEnum<Type> m_Type;
  plUInt8 m_uiArrayElements = 0;
  plUInt16 m_uiOffset = 0;
};

/// \brief Reflection data of a shader constant buffer.
/// \sa plShaderResourceBinding
class PL_RENDERERFOUNDATION_DLL plShaderConstantBufferLayout : public plRefCounted
{
public:
  plUInt32 m_uiTotalSize = 0;
  plHybridArray<plShaderConstant, 16> m_Constants;
};

/// \brief Shader reflection of the vertex shader input.
/// This is needed to figure out how to map a plGALVertexDeclaration to a vertex shader stage.
/// \sa plGALShaderByteCode
struct PL_RENDERERFOUNDATION_DLL plShaderVertexInputAttribute
{
  PL_DECLARE_MEM_RELOCATABLE_TYPE();

  plEnum<plGALVertexAttributeSemantic> m_eSemantic = plGALVertexAttributeSemantic::Position;
  plEnum<plGALResourceFormat> m_eFormat = plGALResourceFormat::XYZFloat;
  plUInt8 m_uiLocation = 0; // The bind slot of a vertex input
};

/// \brief Shader reflection of a single shader resource (texture, constant buffer, etc.).
/// \sa plGALShaderByteCode
struct PL_RENDERERFOUNDATION_DLL plShaderResourceBinding
{
  PL_DECLARE_MEM_RELOCATABLE_TYPE();
  plEnum<plGALShaderResourceType> m_ResourceType;             //< The type of shader resource. Note, not all are supported by PL right now.
  plEnum<plGALShaderTextureType> m_TextureType;               //< Only valid if m_ResourceType is Texture, TextureRW or TextureAndSampler.
  plBitflags<plGALShaderStageFlags> m_Stages;                 //< The shader stages under which this resource is bound.
  plInt16 m_iSet = -1;                                        //< The set to which this resource belongs. Aka. Vulkan descriptor set.
  plInt16 m_iSlot = -1;                                       //< The slot under which the resource needs to be bound in the set.
  plUInt32 m_uiArraySize = 1;                                 //< Number of array elements. Only 1 is currently supported. 0 if bindless.
  plHashedString m_sName;                                     //< Name under which a resource must be bound to fulfill this resource binding.
  plScopedRefPointer<plShaderConstantBufferLayout> m_pLayout; //< Only valid if plGALShaderResourceType is ConstantBuffer or PushConstants. #TODO_SHADER We could also support this for StructuredBuffer / StructuredBufferRW, but currently there is no use case for that.

  static plResult CreateMergedShaderResourceBinding(const plArrayPtr<plArrayPtr<const plShaderResourceBinding>>& resourcesPerStage, plDynamicArray<plShaderResourceBinding>& out_bindings);
};

/// \brief This class wraps shader byte code storage.
/// Since byte code can have different requirements for alignment, padding etc. this class manages it.
/// Also since byte code is shared between multiple shaders (e.g. same vertex shaders for different pixel shaders)
/// the instances of the byte codes are reference counted.
class PL_RENDERERFOUNDATION_DLL plGALShaderByteCode : public plRefCounted
{
public:
  plGALShaderByteCode();
  ~plGALShaderByteCode();

  inline const void* GetByteCode() const;
  inline plUInt32 GetSize() const;
  inline bool IsValid() const;

  const plShaderResourceBinding* GetShaderResourceBinding(const plTempHashedString& sName) const
  {
    for (auto& binding : m_ShaderResourceBindings)
    {
      if (binding.m_sName == sName)
      {
        return &binding;
      }
    }

    return nullptr;
  }

public:
  // Filled out by Shader Compiler platform implementation
  plDynamicArray<plUInt8> m_ByteCode;
  plHybridArray<plShaderResourceBinding, 8> m_ShaderResourceBindings;
  plHybridArray<plShaderVertexInputAttribute, 8> m_ShaderVertexInput;

  // Only set in the hull shader.
  plUInt8 m_uiTessellationPatchControlPoints = 0;

  // Filled out by compiler base library
  plEnum<plGALShaderStage> m_Stage = plGALShaderStage::ENUM_COUNT;
  bool m_bWasCompiledWithDebug = false;
};

#include <RendererFoundation/Shader/Implementation/ShaderByteCode_inl.h>
