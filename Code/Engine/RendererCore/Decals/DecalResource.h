#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <RendererCore/RendererCoreDLL.h>

using plDecalResourceHandle = plTypedResourceHandle<class plDecalResource>;

struct plDecalResourceDescriptor
{
};

class PLASMA_RENDERERCORE_DLL plDecalResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDecalResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plDecalResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plDecalResource, plDecalResourceDescriptor);

public:
  plDecalResource();

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
};

class PLASMA_RENDERERCORE_DLL plDecalResourceLoader : public plResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)
    {
    }

    plContiguousMemoryStreamStorage m_Storage;
    plMemoryStreamReader m_Reader;
  };

  virtual plResourceLoadData OpenDataStream(const plResource* pResource) override;
  virtual void CloseDataStream(const plResource* pResource, const plResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const plResource* pResource) const override;
};
