#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Declarations.h>

/// \brief Output of ParseShaderResources. A shader resource definition found inside the shader source code.
struct plShaderResourceDefinition
{
  /// \brief Just the declaration inside the shader source, e.g. "Texture1D Texture".
  plStringView m_sDeclaration;
  /// \brief The declaration with any optional register mappings, e.g. "Texture1D Texture : register(12t, space3)"
  plStringView m_sDeclarationAndRegister;
  /// \brief The extracted reflection of the resource containing type, slot, set etc.
  plShaderResourceBinding m_Binding;
};

/// \brief Flags that affect the compilation process of a shader
struct plShaderCompilerFlags
{
  using StorageType = plUInt8;
  enum Enum
  {
    Debug = PL_BIT(0),
    Default = 0,
  };

  struct Bits
  {
    StorageType Debug : 1;
  };
};
PL_DECLARE_FLAGS_OPERATORS(plShaderCompilerFlags);

/// \brief Storage used during the shader compilation process.
struct PL_RENDERERCORE_DLL plShaderProgramData
{
  plShaderProgramData()
  {
    m_sPlatform = {};
    m_sSourceFile = {};

    for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
    {
      m_bWriteToDisk[stage] = true;
      m_sShaderSource[stage].Clear();
      m_Resources[stage].Clear();
      m_uiSourceHash[stage] = 0;
      m_ByteCode[stage].Clear();
    }
  }

  plBitflags<plShaderCompilerFlags> m_Flags;
  plStringView m_sPlatform;
  plStringView m_sSourceFile;
  plString m_sShaderSource[plGALShaderStage::ENUM_COUNT];
  plHybridArray<plShaderResourceDefinition, 8> m_Resources[plGALShaderStage::ENUM_COUNT];
  plUInt32 m_uiSourceHash[plGALShaderStage::ENUM_COUNT];
  plSharedPtr<plGALShaderByteCode> m_ByteCode[plGALShaderStage::ENUM_COUNT];
  bool m_bWriteToDisk[plGALShaderStage::ENUM_COUNT];
};