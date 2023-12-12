#pragma once

#include <Foundation/IO/DependencyFile.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

struct PLASMA_RENDERERCORE_DLL plShaderStateResourceDescriptor
{
  plGALBlendStateCreationDescription m_BlendDesc;
  plGALDepthStencilStateCreationDescription m_DepthStencilDesc;
  plGALRasterizerStateCreationDescription m_RasterizerDesc;

  plResult Parse(const char* szSource);
  void Load(plStreamReader& inout_stream);
  void Save(plStreamWriter& inout_stream) const;

  plUInt32 CalculateHash() const;
};

class PLASMA_RENDERERCORE_DLL plShaderPermutationBinary
{
public:
  plShaderPermutationBinary();

  plResult Write(plStreamWriter& inout_stream);
  plResult Read(plStreamReader& inout_stream, bool& out_bOldVersion);

  plUInt32 m_uiShaderStageHashes[plGALShaderStage::ENUM_COUNT];

  plDependencyFile m_DependencyFile;

  plShaderStateResourceDescriptor m_StateDescriptor;

  plHybridArray<plPermutationVar, 16> m_PermutationVars;
};
