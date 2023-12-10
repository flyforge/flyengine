#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <Texture/Image/Formats/DdsFileFormat.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

void plMaterialResourceDescriptor::Clear()
{
  m_hBaseMaterial.Invalidate();
  m_sSurface.Clear();
  m_hShader.Invalidate();
  m_PermutationVars.Clear();
  m_Parameters.Clear();
  m_Texture2DBindings.Clear();
  m_TextureCubeBindings.Clear();
  m_RenderDataCategory = plInvalidRenderDataCategory;
}

bool plMaterialResourceDescriptor::operator==(const plMaterialResourceDescriptor& other) const
{
  return m_hBaseMaterial == other.m_hBaseMaterial &&
         m_hShader == other.m_hShader &&
         m_PermutationVars == other.m_PermutationVars &&
         m_Parameters == other.m_Parameters &&
         m_Texture2DBindings == other.m_Texture2DBindings &&
         m_Texture3DBindings == other.m_Texture3DBindings &&
         m_TextureCubeBindings == other.m_TextureCubeBindings &&
         m_RenderDataCategory == other.m_RenderDataCategory;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMaterialResource, 1, plRTTIDefaultAllocator<plMaterialResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plMaterialResource);
// clang-format on

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, MaterialResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    plMaterialResource::ClearCache();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plDeque<plMaterialResource::CachedValues> plMaterialResource::s_CachedValues;

plMaterialResource::plMaterialResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
  m_iLastUpdated = 0;
  m_iLastConstantsUpdated = 0;
  m_uiCacheIndex = plInvalidIndex;
  m_pCachedValues = nullptr;

  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plMaterialResource::OnResourceEvent, this));
}

plMaterialResource::~plMaterialResource()
{
  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plMaterialResource::OnResourceEvent, this));
}

plHashedString plMaterialResource::GetPermutationValue(const plTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  plHashedString sResult;
  pCachedValues->m_PermutationVars.TryGetValue(sName, sResult);

  return sResult;
}

plHashedString plMaterialResource::GetSurface() const
{
  if (!m_mDesc.m_sSurface.IsEmpty())
    return m_mDesc.m_sSurface;

  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    plResourceLock<plMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, plResourceAcquireMode::BlockTillLoaded);
    return pBaseMaterial->GetSurface();
  }

  return plHashedString();
}

