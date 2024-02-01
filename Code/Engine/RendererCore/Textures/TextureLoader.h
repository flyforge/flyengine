#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <Texture/Image/Image.h>
#include <Texture/plTexFormat/plTexFormat.h>

class PL_RENDERERCORE_DLL plTextureResourceLoader : public plResourceTypeLoader
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
    plImage m_Image;

    bool m_bIsFallback = false;
    plTexFormat m_TexFormat;
  };

  virtual plResourceLoadData OpenDataStream(const plResource* pResource) override;
  virtual void CloseDataStream(const plResource* pResource, const plResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const plResource* pResource) const override;

  static plResult LoadTexFile(plStreamReader& inout_stream, LoadedData& ref_data);
  static void WriteTextureLoadStream(plStreamWriter& inout_stream, const LoadedData& data);
};
