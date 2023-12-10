#pragma once

#include <RendererCore/Meshes/MeshResourceDescriptor.h>

class PLASMA_RENDERERCORE_DLL plCpuMeshResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCpuMeshResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plCpuMeshResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plCpuMeshResource, plMeshResourceDescriptor);

public:
  plCpuMeshResource();

  const plMeshResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plMeshResourceDescriptor m_Descriptor;
};

using plCpuMeshResourceHandle = plTypedResourceHandle<class plCpuMeshResource>;
