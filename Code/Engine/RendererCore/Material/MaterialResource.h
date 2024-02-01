#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

using plMaterialResourceHandle = plTypedResourceHandle<class plMaterialResource>;
using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;
using plTextureCubeResourceHandle = plTypedResourceHandle<class plTextureCubeResource>;

struct plMaterialResourceDescriptor
{
  struct Parameter
  {
    plHashedString m_Name;
    plVariant m_Value;

    PL_FORCE_INLINE bool operator==(const Parameter& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  struct Texture2DBinding
  {
    plHashedString m_Name;
    plTexture2DResourceHandle m_Value;

    PL_FORCE_INLINE bool operator==(const Texture2DBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  struct TextureCubeBinding
  {
    plHashedString m_Name;
    plTextureCubeResourceHandle m_Value;

    PL_FORCE_INLINE bool operator==(const TextureCubeBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  void Clear();

  bool operator==(const plMaterialResourceDescriptor& other) const;
  PL_FORCE_INLINE bool operator!=(const plMaterialResourceDescriptor& other) const { return !(*this == other); }

  plMaterialResourceHandle m_hBaseMaterial;
  // plSurfaceResource is not linked into this project (not true anymore -> could be changed)
  // this is not used for game purposes but rather for automatic collision mesh generation, so we only store the asset ID here
  plHashedString m_sSurface;
  plShaderResourceHandle m_hShader;
  plDynamicArray<plPermutationVar> m_PermutationVars;
  plDynamicArray<Parameter> m_Parameters;
  plDynamicArray<Texture2DBinding> m_Texture2DBindings;
  plDynamicArray<TextureCubeBinding> m_TextureCubeBindings;
  plRenderData::Category m_RenderDataCategory;
};

class PL_RENDERERCORE_DLL plMaterialResource final : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plMaterialResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plMaterialResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plMaterialResource, plMaterialResourceDescriptor);

public:
  plMaterialResource();
  ~plMaterialResource();

  plHashedString GetPermutationValue(const plTempHashedString& sName);
  plHashedString GetSurface() const;

  void SetParameter(const plHashedString& sName, const plVariant& value);
  void SetParameter(const char* szName, const plVariant& value);
  plVariant GetParameter(const plTempHashedString& sName);

  void SetTexture2DBinding(const plHashedString& sName, const plTexture2DResourceHandle& value);
  void SetTexture2DBinding(const char* szName, const plTexture2DResourceHandle& value);
  plTexture2DResourceHandle GetTexture2DBinding(const plTempHashedString& sName);

  void SetTextureCubeBinding(const plHashedString& sName, const plTextureCubeResourceHandle& value);
  void SetTextureCubeBinding(const char* szName, const plTextureCubeResourceHandle& value);
  plTextureCubeResourceHandle GetTextureCubeBinding(const plTempHashedString& sName);

  plRenderData::Category GetRenderDataCategory();

  /// \brief Copies current desc to original desc so the material is not modified on reset
  void PreserveCurrentDesc();
  virtual void ResetResource() override;

  const plMaterialResourceDescriptor& GetCurrentDesc() const;

  /// \brief Use these enum values together with GetDefaultMaterialFileName() to get the default file names for these material types.
  enum class DefaultMaterialType
  {
    Fullbright,
    FullbrightAlphaTest,
    Lit,
    LitAlphaTest,
    Sky,
    MissingMaterial
  };

  /// \brief Returns the default material file name for the given type (materials in Data/Base/Materials/BaseMaterials).
  static const char* GetDefaultMaterialFileName(DefaultMaterialType materialType);

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  plMaterialResourceDescriptor m_mOriginalDesc; // stores the state at loading, such that SetParameter etc. calls can be reset later
  plMaterialResourceDescriptor m_mDesc;

  friend class plRenderContext;
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, MaterialResource);

  plEvent<const plMaterialResource*, plMutex> m_ModifiedEvent;
  void OnBaseMaterialModified(const plMaterialResource* pModifiedMaterial);
  void OnResourceEvent(const plResourceEvent& resourceEvent);

  void AddPermutationVar(plStringView sName, plStringView sValue);

  plAtomicInteger32 m_iLastModified;
  plAtomicInteger32 m_iLastConstantsModified;
  plInt32 m_iLastUpdated;
  plInt32 m_iLastConstantsUpdated;

  bool IsModified();
  bool AreConstantsModified();

  void UpdateConstantBuffer(plShaderPermutationResource* pShaderPermutation);

  plConstantBufferStorageHandle m_hConstantBufferStorage;

  struct CachedValues
  {
    plShaderResourceHandle m_hShader;
    plHashTable<plHashedString, plHashedString> m_PermutationVars;
    plHashTable<plHashedString, plVariant> m_Parameters;
    plHashTable<plHashedString, plTexture2DResourceHandle> m_Texture2DBindings;
    plHashTable<plHashedString, plTextureCubeResourceHandle> m_TextureCubeBindings;
    plRenderData::Category m_RenderDataCategory;

    void Reset();
  };

  plUInt32 m_uiCacheIndex;
  CachedValues* m_pCachedValues;

  CachedValues* GetOrUpdateCachedValues();
  static CachedValues* AllocateCache(plUInt32& inout_uiCacheIndex);
  static void DeallocateCache(plUInt32 uiCacheIndex);

  plMutex m_UpdateCacheMutex;
  static plDeque<plMaterialResource::CachedValues> s_CachedValues;

  static void ClearCache();
};
