#pragma once

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

// \brief Flags that affect the compilation process of a shader
struct plShaderCompilerFlags
{
  using StorageType = plUInt8;
  enum Enum
  {
    Debug = PLASMA_BIT(0),
    Default = 0,
  };

  struct Bits
  {
    StorageType Debug : 1;
  };
};
PLASMA_DECLARE_FLAGS_OPERATORS(plShaderCompilerFlags);

class PLASMA_RENDERERCORE_DLL plShaderProgramCompiler : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plShaderProgramCompiler, plReflectedClass);

public:
  struct plShaderProgramData
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
    plHybridArray<plShaderParser::ResourceDefinition, 8> m_Resources[plGALShaderStage::ENUM_COUNT];
    plUInt32 m_uiSourceHash[plGALShaderStage::ENUM_COUNT];
    plSharedPtr<plGALShaderByteCode> m_ByteCode[plGALShaderStage::ENUM_COUNT];
    bool m_bWriteToDisk[plGALShaderStage::ENUM_COUNT];
  };

  virtual void GetSupportedPlatforms(plHybridArray<plString, 4>& ref_platforms) = 0;

  virtual plResult Compile(plShaderProgramData& inout_data, plLogInterface* pLog) = 0;

  struct ShaderResourceMapping
  {
    plString m_sName;
    plUInt16 m_uiSlot;
  };
  virtual plResult DefineShaderResourceBindings(plShaderProgramData& inout_data, plHashTable<plHashedString, plShaderResourceBinding>& ref_resourceBinding, plLogInterface* pLog) = 0;
  virtual void CreateShaderResourceDeclaration(plStringView sDeclaration, const plShaderResourceBinding& binding, plStringBuilder& out_sDeclaration) = 0;
};

class PLASMA_RENDERERCORE_DLL plShaderCompiler
{
public:
  plResult CompileShaderPermutationForPlatforms(plStringView sFile, const plArrayPtr<const plPermutationVar>& permutationVars, plLogInterface* pLog, plStringView sPlatform = "ALL");

private:
  plResult RunShaderCompiler(plStringView sFile, plStringView sPlatform, plShaderProgramCompiler* pCompiler, plLogInterface* pLog);
  static plResult MergeShaderResourceBindings(const plShaderProgramCompiler::plShaderProgramData& spd, plHashTable<plHashedString, plShaderResourceBinding>& out_bindings);

  void WriteFailedShaderSource(plShaderProgramCompiler::plShaderProgramData& spd, plLogInterface* pLog);

  bool PassThroughUnknownCommandCB(plStringView sCmd) { return sCmd == "version"; }

  struct plShaderData
  {
    plString m_Platforms;
    plHybridArray<plPermutationVar, 16> m_Permutations;
    plHybridArray<plPermutationVar, 16> m_FixedPermVars;
    plString m_StateSource;
    plString m_ShaderStageSource[plGALShaderStage::ENUM_COUNT];
  };

  plResult FileOpen(plStringView sAbsoluteFile, plDynamicArray<plUInt8>& FileContent, plTimestamp& out_FileModification);

  plStringBuilder m_StageSourceFile[plGALShaderStage::ENUM_COUNT];

  plTokenizedFileCache m_FileCache;
  plShaderData m_ShaderData;

  plSet<plString> m_IncludeFiles;
};
