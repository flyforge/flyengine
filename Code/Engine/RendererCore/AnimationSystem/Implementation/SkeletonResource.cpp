#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSkeletonResource, 1, plRTTIDefaultAllocator<plSkeletonResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plSkeletonResource);
// clang-format on

plSkeletonResource::plSkeletonResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plSkeletonResource::~plSkeletonResource() = default;

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plSkeletonResource, plSkeletonResourceDescriptor)
{
  m_pDescriptor = PLASMA_DEFAULT_NEW(plSkeletonResourceDescriptor);
  *m_pDescriptor = std::move(descriptor);

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

plResourceLoadDesc plSkeletonResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plSkeletonResource::UpdateContent(plStreamReader* Stream)
{
  PLASMA_LOG_BLOCK("plSkeletonResource::UpdateContent", GetResourceDescription().GetData());

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

  // skip the asset file header at the start of the file
  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = PLASMA_DEFAULT_NEW(plSkeletonResourceDescriptor);
  m_pDescriptor->Deserialize(*Stream).IgnoreResult();

  res.m_State = plResourceState::Loaded;
  return res;
}

void plSkeletonResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plSkeletonResource); // TODO
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plSkeletonResourceDescriptor::plSkeletonResourceDescriptor() = default;
plSkeletonResourceDescriptor::~plSkeletonResourceDescriptor() = default;
plSkeletonResourceDescriptor::plSkeletonResourceDescriptor(plSkeletonResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

void plSkeletonResourceDescriptor::operator=(plSkeletonResourceDescriptor&& rhs)
{
  m_Skeleton = std::move(rhs.m_Skeleton);
  m_Geometry = std::move(rhs.m_Geometry);
}

plUInt64 plSkeletonResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_Geometry.GetHeapMemoryUsage() + m_Skeleton.GetHeapMemoryUsage();
}

plResult plSkeletonResourceDescriptor::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(7);

  m_Skeleton.Save(inout_stream);
  inout_stream << m_RootTransform;
  inout_stream << m_fMaxImpulse;

  const plUInt16 uiNumGeom = static_cast<plUInt16>(m_Geometry.GetCount());
  inout_stream << uiNumGeom;

  for (plUInt32 i = 0; i < uiNumGeom; ++i)
  {
    const auto& geo = m_Geometry[i];

    inout_stream << geo.m_uiAttachedToJoint;
    inout_stream << geo.m_Type;
    inout_stream << geo.m_Transform;

    PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteArray(geo.m_VertexPositions));
    PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteArray(geo.m_TriangleIndices));
  }

  return PLASMA_SUCCESS;
}

plResult plSkeletonResourceDescriptor::Deserialize(plStreamReader& inout_stream)
{
  const plTypeVersion version = inout_stream.ReadVersion(7);

  if (version < 6)
    return PLASMA_FAILURE;

  m_Skeleton.Load(inout_stream);

  inout_stream >> m_RootTransform;

  if (version >= 7)
  {
    inout_stream >> m_fMaxImpulse;
  }

  m_Geometry.Clear();

  plUInt16 uiNumGeom = 0;
  inout_stream >> uiNumGeom;
  m_Geometry.Reserve(uiNumGeom);

  for (plUInt32 i = 0; i < uiNumGeom; ++i)
  {
    auto& geo = m_Geometry.ExpandAndGetRef();

    inout_stream >> geo.m_uiAttachedToJoint;
    inout_stream >> geo.m_Type;
    inout_stream >> geo.m_Transform;

    if (version <= 6)
    {
      plStringBuilder sName;
      plSurfaceResourceHandle hSurface;
      plUInt8 uiCollisionLayer;

      inout_stream >> sName;
      inout_stream >> hSurface;
      inout_stream >> uiCollisionLayer;
    }

    if (version >= 7)
    {
      PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadArray(geo.m_VertexPositions));
      PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadArray(geo.m_TriangleIndices));
    }
  }

  // make sure the geometry is sorted by bones
  // this allows to make the algorithm for creating the bone geometry more efficient
  m_Geometry.Sort([](const plSkeletonResourceGeometry& lhs, const plSkeletonResourceGeometry& rhs) -> bool { return lhs.m_uiAttachedToJoint < rhs.m_uiAttachedToJoint; });

  return PLASMA_SUCCESS;
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonResource);
