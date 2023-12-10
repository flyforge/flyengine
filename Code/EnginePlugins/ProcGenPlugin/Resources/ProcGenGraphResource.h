#pragma once

#include <Core/ResourceManager/Resource.h>
#include <ProcGenPlugin/Declarations.h>

using plProcGenGraphResourceHandle = plTypedResourceHandle<class plProcGenGraphResource>;

struct PLASMA_PROCGENPLUGIN_DLL plProcGenGraphResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class PLASMA_PROCGENPLUGIN_DLL plProcGenGraphResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGenGraphResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plProcGenGraphResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plProcGenGraphResource, plProcGenGraphResourceDescriptor);

public:
  plProcGenGraphResource();
  ~plProcGenGraphResource();

  const plDynamicArray<plSharedPtr<const plProcGenInternal::PlacementOutput>>& GetPlacementOutputs() const;
  const plDynamicArray<plSharedPtr<const plProcGenInternal::VertexColorOutput>>& GetVertexColorOutputs() const;

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  plDynamicArray<plSharedPtr<const plProcGenInternal::PlacementOutput>> m_PlacementOutputs;
  plDynamicArray<plSharedPtr<const plProcGenInternal::VertexColorOutput>> m_VertexColorOutputs;

  plSharedPtr<plProcGenInternal::GraphSharedDataBase> m_pSharedData;
};
