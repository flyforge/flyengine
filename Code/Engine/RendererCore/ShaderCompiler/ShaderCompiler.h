#pragma once

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/Declarations.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

/// \brief Shader compiler interface.
/// Custom shader compiles need to derive from this class and implement the pure virtual interface functions. Instances are created via reflection so each implementation must be properly reflected.
class PL_RENDERERCORE_DLL plShaderProgramCompiler : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plShaderProgramCompiler, plReflectedClass);

public:
  /// \brief Returns the platforms that this shader compiler supports.
  /// \param out_platforms Filled with the platforms this compiler supports.
  virtual void GetSupportedPlatforms(plHybridArray<plString, 4>& out_platforms) = 0;

  /// Allows the shader compiler to modify the shader source before hashing and compiling. This allows it to implement custom features by injecting code before the compile process. Mostly used to define resource bindings that do not cause conflicts across shader stages.
  /// \param inout_data The state of the shader compiler. Only m_sShaderSource should be modified by the implementation.
  /// \param pLog Logging interface to be used when outputting any errors.
  /// \return Returns whether the shader could be modified. On failure, the shader won't be compiled.
  virtual plResult ModifyShaderSource(plShaderProgramData& inout_data, plLogInterface* pLog) = 0;

  /// Compiles the shader comprised of multiple stages defined in inout_data.
  /// \param inout_data The state of the shader compiler. m_Resources and m_ByteCode should be written to on successful return code.
  /// \param pLog Logging interface to be used when outputting any errors.
  /// \return Returns whether the shader was compiled successfully. On failure, errors should be written to pLog.
  virtual plResult Compile(plShaderProgramData& inout_data, plLogInterface* pLog) = 0;
};

class PL_RENDERERCORE_DLL plShaderCompiler
{
public:
  plResult CompileShaderPermutationForPlatforms(plStringView sFile, const plArrayPtr<const plPermutationVar>& permutationVars, plLogInterface* pLog, plStringView sPlatform = "ALL");

private:
  plResult RunShaderCompiler(plStringView sFile, plStringView sPlatform, plShaderProgramCompiler* pCompiler, plLogInterface* pLog);

  void WriteFailedShaderSource(plShaderProgramData& spd, plLogInterface* pLog);

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
