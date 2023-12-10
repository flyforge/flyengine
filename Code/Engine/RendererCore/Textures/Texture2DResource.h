#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class plImage;

using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;

/// \brief Use this descriptor in calls to plResourceManager::CreateResource<plTexture2DResource> to create textures from data in memory.
struct PLASMA_RENDERERCORE_DLL plTexture2DResourceDescriptor
{
  /// Describes the texture format, etc.
  plGALTextureCreationDescription m_DescGAL;
  plGALSamplerStateCreationDescription m_SamplerDesc;

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  plUInt8 m_uiQualityLevelsDiscardable = 0;

  /// How many additional quality levels can be loaded (typically from file).
  plUInt8 m_uiQualityLevelsLoadable = 0;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not
  /// initialize data.
  plArrayPtr<plGALSystemMemoryDescription> m_InitialContent;
};

class PLASMA_RENDERERCORE_DLL plTexture2DResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTexture2DResource, plResource);

  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plTexture2DResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plTexture2DResource, plTexture2DResourceDescriptor);

public:
  plTexture2DResource();

  PLASMA_ALWAYS_INLINE plGALResourceFormat::Enum GetFormat() const { return m_Format; }
  PLASMA_ALWAYS_INLINE plUInt32 GetWidth() const { return m_uiWidth; }
  PLASMA_ALWAYS_INLINE plUInt32 GetHeight() const { return m_uiHeight; }
  PLASMA_ALWAYS_INLINE plGALTextureType::Enum GetType() const { return m_Type; }

  static void FillOutDescriptor(plTexture2DResourceDescriptor& ref_td, const plImage* pImage, bool bSRGB, plUInt32 uiNumMipLevels,
    plUInt32& out_uiMemoryUsed, plHybridArray<plGALSystemMemoryDescription, 32>& ref_initData);

  const plGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const plGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plTexture2DResource(DoUpdate ResourceUpdateThread);

  plUInt8 m_uiLoadedTextures = 0;
  plGALTextureHandle m_hGALTexture[2];
  plUInt32 m_uiMemoryGPU[2] = {0, 0};

  plGALTextureType::Enum m_Type = plGALTextureType::Invalid;
  plGALResourceFormat::Enum m_Format = plGALResourceFormat::Invalid;
  plUInt32 m_uiWidth = 0;
  plUInt32 m_uiHeight = 0;

  plGALSamplerStateHandle m_hSamplerState;
};

//////////////////////////////////////////////////////////////////////////

using plRenderToTexture2DResourceHandle = plTypedResourceHandle<class plRenderToTexture2DResource>;

struct PLASMA_RENDERERCORE_DLL plRenderToTexture2DResourceDescriptor
{
  plUInt32 m_uiWidth = 0;
  plUInt32 m_uiHeight = 0;
  plEnum<plGALMSAASampleCount> m_SampleCount;
  plEnum<plGALResourceFormat> m_Format;
  plGALSamplerStateCreationDescription m_SamplerDesc;
  plArrayPtr<plGALSystemMemoryDescription> m_InitialContent;
};

class PLASMA_RENDERERCORE_DLL plRenderToTexture2DResource : public plTexture2DResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRenderToTexture2DResource, plTexture2DResource);

  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plRenderToTexture2DResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plRenderToTexture2DResource, plRenderToTexture2DResourceDescriptor);

public:
  plGALRenderTargetViewHandle GetRenderTargetView() const;
  void AddRenderView(plViewHandle hView);
  void RemoveRenderView(plViewHandle hView);
  const plDynamicArray<plViewHandle>& GetAllRenderViews() const;

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

protected:
  // other views that use this texture as their target
  plDynamicArray<plViewHandle> m_RenderViews;
};
