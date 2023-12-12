#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <RendererCore/RendererCoreDLL.h>

class plRenderPipeline;
struct plRenderPipelineResourceDescriptor;

struct PLASMA_RENDERERCORE_DLL plRenderPipelineResourceLoader
{
  static plInternal::NewInstance<plRenderPipeline> CreateRenderPipeline(const plRenderPipelineResourceDescriptor& desc);
  static void CreateRenderPipelineResourceDescriptor(const plRenderPipeline* pPipeline, plRenderPipelineResourceDescriptor& desc);
};

class PLASMA_RENDERERCORE_DLL plRenderPipelineRttiConverterContext : public plRttiConverterContext
{
public:
  plRenderPipelineRttiConverterContext()
    : m_pRenderPipeline(nullptr)
  {
  }

  virtual void Clear() override;

  virtual plInternal::NewInstance<void> CreateObject(const plUuid& guid, const plRTTI* pRtti) override;
  virtual void DeleteObject(const plUuid& guid) override;

  plRenderPipeline* m_pRenderPipeline;
};
