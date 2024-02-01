#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <RendererCore/RendererCoreDLL.h>

class plRenderPipeline;
struct plRenderPipelineResourceDescriptor;
class plStreamWriter;
class plRenderPipelinePass;
class plExtractor;

struct PL_RENDERERCORE_DLL plRenderPipelineResourceLoaderConnection
{
  plUInt32 m_uiSource;
  plUInt32 m_uiTarget;
  plString m_sSourcePin;
  plString m_sTargetPin;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};
PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plRenderPipelineResourceLoaderConnection);

struct PL_RENDERERCORE_DLL plRenderPipelineResourceLoader
{
  static plInternal::NewInstance<plRenderPipeline> CreateRenderPipeline(const plRenderPipelineResourceDescriptor& desc);
  static void CreateRenderPipelineResourceDescriptor(const plRenderPipeline* pPipeline, plRenderPipelineResourceDescriptor& ref_desc);
  static plResult ExportPipeline(plArrayPtr<const plRenderPipelinePass* const> passes, plArrayPtr<const plExtractor* const> extractors, plArrayPtr<const plRenderPipelineResourceLoaderConnection> connections, plStreamWriter& ref_streamWriter);
};
