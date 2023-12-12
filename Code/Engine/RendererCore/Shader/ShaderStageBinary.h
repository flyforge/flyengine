#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class PLASMA_RENDERERCORE_DLL plShaderConstantBufferLayout : public plRefCounted
{
public:
  struct Constant
  {
    PLASMA_DECLARE_MEM_RELOCATABLE_TYPE();

    struct Type
    {
      typedef plUInt8 StorageType;

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

    Constant()
    {
      m_uiArrayElements = 0;
      m_uiOffset = 0;
    }

    void CopyDataFormVariant(plUInt8* pDest, plVariant* pValue) const;

    plHashedString m_sName;
    plEnum<Type> m_Type;
    plUInt8 m_uiArrayElements;
    plUInt16 m_uiOffset;
  };

private:
  friend class plShaderStageBinary;
  friend class plMemoryUtils;

  plShaderConstantBufferLayout();
  ~plShaderConstantBufferLayout();

public:
  plResult Write(plStreamWriter& inout_stream) const;
  plResult Read(plStreamReader& inout_stream);

  plUInt32 m_uiTotalSize;
  plHybridArray<Constant, 16> m_Constants;
};

struct PLASMA_RENDERERCORE_DLL plShaderResourceBinding
{
  PLASMA_DECLARE_MEM_RELOCATABLE_TYPE();


  plShaderResourceBinding();
  ~plShaderResourceBinding();

  plShaderResourceType::Enum m_Type;
  plInt32 m_iSlot;
  plHashedString m_sName;
  plScopedRefPointer<plShaderConstantBufferLayout> m_pLayout;
};

class PLASMA_RENDERERCORE_DLL plShaderStageBinary
{
public:
  enum Version
  {
    Version0,
    Version1,
    Version2,
    Version3, // Added Material Parameters
    Version4, // Constant buffer layouts
    Version5, // Debug flag

    ENUM_COUNT,
    VersionCurrent = ENUM_COUNT - 1
  };

  plShaderStageBinary();
  ~plShaderStageBinary();

  plResult Write(plStreamWriter& inout_stream) const;
  plResult Read(plStreamReader& inout_stream);

  plDynamicArray<plUInt8>& GetByteCode();

  void AddShaderResourceBinding(const plShaderResourceBinding& binding);
  plArrayPtr<const plShaderResourceBinding> GetShaderResourceBindings() const;
  const plShaderResourceBinding* GetShaderResourceBinding(const plTempHashedString& sName) const;

  plShaderConstantBufferLayout* CreateConstantBufferLayout() const;

private:
  friend class plRenderContext;
  friend class plShaderCompiler;
  friend class plShaderPermutationResource;
  friend class plShaderPermutationResourceLoader;

  plUInt32 m_uiSourceHash = 0;
  plGALShaderStage::Enum m_Stage = plGALShaderStage::ENUM_COUNT;
  plDynamicArray<plUInt8> m_ByteCode;
  plScopedRefPointer<plGALShaderByteCode> m_GALByteCode;
  plHybridArray<plShaderResourceBinding, 8> m_ShaderResourceBindings;
  bool m_bWasCompiledWithDebug = false;

  plResult WriteStageBinary(plLogInterface* pLog) const;
  static plShaderStageBinary* LoadStageBinary(plGALShaderStage::Enum Stage, plUInt32 uiHash);

  static void OnEngineShutdown();

  static plMap<plUInt32, plShaderStageBinary> s_ShaderStageBinaries[plGALShaderStage::ENUM_COUNT];
};