void plMaterialResource::SetParameter(const plHashedString& sName, const plVariant& value)
{
  plUInt32 uiIndex = plInvalidIndex;
  for (plUInt32 i = 0; i < m_mDesc.m_Parameters.GetCount(); ++i)
  {
    if (m_mDesc.m_Parameters[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != plInvalidIndex)
    {
      if (m_mDesc.m_Parameters[uiIndex].m_Value == value)
      {
        return;
      }

      m_mDesc.m_Parameters[uiIndex].m_Value = value;
    }
    else
    {
      auto& param = m_mDesc.m_Parameters.ExpandAndGetRef();
      param.m_Name = sName;
      param.m_Value = value;
    }
  }
  else
  {
    if (uiIndex == plInvalidIndex)
    {
      return;
    }

    m_mDesc.m_Parameters.RemoveAtAndSwap(uiIndex);
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void plMaterialResource::SetParameter(const char* szName, const plVariant& value)
{
  plTempHashedString sName(szName);

  plUInt32 uiIndex = plInvalidIndex;
  for (plUInt32 i = 0; i < m_mDesc.m_Parameters.GetCount(); ++i)
  {
    if (m_mDesc.m_Parameters[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != plInvalidIndex)
    {
      if (m_mDesc.m_Parameters[uiIndex].m_Value == value)
      {
        return;
      }

      m_mDesc.m_Parameters[uiIndex].m_Value = value;
    }
    else
    {
      auto& param = m_mDesc.m_Parameters.ExpandAndGetRef();
      param.m_Name.Assign(szName);
      param.m_Value = value;
    }
  }
  else
  {
    if (uiIndex == plInvalidIndex)
    {
      return;
    }

    m_mDesc.m_Parameters.RemoveAtAndSwap(uiIndex);
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

plVariant plMaterialResource::GetParameter(const plTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  plVariant value;
  pCachedValues->m_Parameters.TryGetValue(sName, value);

  return value;
}

void plMaterialResource::SetTexture2DBinding(const plHashedString& sName, const plTexture2DResourceHandle& value)
{
  plUInt32 uiIndex = plInvalidIndex;
  for (plUInt32 i = 0; i < m_mDesc.m_Texture2DBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_Texture2DBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
      binding.m_Name = sName;
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void plMaterialResource::SetTexture2DBinding(const char* szName, const plTexture2DResourceHandle& value)
{
  plTempHashedString sName(szName);

  plUInt32 uiIndex = plInvalidIndex;
  for (plUInt32 i = 0; i < m_mDesc.m_Texture2DBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_Texture2DBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
      binding.m_Name.Assign(szName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

plTexture2DResourceHandle plMaterialResource::GetTexture2DBinding(const plTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  // Use pointer to prevent ref counting
  plTexture2DResourceHandle* pBinding;
  if (pCachedValues->m_Texture2DBindings.TryGetValue(sName, pBinding))
  {
    return *pBinding;
  }

  return plTexture2DResourceHandle();
}


void plMaterialResource::SetTextureCubeBinding(const plHashedString& sName, const plTextureCubeResourceHandle& value)
{
  plUInt32 uiIndex = plInvalidIndex;
  for (plUInt32 i = 0; i < m_mDesc.m_TextureCubeBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_TextureCubeBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
      binding.m_Name = sName;
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void plMaterialResource::SetTextureCubeBinding(const char* szName, const plTextureCubeResourceHandle& value)
{
  plTempHashedString sName(szName);

  plUInt32 uiIndex = plInvalidIndex;
  for (plUInt32 i = 0; i < m_mDesc.m_TextureCubeBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_TextureCubeBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
      binding.m_Name.Assign(szName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

plTextureCubeResourceHandle plMaterialResource::GetTextureCubeBinding(const plTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  // Use pointer to prevent ref counting
  plTextureCubeResourceHandle* pBinding;
  if (pCachedValues->m_TextureCubeBindings.TryGetValue(sName, pBinding))
  {
    return *pBinding;
  }

  return plTextureCubeResourceHandle();
}

void plMaterialResource::SetTexture3DBinding(const plHashedString& sName, const plTexture3DResourceHandle& value)
{
  plUInt32 uiIndex = plInvalidIndex;
  for (plUInt32 i = 0; i < m_mDesc.m_Texture3DBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_Texture3DBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_Texture3DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_Texture3DBindings.ExpandAndGetRef();
      binding.m_Name = sName;
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_Texture3DBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void plMaterialResource::SetTexture3DBinding(const char* szName, const plTexture3DResourceHandle& value)
{
  plTempHashedString sName(szName);

  plUInt32 uiIndex = plInvalidIndex;
  for (plUInt32 i = 0; i < m_mDesc.m_Texture3DBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_Texture3DBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_Texture3DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_Texture3DBindings.ExpandAndGetRef();
      binding.m_Name.Assign(szName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != plInvalidIndex)
    {
      m_mDesc.m_Texture3DBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

plTexture3DResourceHandle plMaterialResource::GetTexture3DBinding(const plTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  // Use pointer to prevent ref counting
  plTexture3DResourceHandle* pBinding;
  if (pCachedValues->m_Texture3DBindings.TryGetValue(sName, pBinding))
  {
    return *pBinding;
  }

  return plTexture3DResourceHandle();
}

plRenderData::Category plMaterialResource::GetRenderDataCategory()
{
  auto pCachedValues = GetOrUpdateCachedValues();
  return pCachedValues->m_RenderDataCategory;
}

void plMaterialResource::PreserveCurrentDesc()
{
  m_mOriginalDesc = m_mDesc;
}

void plMaterialResource::ResetResource()
{
  if (m_mDesc != m_mOriginalDesc)
  {
    m_mDesc = m_mOriginalDesc;

    m_iLastModified.Increment();
    m_iLastConstantsModified.Increment();

    m_ModifiedEvent.Broadcast(this);
  }
}

const char* plMaterialResource::GetDefaultMaterialFileName(DefaultMaterialType materialType)
{
  switch (materialType)
  {
    case DefaultMaterialType::Fullbright:
      return "Base/Materials/BaseMaterials/Fullbright.plMaterialAsset";
    case DefaultMaterialType::FullbrightAlphaTest:
      return "Base/Materials/BaseMaterials/FullbrightAlphaTest.plMaterialAsset";
    case DefaultMaterialType::Lit:
      return "Base/Materials/BaseMaterials/Lit.plMaterialAsset";
    case DefaultMaterialType::LitAlphaTest:
      return "Base/Materials/BaseMaterials/LitAlphaTest.plMaterialAsset";
    case DefaultMaterialType::Sky:
      return "Base/Materials/BaseMaterials/Sky.plMaterialAsset";
    case DefaultMaterialType::MissingMaterial:
      return "Base/Materials/Common/MissingMaterial.plMaterialAsset";
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      return "";
  }
}

plResourceLoadDesc plMaterialResource::UnloadData(Unload WhatToUnload)
{
  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    plResourceLock<plMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, plResourceAcquireMode::PointerOnly);

    auto d = plMakeDelegate(&plMaterialResource::OnBaseMaterialModified, this);
    if (pBaseMaterial->m_ModifiedEvent.HasEventHandler(d))
    {
      pBaseMaterial->m_ModifiedEvent.RemoveEventHandler(d);
    }
  }

  m_mDesc.Clear();
  m_mOriginalDesc.Clear();

  if (!m_hConstantBufferStorage.IsInvalidated())
  {
    plRenderContext::DeleteConstantBufferStorage(m_hConstantBufferStorage);
    m_hConstantBufferStorage.Invalidate();
  }

  DeallocateCache(m_uiCacheIndex);
  m_uiCacheIndex = plInvalidIndex;
  m_pCachedValues = nullptr;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plMaterialResource::UpdateContent(plStreamReader* pOuterStream)
{
  m_mDesc.Clear();
  m_mOriginalDesc.Clear();

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  if (pOuterStream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  plStringBuilder sAbsFilePath;
  (*pOuterStream) >> sAbsFilePath;

  if (sAbsFilePath.HasExtension("plMaterialBin"))
  {
    plStringBuilder sTemp, sTemp2;

    plAssetFileHeader AssetHash;
    AssetHash.Read(*pOuterStream).IgnoreResult();

    plUInt8 uiVersion = 0;
    (*pOuterStream) >> uiVersion;
    PLASMA_ASSERT_DEV(uiVersion >= 4 && uiVersion <= 8, "Unknown plMaterialBin version {0}", uiVersion);

    plUInt8 uiCompressionMode = 0;
    if (uiVersion >= 6)
    {
      *pOuterStream >> uiCompressionMode;
    }

    plStreamReader* pInnerStream = pOuterStream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    plCompressedStreamReaderZstd decompressorZstd;
#endif

    switch (uiCompressionMode)
    {
      case 0:
        break;

      case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        decompressorZstd.SetInputStream(pOuterStream);
        pInnerStream = &decompressorZstd;
        break;
#else
        plLog::Error("Material resource is compressed with zstandard, but support for this compressor is not compiled in.");
        res.m_State = plResourceState::LoadedResourceMissing;
        return res;
#endif

      default:
        plLog::Error("Material resource is compressed with an unknown algorithm.");
        res.m_State = plResourceState::LoadedResourceMissing;
        return res;
    }

    plStreamReader& s = *pInnerStream;

    // Base material
    {
      s >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hBaseMaterial = plResourceManager::LoadResource<plMaterialResource>(sTemp);
    }

    // Surface
    {
      s >> sTemp;
      m_mDesc.m_sSurface.Assign(sTemp.GetView());
    }

    // Shader
    {
      s >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hShader = plResourceManager::LoadResource<plShaderResource>(sTemp);
    }

    // Permutation Variables
    {
      plUInt16 uiPermVars;
      s >> uiPermVars;

      m_mDesc.m_PermutationVars.Reserve(uiPermVars);

      for (plUInt16 i = 0; i < uiPermVars; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          AddPermutationVar(sTemp, sTemp2);
        }
      }
    }

    // 2D Textures
    {
      plUInt16 uiTextures = 0;
      s >> uiTextures;

      m_mDesc.m_Texture2DBindings.Reserve(uiTextures);

      for (plUInt16 i = 0; i < uiTextures; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          plMaterialResourceDescriptor::Texture2DBinding& tc = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = plResourceManager::LoadResource<plTexture2DResource>(sTemp2);
        }
      }
    }

    // 3D Textures
    if (uiVersion >= 7)
    {
      plUInt16 uiTextures = 0;
      s >> uiTextures;

      m_mDesc.m_Texture3DBindings.Reserve(uiTextures);

      for (plUInt16 i = 0; i < uiTextures; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          plMaterialResourceDescriptor::Texture3DBinding& tc = m_mDesc.m_Texture3DBindings.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = plResourceManager::LoadResource<plTexture3DResource>(sTemp2);
        }
      }
    }

    // Cube Textures
    {
      plUInt16 uiTextures = 0;
      s >> uiTextures;

      m_mDesc.m_TextureCubeBindings.Reserve(uiTextures);

      for (plUInt16 i = 0; i < uiTextures; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          plMaterialResourceDescriptor::TextureCubeBinding& tc = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = plResourceManager::LoadResource<plTextureCubeResource>(sTemp2);
        }
      }
    }

    // Shader constants
    {
      plUInt16 uiConstants = 0;
      s >> uiConstants;

      m_mDesc.m_Parameters.Reserve(uiConstants);

      plVariant vTemp;

      for (plUInt16 i = 0; i < uiConstants; ++i)
      {
        s >> sTemp;
        s >> vTemp;

        if (!sTemp.IsEmpty() && vTemp.IsValid())
        {
          plMaterialResourceDescriptor::Parameter& tc = m_mDesc.m_Parameters.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = vTemp;
        }
      }
    }

    // Render data category
    if (uiVersion >= 8)
    {
      plStringBuilder sRenderDataCategoryName;
      s >> sRenderDataCategoryName;

      plTempHashedString sCategoryNameHashed(sRenderDataCategoryName.GetView());
      if (sCategoryNameHashed != plTempHashedString("<Invalid>"))
      {
        m_mDesc.m_RenderDataCategory = plRenderData::FindCategory(sCategoryNameHashed);
        if (m_mDesc.m_RenderDataCategory == plInvalidRenderDataCategory)
        {
          plLog::Error("Material '{}' uses an invalid render data category '{}'", GetResourceDescription(), sRenderDataCategoryName);
        }
      }
    }

    if (uiVersion >= 5)
    {
      plStreamReader& s = *pInnerStream;

      plStringBuilder sResourceName;
      s >> sResourceName;

      plTextureResourceLoader::LoadedData embedded;

      while (!sResourceName.IsEmpty())
      {
        plUInt32 dataSize = 0;
        s >> dataSize;

        plTextureResourceLoader::LoadTexFile(s, embedded).IgnoreResult();
        embedded.m_bIsFallback = true;

        plDefaultMemoryStreamStorage storage;
        plMemoryStreamWriter loadStreamWriter(&storage);
        plTextureResourceLoader::WriteTextureLoadStream(loadStreamWriter, embedded);

        plMemoryStreamReader loadStreamReader(&storage);

        plTexture2DResourceHandle hTexture = plResourceManager::LoadResource<plTexture2DResource>(sResourceName);
        plResourceManager::SetResourceLowResData(hTexture, &loadStreamReader);

        s >> sResourceName;
      }
    }
  }

  if (sAbsFilePath.HasExtension("plMaterial"))
  {
    plOpenDdlReader reader;

    if (reader.ParseDocument(*pOuterStream, 0, plLog::GetThreadLocalLogSystem()).Failed())
    {
      res.m_State = plResourceState::LoadedResourceMissing;
      return res;
    }

    const plOpenDdlReaderElement* pRoot = reader.GetRootElement();

    // Read the base material
    if (const plOpenDdlReaderElement* pBase = pRoot->FindChildOfType(plOpenDdlPrimitiveType::String, "BaseMaterial"))
    {
      m_mDesc.m_hBaseMaterial = plResourceManager::LoadResource<plMaterialResource>(pBase->GetPrimitivesString()[0]);
    }

    // Read the shader
    if (const plOpenDdlReaderElement* pShader = pRoot->FindChildOfType(plOpenDdlPrimitiveType::String, "Shader"))
    {
      m_mDesc.m_hShader = plResourceManager::LoadResource<plShaderResource>(pShader->GetPrimitivesString()[0]);
    }

    // Read the render data category
    if (const plOpenDdlReaderElement* pRenderDataCategory = pRoot->FindChildOfType(plOpenDdlPrimitiveType::String, "RenderDataCategory"))
    {
      m_mDesc.m_RenderDataCategory = plRenderData::FindCategory(plTempHashedString(pRenderDataCategory->GetPrimitivesString()[0]));
    }

    for (const plOpenDdlReaderElement* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
    {
      // Read the shader permutation variables
      if (pChild->IsCustomType("Permutation"))
      {
        const plOpenDdlReaderElement* pName = pChild->FindChildOfType(plOpenDdlPrimitiveType::String, "Variable");
        const plOpenDdlReaderElement* pValue = pChild->FindChildOfType(plOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          AddPermutationVar(pName->GetPrimitivesString()[0], pValue->GetPrimitivesString()[0]);
        }
      }

      // Read the shader constants
      if (pChild->IsCustomType("Constant"))
      {
        const plOpenDdlReaderElement* pName = pChild->FindChildOfType(plOpenDdlPrimitiveType::String, "Variable");
        const plOpenDdlReaderElement* pValue = pChild->FindChild("Value");

        plVariant value;
        if (pName && pValue && plOpenDdlUtils::ConvertToVariant(pValue, value).Succeeded())
        {
          plMaterialResourceDescriptor::Parameter& sc = m_mDesc.m_Parameters.ExpandAndGetRef();
          sc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          sc.m_Value = value;
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("Texture2D"))
      {
        const plOpenDdlReaderElement* pName = pChild->FindChildOfType(plOpenDdlPrimitiveType::String, "Variable");
        const plOpenDdlReaderElement* pValue = pChild->FindChildOfType(plOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          plMaterialResourceDescriptor::Texture2DBinding& tc = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
          tc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          tc.m_Value = plResourceManager::LoadResource<plTexture2DResource>(pValue->GetPrimitivesString()[0]);
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("TextureCube"))
      {
        const plOpenDdlReaderElement* pName = pChild->FindChildOfType(plOpenDdlPrimitiveType::String, "Variable");
        const plOpenDdlReaderElement* pValue = pChild->FindChildOfType(plOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          plMaterialResourceDescriptor::TextureCubeBinding& tc = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
          tc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          tc.m_Value = plResourceManager::LoadResource<plTextureCubeResource>(pValue->GetPrimitivesString()[0]);
        }
      }
    }
  }

  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    // Block till the base material has been fully loaded to ensure that all parameters have their final value once this material is loaded.
    plResourceLock<plMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, plResourceAcquireMode::BlockTillLoaded);

    if (!pBaseMaterial->m_ModifiedEvent.HasEventHandler(plMakeDelegate(&plMaterialResource::OnBaseMaterialModified, this)))
    {
      pBaseMaterial->m_ModifiedEvent.AddEventHandler(plMakeDelegate(&plMaterialResource::OnBaseMaterialModified, this));
    }
  }

  m_mOriginalDesc = m_mDesc;

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);

  return res;
}

void plMaterialResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU =
    sizeof(plMaterialResource) + (plUInt32)(m_mDesc.m_PermutationVars.GetHeapMemoryUsage() + m_mDesc.m_Parameters.GetHeapMemoryUsage() + m_mDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mDesc.m_TextureCubeBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_PermutationVars.GetHeapMemoryUsage() +
                                            m_mOriginalDesc.m_Parameters.GetHeapMemoryUsage() + m_mOriginalDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_Texture3DBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_TextureCubeBindings.GetHeapMemoryUsage());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plMaterialResource, plMaterialResourceDescriptor)
{
  m_mDesc = descriptor;
  m_mOriginalDesc = descriptor;

  plResourceLoadDesc res;
  res.m_State = plResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    // Can't block here for the base material since this would result in a deadlock
    plResourceLock<plMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, plResourceAcquireMode::PointerOnly);
    pBaseMaterial->m_ModifiedEvent.AddEventHandler(plMakeDelegate(&plMaterialResource::OnBaseMaterialModified, this));
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  return res;
}

void plMaterialResource::OnBaseMaterialModified(const plMaterialResource* pModifiedMaterial)
{
  PLASMA_ASSERT_DEV(m_mDesc.m_hBaseMaterial == pModifiedMaterial, "Implementation error");

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void plMaterialResource::OnResourceEvent(const plResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != plResourceEvent::Type::ResourceContentUpdated)
    return;

  if (m_pCachedValues != nullptr && m_pCachedValues->m_hShader == resourceEvent.m_pResource)
  {
    m_iLastConstantsModified.Increment();
  }
}

void plMaterialResource::AddPermutationVar(plStringView sName, plStringView sValue)
{
  plHashedString sNameHashed;
  sNameHashed.Assign(sName);
  plHashedString sValueHashed;
  sValueHashed.Assign(sValue);

  if (plShaderManager::IsPermutationValueAllowed(sNameHashed, sValueHashed))
  {
    plPermutationVar& pv = m_mDesc.m_PermutationVars.ExpandAndGetRef();
    pv.m_sName = sNameHashed;
    pv.m_sValue = sValueHashed;
  }
}

bool plMaterialResource::IsModified()
{
  return m_iLastModified != m_iLastUpdated;
}

bool plMaterialResource::AreConstantsModified()
{
  return m_iLastConstantsModified != m_iLastConstantsUpdated;
}

void plMaterialResource::UpdateConstantBuffer(plShaderPermutationResource* pShaderPermutation)
{
  if (pShaderPermutation == nullptr)
    return;

  plTempHashedString sConstantBufferName("plMaterialConstants");
  const plShaderResourceBinding* pBinding = pShaderPermutation->GetShaderStageBinary(plGALShaderStage::PixelShader)->GetByteCode()->GetShaderResourceBinding(sConstantBufferName);
  if (pBinding == nullptr)
  {
    pBinding = pShaderPermutation->GetShaderStageBinary(plGALShaderStage::VertexShader)->GetByteCode()->GetShaderResourceBinding(sConstantBufferName);
  }

  const plShaderConstantBufferLayout* pLayout = pBinding != nullptr ? pBinding->m_pLayout : nullptr;
  if (pLayout == nullptr)
    return;

  auto pCachedValues = GetOrUpdateCachedValues();

  m_iLastConstantsUpdated = m_iLastConstantsModified;

  if (m_hConstantBufferStorage.IsInvalidated())
  {
    m_hConstantBufferStorage = plRenderContext::CreateConstantBufferStorage(pLayout->m_uiTotalSize);
  }

  plConstantBufferStorageBase* pStorage = nullptr;
  if (plRenderContext::TryGetConstantBufferStorage(m_hConstantBufferStorage, pStorage))
  {
    plArrayPtr<plUInt8> data = pStorage->GetRawDataForWriting();
    if (data.GetCount() != pLayout->m_uiTotalSize)
    {
      plRenderContext::DeleteConstantBufferStorage(m_hConstantBufferStorage);
      m_hConstantBufferStorage = plRenderContext::CreateConstantBufferStorage(pLayout->m_uiTotalSize);

      PLASMA_VERIFY(plRenderContext::TryGetConstantBufferStorage(m_hConstantBufferStorage, pStorage), "");
    }

    for (auto& constant : pLayout->m_Constants)
    {
      if (constant.m_uiOffset + plShaderConstant::s_TypeSize[constant.m_Type.GetValue()] <= data.GetCount())
      {
        plUInt8* pDest = &data[constant.m_uiOffset];

        plVariant* pValue = nullptr;
        pCachedValues->m_Parameters.TryGetValue(constant.m_sName, pValue);

        constant.CopyDataFormVariant(pDest, pValue);
      }
    }
  }
}

plMaterialResource::CachedValues* plMaterialResource::GetOrUpdateCachedValues()
{
  if (!IsModified())
  {
    PLASMA_ASSERT_DEV(m_pCachedValues != nullptr, "");
    return m_pCachedValues;
  }

  plHybridArray<plMaterialResource*, 16> materialHierarchy;
  plMaterialResource* pCurrentMaterial = this;

  while (true)
  {
    materialHierarchy.PushBack(pCurrentMaterial);

    const plMaterialResourceHandle& hBaseMaterial = pCurrentMaterial->m_mDesc.m_hBaseMaterial;
    if (!hBaseMaterial.IsValid())
      break;

    // Ensure that the base material is loaded at this point.
    // For loaded materials this will always be the case but is still necessary for runtime created materials.
    pCurrentMaterial = plResourceManager::BeginAcquireResource(hBaseMaterial, plResourceAcquireMode::BlockTillLoaded);
  }

  PLASMA_SCOPE_EXIT(for (plUInt32 i = materialHierarchy.GetCount(); i-- > 1;) {
    plMaterialResource* pMaterial = materialHierarchy[i];
    plResourceManager::EndAcquireResource(pMaterial);

    materialHierarchy[i] = nullptr;
  });

  PLASMA_LOCK(m_UpdateCacheMutex);

  if (!IsModified())
  {
    PLASMA_ASSERT_DEV(m_pCachedValues != nullptr, "");
    return m_pCachedValues;
  }

  m_pCachedValues = AllocateCache(m_uiCacheIndex);

  // set state of parent material first
  for (plUInt32 i = materialHierarchy.GetCount(); i-- > 0;)
  {
    plMaterialResource* pMaterial = materialHierarchy[i];
    const plMaterialResourceDescriptor& desc = pMaterial->m_mDesc;

    if (desc.m_hShader.IsValid())
      m_pCachedValues->m_hShader = desc.m_hShader;

    for (const auto& permutationVar : desc.m_PermutationVars)
    {
      m_pCachedValues->m_PermutationVars.Insert(permutationVar.m_sName, permutationVar.m_sValue);
    }

    for (const auto& param : desc.m_Parameters)
    {
      m_pCachedValues->m_Parameters.Insert(param.m_Name, param.m_Value);
    }

    for (const auto& textureBinding : desc.m_Texture2DBindings)
    {
      m_pCachedValues->m_Texture2DBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    for (const auto& textureBinding : desc.m_TextureCubeBindings)
    {
      m_pCachedValues->m_TextureCubeBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    if (desc.m_RenderDataCategory != plInvalidRenderDataCategory)
    {
      m_pCachedValues->m_RenderDataCategory = desc.m_RenderDataCategory;
    }
  }

  if (m_pCachedValues->m_RenderDataCategory == plInvalidRenderDataCategory)
  {
    plHashedString sBlendModeValue;
    if (m_pCachedValues->m_PermutationVars.TryGetValue("BLEND_MODE", sBlendModeValue))
    {
      if (sBlendModeValue == plTempHashedString("BLEND_MODE_OPAQUE"))
      {
        m_pCachedValues->m_RenderDataCategory = plDefaultRenderDataCategories::LitOpaque;
      }
      else if (sBlendModeValue == plTempHashedString("BLEND_MODE_MASKED"))
      {
        m_pCachedValues->m_RenderDataCategory = plDefaultRenderDataCategories::LitMasked;
      }
      else
      {
        m_pCachedValues->m_RenderDataCategory = plDefaultRenderDataCategories::LitTransparent;
      }
    }
    else
    {
      m_pCachedValues->m_RenderDataCategory = plDefaultRenderDataCategories::LitOpaque;
    }
  }

  m_iLastUpdated = m_iLastModified;
  return m_pCachedValues;
}

namespace
{
  static plMutex s_MaterialCacheMutex;

  struct FreeCacheEntry
  {
    PLASMA_DECLARE_POD_TYPE();

    plUInt32 m_uiIndex;
    plUInt64 m_uiFrame;
  };

  static plDynamicArray<FreeCacheEntry, plStaticAllocatorWrapper> s_FreeMaterialCacheEntries;
} // namespace

void plMaterialResource::CachedValues::Reset()
{
  m_hShader.Invalidate();
  m_PermutationVars.Clear();
  m_Parameters.Clear();
  m_Texture2DBindings.Clear();
  m_TextureCubeBindings.Clear();
  m_RenderDataCategory = plInvalidRenderDataCategory;
}

// static
plMaterialResource::CachedValues* plMaterialResource::AllocateCache(plUInt32& inout_uiCacheIndex)
{
  PLASMA_LOCK(s_MaterialCacheMutex);

  plUInt32 uiOldCacheIndex = inout_uiCacheIndex;

  plUInt64 uiCurrentFrame = plRenderWorld::GetFrameCounter();
  if (!s_FreeMaterialCacheEntries.IsEmpty() && s_FreeMaterialCacheEntries[0].m_uiFrame < uiCurrentFrame)
  {
    inout_uiCacheIndex = s_FreeMaterialCacheEntries[0].m_uiIndex;
    s_FreeMaterialCacheEntries.RemoveAtAndCopy(0);
  }
  else
  {
    inout_uiCacheIndex = s_CachedValues.GetCount();
    s_CachedValues.ExpandAndGetRef();
  }

  DeallocateCache(uiOldCacheIndex);

  return &s_CachedValues[inout_uiCacheIndex];
}

// static
void plMaterialResource::DeallocateCache(plUInt32 uiCacheIndex)
{
  if (uiCacheIndex != plInvalidIndex)
  {
    PLASMA_LOCK(s_MaterialCacheMutex);

    if (uiCacheIndex < s_CachedValues.GetCount())
    {
      s_CachedValues[uiCacheIndex].Reset();

      auto& freeEntry = s_FreeMaterialCacheEntries.ExpandAndGetRef();
      freeEntry.m_uiIndex = uiCacheIndex;
      freeEntry.m_uiFrame = plRenderWorld::GetFrameCounter();
    }
  }
}

// static
void plMaterialResource::ClearCache()
{
  PLASMA_LOCK(s_MaterialCacheMutex);

  s_CachedValues.Clear();
  s_FreeMaterialCacheEntries.Clear();
}

const plMaterialResourceDescriptor& plMaterialResource::GetCurrentDesc() const
{
  return m_mDesc;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Material_Implementation_MaterialResource);
