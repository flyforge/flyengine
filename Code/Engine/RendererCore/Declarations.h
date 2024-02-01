#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/RendererCoreDLL.h>

class plShaderStageBinary;
struct plVertexDeclarationInfo;

using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;
using plRenderToTexture2DResourceHandle = plTypedResourceHandle<class plRenderToTexture2DResource>;
using plTextureCubeResourceHandle = plTypedResourceHandle<class plTextureCubeResource>;
using plMeshBufferResourceHandle = plTypedResourceHandle<class plMeshBufferResource>;
using plDynamicMeshBufferResourceHandle = plTypedResourceHandle<class plDynamicMeshBufferResource>;
using plMeshResourceHandle = plTypedResourceHandle<class plMeshResource>;
using plMaterialResourceHandle = plTypedResourceHandle<class plMaterialResource>;
using plShaderResourceHandle = plTypedResourceHandle<class plShaderResource>;
using plShaderPermutationResourceHandle = plTypedResourceHandle<class plShaderPermutationResource>;
using plRenderPipelineResourceHandle = plTypedResourceHandle<class plRenderPipelineResource>;
using plDecalResourceHandle = plTypedResourceHandle<class plDecalResource>;
using plDecalAtlasResourceHandle = plTypedResourceHandle<class plDecalAtlasResource>;

struct PL_RENDERERCORE_DLL plPermutationVar
{
  PL_DECLARE_MEM_RELOCATABLE_TYPE();

  plHashedString m_sName;
  plHashedString m_sValue;

  PL_ALWAYS_INLINE bool operator==(const plPermutationVar& other) const { return m_sName == other.m_sName && m_sValue == other.m_sValue; }
};
