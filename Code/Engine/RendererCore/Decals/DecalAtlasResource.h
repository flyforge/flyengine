#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/RendererCoreDLL.h>
#include <Texture/Utils/TextureAtlasDesc.h>

using plDecalAtlasResourceHandle = plTypedResourceHandle<class plDecalAtlasResource>;
using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;

class plImage;

struct plDecalAtlasResourceDescriptor
{
};

class PL_RENDERERCORE_DLL plDecalAtlasResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plDecalAtlasResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plDecalAtlasResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plDecalAtlasResource, plDecalAtlasResourceDescriptor);

public:
  plDecalAtlasResource();

  /// \brief Returns the one global decal atlas resource
  static plDecalAtlasResourceHandle GetDecalAtlasResource();

  const plTexture2DResourceHandle& GetBaseColorTexture() const { return m_hBaseColor; }
  const plTexture2DResourceHandle& GetNormalTexture() const { return m_hNormal; }
  const plTexture2DResourceHandle& GetORMTexture() const { return m_hORM; }
  const plVec2U32& GetBaseColorTextureSize() const { return m_vBaseColorSize; }
  const plVec2U32& GetNormalTextureSize() const { return m_vNormalSize; }
  const plVec2U32& GetORMTextureSize() const { return m_vORMSize; }
  const plTextureAtlasRuntimeDesc& GetAtlas() const { return m_Atlas; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void ReportResourceIsMissing() override;

  void ReadDecalInfo(plStreamReader* Stream);

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void CreateLayerTexture(const plImage& img, bool bSRGB, plTexture2DResourceHandle& out_hTexture);

  plTextureAtlasRuntimeDesc m_Atlas;
  static plUInt32 s_uiDecalAtlasResources;
  plTexture2DResourceHandle m_hBaseColor;
  plTexture2DResourceHandle m_hNormal;
  plTexture2DResourceHandle m_hORM;
  plVec2U32 m_vBaseColorSize;
  plVec2U32 m_vNormalSize;
  plVec2U32 m_vORMSize;
};
