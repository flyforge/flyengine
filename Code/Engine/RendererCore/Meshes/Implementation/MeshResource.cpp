#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshResource, 1, plRTTIDefaultAllocator<plMeshResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plMeshResource);
// clang-format on

plUInt32 plMeshResource::s_uiMeshBufferNameSuffix = 0;

plMeshResource::plMeshResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
  m_Bounds = plBoundingBoxSphere::MakeInvalid();
}

plResourceLoadDesc plMeshResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire mesh
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    m_SubMeshes.Clear();
    m_SubMeshes.Compact();
    m_Materials.Clear();
    m_Materials.Compact();
    m_Bones.Clear();
    m_Bones.Compact();

    m_hMeshBuffer.Invalidate();
    m_hDefaultSkeleton.Invalidate();

    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = plResourceState::Unloaded;
  }

  return res;
}

plResourceLoadDesc plMeshResource::UpdateContent(plStreamReader* Stream)
{
  plMeshResourceDescriptor desc;
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(desc));
}

void plMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plMeshResource) + (plUInt32)m_SubMeshes.GetHeapMemoryUsage() + (plUInt32)m_Materials.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plMeshResource, plMeshResourceDescriptor)
{
  // if there is an existing mesh buffer to use, take that
  m_hMeshBuffer = descriptor.GetExistingMeshBuffer();

  m_hDefaultSkeleton = descriptor.m_hDefaultSkeleton;
  m_Bones = descriptor.m_Bones;
  m_fMaxBoneVertexOffset = descriptor.m_fMaxBoneVertexOffset;

  // otherwise create a new mesh buffer from the descriptor
  if (!m_hMeshBuffer.IsValid())
  {
    s_uiMeshBufferNameSuffix++;
    plStringBuilder sMbName;
    sMbName.Format("{0}  [MeshBuffer {1}]", GetResourceID(), plArgU(s_uiMeshBufferNameSuffix, 4, true, 16, true));

    // note: this gets move'd, might be invalid afterwards
    plMeshBufferResourceDescriptor& mb = descriptor.MeshBufferDesc();

    m_hMeshBuffer = plResourceManager::CreateResource<plMeshBufferResource>(sMbName, std::move(mb), GetResourceDescription());
  }

  m_SubMeshes = descriptor.GetSubMeshes();

  m_Materials.Clear();
  m_Materials.Reserve(descriptor.GetMaterials().GetCount());

  // copy all the material assignments and load the materials
  for (const auto& mat : descriptor.GetMaterials())
  {
    plMaterialResourceHandle hMat;

    if (!mat.m_sPath.IsEmpty())
      hMat = plResourceManager::LoadResource<plMaterialResource>(mat.m_sPath);

    m_Materials.PushBack(hMat); // may be an invalid handle
  }

  m_Bounds = descriptor.GetBounds();
  PLASMA_ASSERT_DEV(m_Bounds.IsValid(), "The mesh bounds are invalid. Make sure to call plMeshResourceDescriptor::ComputeBounds()");

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshResource);
