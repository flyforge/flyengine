#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <Texture/Image/Image.h>

using plTextureCubeResourceHandle = plTypedResourceHandle<class plTextureCubeResource>;

/// \brief Use this descriptor in calls to plResourceManager::CreateResource<plTextureCubeResource> to create textures from data in memory.
struct plTextureCubeResourceDescriptor
{
  plTextureCubeResourceDescriptor()
  {
    m_uiQualityLevelsDiscardable = 0;
    m_uiQualityLevelsLoadable = 0;
  }

  /// Describes the texture format, etc.
  plGALTextureCreationDescription m_DescGAL;
  plGALSamplerStateCreationDescription m_SamplerDesc;

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  plUInt8 m_uiQualityLevelsDiscardable;

  /// How many additional quality levels can be loaded (typically from file).
  plUInt8 m_uiQualityLevelsLoadable;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not
  /// initialize data.
  plArrayPtr<plGALSystemMemoryDescription> m_InitialContent;
};

class PLASMA_RENDERERCORE_DLL plTextureCubeResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureCubeResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plTextureCubeResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plTextureCubeResource, plTextureCubeResourceDescriptor);

public:
  plTextureCubeResource();

  PLASMA_ALWAYS_INLINE plGALResourceFormat::Enum GetFormat() const { return m_Format; }
  PLASMA_ALWAYS_INLINE plUInt32 GetWidthAndHeight() const { return m_uiWidthAndHeight; }

  const plGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const plGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plUInt8 m_uiLoadedTextures;
  plGALTextureHandle m_hGALTexture[2];
  plUInt32 m_uiMemoryGPU[2];

  plGALResourceFormat::Enum m_Format;
  plUInt32 m_uiWidthAndHeight;

  plGALSamplerStateHandle m_hSamplerState;
};
