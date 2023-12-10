#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/HashTable.h>
#include <RendererCore/RendererCoreDLL.h>

using plRenderPipelineResourceHandle = plTypedResourceHandle<class plRenderPipelineResource>;
class plRenderPipeline;

struct plRenderPipelineResourceDescriptor
{
  void Clear() {}

  void CreateFromRenderPipeline(const plRenderPipeline* pPipeline);

  plDynamicArray<plUInt8> m_SerializedPipeline;
  plString m_sPath;
};

class PLASMA_RENDERERCORE_DLL plRenderPipelineResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRenderPipelineResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plRenderPipelineResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plRenderPipelineResource, plRenderPipelineResourceDescriptor);

public:
  plRenderPipelineResource();

  PLASMA_ALWAYS_INLINE const plRenderPipelineResourceDescriptor& GetDescriptor() { return m_Desc; }

  plInternal::NewInstance<plRenderPipeline> CreateRenderPipeline() const;

public:
  static plRenderPipelineResourceHandle CreateMissingPipeline();

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  plRenderPipelineResourceDescriptor m_Desc;
};
