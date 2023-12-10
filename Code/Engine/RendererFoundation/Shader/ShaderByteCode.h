
#pragma once

#include <Foundation/Containers/DynamicArray.h>
//#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <Foundation/Types/RefCounted.h>

struct PLASMA_RENDERERFOUNDATION_DLL plShaderConstant
{
  PLASMA_DECLARE_MEM_RELOCATABLE_TYPE();

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

class PLASMA_RENDERERFOUNDATION_DLL plShaderConstantBufferLayout : public plRefCounted
{
public:
  plUInt32 m_uiTotalSize = 0;
  plHybridArray<plShaderConstant, 16> m_Constants;
};

/// \brief This is needed to figure out how to map a plGALVertexDeclaration to a vertex shader stage.
struct PLASMA_RENDERERFOUNDATION_DLL plShaderVertexInputAttribute
{
  PLASMA_DECLARE_MEM_RELOCATABLE_TYPE();

  plEnum<plGALVertexAttributeSemantic> m_eSemantic = plGALVertexAttributeSemantic::Position;
  plEnum<plGALResourceFormat> m_eFormat = plGALResourceFormat::XYZFloat;
  plUInt8 m_uiLocation = 0; // The bind slot of a vertex input
};

struct PLASMA_RENDERERFOUNDATION_DLL plShaderResourceBinding
{
  PLASMA_DECLARE_MEM_RELOCATABLE_TYPE();

  plEnum<plGALShaderDescriptorType> m_DescriptorType;
  plEnum<plGALShaderTextureType> m_TextureType;
  plBitflags<plGALShaderStageFlags> m_Stages;
  plInt16 m_iSet = -1;
  plInt16 m_iSlot = -1;
  plUInt32 m_uiArraySize = 1; // 0 if bindless
  plHashedString m_sName;
  plScopedRefPointer<plShaderConstantBufferLayout> m_pLayout; // Only valid if plGALShaderDescriptorType is ConstantBuffer or StructuredBuffer / StructuredBufferRW ??? #TODO_SHADER

  static plResult CreateMergedShaderResourceBinding(const plArrayPtr<plArrayPtr<const plShaderResourceBinding>>& resourcesPerStage, plDynamicArray<plShaderResourceBinding>& out_bindings);
};

/// \brief This class wraps shader byte code storage.
/// Since byte code can have different requirements for alignment, padding etc. this class manages it.
/// Also since byte code is shared between multiple shaders (e.g. same vertex shaders for different pixel shaders)
/// the instances of the byte codes are reference counted.
class PLASMA_RENDERERFOUNDATION_DLL plGALShaderByteCode : public plRefCounted
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

  // Filled out by compiler base library
  plEnum<plGALShaderStage> m_Stage = plGALShaderStage::ENUM_COUNT;
  bool m_bWasCompiledWithDebug = false;
};

#include <RendererFoundation/Shader/Implementation/ShaderByteCode_inl.h>
