#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plMsgSetMeshMaterial);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgSetMeshMaterial, 1, plRTTIDefaultAllocator<plMsgSetMeshMaterial>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PL_MEMBER_PROPERTY("MaterialSlot", m_uiMaterialSlot),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plMsgSetMeshMaterial::SetMaterialFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = plResourceManager::LoadResource<plMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* plMsgSetMeshMaterial::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void plMsgSetMeshMaterial::Serialize(plStreamWriter& inout_stream) const
{
  // has to be stringyfied for transfer
  inout_stream << GetMaterialFile();
  inout_stream << m_uiMaterialSlot;
}

void plMsgSetMeshMaterial::Deserialize(plStreamReader& inout_stream, plUInt8 uiTypeVersion)
{
  plStringBuilder file;
  inout_stream >> file;
  SetMaterialFile(file);

  inout_stream >> m_uiMaterialSlot;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshRenderData, 1, plRTTIDefaultAllocator<plMeshRenderData>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plMeshComponentBase, 3)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    PL_MESSAGE_HANDLER(plMsgSetColor, OnMsgSetColor),
  } PL_END_MESSAGEHANDLERS;
}
PL_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

plMeshComponentBase::plMeshComponentBase() = default;
plMeshComponentBase::~plMeshComponentBase() = default;

void plMeshComponentBase::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  // ignore components that have created meshes (?)

  s << m_hMesh;

  s << m_Materials.GetCount();
  for (const auto& mat : m_Materials)
  {
    s << mat;
  }

  s << m_Color;
  s << m_fSortingDepthOffset;
}

void plMeshComponentBase::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();

  s >> m_hMesh;

  if (uiVersion < 2)
  {
    plUInt32 uiCategory = 0;
    s >> uiCategory;
  }

  plUInt32 uiMaterials = 0;
  s >> uiMaterials;

  m_Materials.SetCount(uiMaterials);

  for (auto& mat : m_Materials)
  {
    s >> mat;
  }

  s >> m_Color;

  if (uiVersion >= 3)
  {
    s >> m_fSortingDepthOffset;
  }
}

plResult plMeshComponentBase::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  if (m_hMesh.IsValid())
  {
    plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::AllowLoadingFallback);
    ref_bounds = pMesh->GetBounds();
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

void plMeshComponentBase::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::AllowLoadingFallback);
  plArrayPtr<const plMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (plUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const plUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    plMaterialResourceHandle hMaterial;

    // If we have a material override, use that otherwise use the default mesh material.
    if (GetMaterial(uiMaterialIndex).IsValid())
      hMaterial = m_Materials[uiMaterialIndex];
    else
      hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    plMeshRenderData* pRenderData = CreateRenderData();
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform() * pRenderData->m_GlobalTransform;
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_fSortingDepthOffset = m_fSortingDepthOffset;
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_Color = m_Color;
      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
    }

    bool bDontCacheYet = false;

    // Determine render data category.
    plRenderData::Category category = plDefaultRenderDataCategories::LitOpaque;
    if (hMaterial.IsValid())
    {
      plResourceLock<plMaterialResource> pMaterial(hMaterial, plResourceAcquireMode::AllowLoadingFallback);

      if (pMaterial.GetAcquireResult() == plResourceAcquireResult::LoadingFallback)
        bDontCacheYet = true;

      category = pMaterial->GetRenderDataCategory();
    }

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? plRenderData::Caching::Never : plRenderData::Caching::IfStatic);
  }
}

void plMeshComponentBase::SetMesh(const plMeshResourceHandle& hMesh)
{
  if (m_hMesh != hMesh)
  {
    m_hMesh = hMesh;

    TriggerLocalBoundsUpdate();
    InvalidateCachedRenderData();
  }
}

void plMeshComponentBase::SetMaterial(plUInt32 uiIndex, const plMaterialResourceHandle& hMaterial)
{
  m_Materials.EnsureCount(uiIndex + 1);

  if (m_Materials[uiIndex] != hMaterial)
  {
    m_Materials[uiIndex] = hMaterial;

    InvalidateCachedRenderData();
  }
}

plMaterialResourceHandle plMeshComponentBase::GetMaterial(plUInt32 uiIndex) const
{
  if (uiIndex >= m_Materials.GetCount())
    return plMaterialResourceHandle();

  return m_Materials[uiIndex];
}

void plMeshComponentBase::SetMeshFile(const char* szFile)
{
  plMeshResourceHandle hMesh;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = plResourceManager::LoadResource<plMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* plMeshComponentBase::GetMeshFile() const
{
  if (!m_hMesh.IsValid())
    return "";

  return m_hMesh.GetResourceID();
}

void plMeshComponentBase::SetColor(const plColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const plColor& plMeshComponentBase::GetColor() const
{
  return m_Color;
}

void plMeshComponentBase::SetSortingDepthOffset(float fOffset)
{
  m_fSortingDepthOffset = fOffset;

  InvalidateCachedRenderData();
}

float plMeshComponentBase::GetSortingDepthOffset() const
{
  return m_fSortingDepthOffset;
}

void plMeshComponentBase::OnMsgSetMeshMaterial(plMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_uiMaterialSlot, ref_msg.m_hMaterial);
}

void plMeshComponentBase::OnMsgSetColor(plMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

plMeshRenderData* plMeshComponentBase::CreateRenderData() const
{
  return plCreateRenderDataForThisFrame<plMeshRenderData>(GetOwner());
}

plUInt32 plMeshComponentBase::Materials_GetCount() const
{
  return m_Materials.GetCount();
}

const char* plMeshComponentBase::Materials_GetValue(plUInt32 uiIndex) const
{
  auto hMat = GetMaterial(uiIndex);

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}


void plMeshComponentBase::Materials_SetValue(plUInt32 uiIndex, const char* value)
{
  if (plStringUtils::IsNullOrEmpty(value))
    SetMaterial(uiIndex, plMaterialResourceHandle());
  else
  {
    auto hMat = plResourceManager::LoadResource<plMaterialResource>(value);
    SetMaterial(uiIndex, hMat);
  }
}


void plMeshComponentBase::Materials_Insert(plUInt32 uiIndex, const char* value)
{
  plMaterialResourceHandle hMat;

  if (!plStringUtils::IsNullOrEmpty(value))
    hMat = plResourceManager::LoadResource<plMaterialResource>(value);

  m_Materials.Insert(hMat, uiIndex);

  InvalidateCachedRenderData();
}


void plMeshComponentBase::Materials_Remove(plUInt32 uiIndex)
{
  m_Materials.RemoveAtAndCopy(uiIndex);

  InvalidateCachedRenderData();
}



PL_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponentBase);
