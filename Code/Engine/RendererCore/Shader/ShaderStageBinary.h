#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

/// Serializes plGALShaderByteCode and provides access to the shader cache via LoadStageBinary.
class PL_RENDERERCORE_DLL plShaderStageBinary
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
    Version6, // Rewrite, no backwards compatibility. Moves all data into plGALShaderByteCode.
    Version7, // Added tessellation support (m_uiTessellationPatchControlPoints)

    ENUM_COUNT,
    VersionCurrent = ENUM_COUNT - 1
  };

  plShaderStageBinary();
  ~plShaderStageBinary();

  plSharedPtr<const plGALShaderByteCode> GetByteCode() const;

private:
  friend class plRenderContext;
  friend class plShaderCompiler;
  friend class plShaderPermutationResource;
  friend class plShaderPermutationResourceLoader;
   
  plResult WriteStageBinary(plLogInterface* pLog) const;
  plResult Write(plStreamWriter& inout_stream) const;
  plResult Read(plStreamReader& inout_stream);
  plResult Write(plStreamWriter& inout_stream, const plShaderConstantBufferLayout& layout) const;
  plResult Read(plStreamReader& inout_stream, plShaderConstantBufferLayout& out_layout);

private:
  plUInt32 m_uiSourceHash = 0;
  plSharedPtr<plGALShaderByteCode> m_pGALByteCode;

private: // statics
  static plShaderStageBinary* LoadStageBinary(plGALShaderStage::Enum Stage, plUInt32 uiHash);

  static void OnEngineShutdown();

  static plMap<plUInt32, plShaderStageBinary> s_ShaderStageBinaries[plGALShaderStage::ENUM_COUNT];
};
