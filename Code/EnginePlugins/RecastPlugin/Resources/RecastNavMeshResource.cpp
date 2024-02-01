#include <RecastPlugin/RecastPluginPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <DetourNavMesh.h>
#include <Foundation/IO/ChunkStream.h>
#include <Recast.h>
#include <RecastAlloc.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRecastNavMeshResource, 1, plRTTIDefaultAllocator<plRecastNavMeshResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plRecastNavMeshResource);
// clang-format on

//////////////////////////////////////////////////////////////////////////

plRecastNavMeshResourceDescriptor::plRecastNavMeshResourceDescriptor() = default;
plRecastNavMeshResourceDescriptor::plRecastNavMeshResourceDescriptor(plRecastNavMeshResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

plRecastNavMeshResourceDescriptor::~plRecastNavMeshResourceDescriptor()
{
  Clear();
}

void plRecastNavMeshResourceDescriptor::operator=(plRecastNavMeshResourceDescriptor&& rhs)
{
  m_DetourNavmeshData = std::move(rhs.m_DetourNavmeshData);

  m_pNavMeshPolygons = rhs.m_pNavMeshPolygons;
  rhs.m_pNavMeshPolygons = nullptr;
}

void plRecastNavMeshResourceDescriptor::Clear()
{
  m_DetourNavmeshData.Clear();
  PL_DEFAULT_DELETE(m_pNavMeshPolygons);
}

//////////////////////////////////////////////////////////////////////////

plResult plRecastNavMeshResourceDescriptor::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);
  PL_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_DetourNavmeshData));

  const bool hasPolygons = m_pNavMeshPolygons != nullptr;
  inout_stream << hasPolygons;

  if (hasPolygons)
  {
    PL_CHECK_AT_COMPILETIME_MSG(sizeof(rcPolyMesh) == sizeof(void*) * 5 + sizeof(int) * 14, "rcPolyMesh data structure has changed");

    const auto& mesh = *m_pNavMeshPolygons;

    inout_stream << (int)mesh.nverts;
    inout_stream << (int)mesh.npolys;
    inout_stream << (int)mesh.npolys; // do not use mesh.maxpolys
    inout_stream << (int)mesh.nvp;
    inout_stream << (float)mesh.bmin[0];
    inout_stream << (float)mesh.bmin[1];
    inout_stream << (float)mesh.bmin[2];
    inout_stream << (float)mesh.bmax[0];
    inout_stream << (float)mesh.bmax[1];
    inout_stream << (float)mesh.bmax[2];
    inout_stream << (float)mesh.cs;
    inout_stream << (float)mesh.ch;
    inout_stream << (int)mesh.borderSize;
    inout_stream << (float)mesh.maxEdgeError;

    PL_ASSERT_DEBUG(mesh.maxpolys >= mesh.npolys, "Invalid navmesh polygon count");

    PL_SUCCEED_OR_RETURN(inout_stream.WriteBytes(mesh.verts, sizeof(plUInt16) * mesh.nverts * 3));
    PL_SUCCEED_OR_RETURN(inout_stream.WriteBytes(mesh.polys, sizeof(plUInt16) * mesh.npolys * mesh.nvp * 2));
    PL_SUCCEED_OR_RETURN(inout_stream.WriteBytes(mesh.regs, sizeof(plUInt16) * mesh.npolys));
    PL_SUCCEED_OR_RETURN(inout_stream.WriteBytes(mesh.flags, sizeof(plUInt16) * mesh.npolys));
    PL_SUCCEED_OR_RETURN(inout_stream.WriteBytes(mesh.areas, sizeof(plUInt8) * mesh.npolys));
  }

  return PL_SUCCESS;
}

plResult plRecastNavMeshResourceDescriptor::Deserialize(plStreamReader& inout_stream)
{
  Clear();

  const plTypeVersion version = inout_stream.ReadVersion(1);
  PL_IGNORE_UNUSED(version);
  PL_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_DetourNavmeshData));

  bool hasPolygons = false;
  inout_stream >> hasPolygons;

  if (hasPolygons)
  {
    PL_CHECK_AT_COMPILETIME_MSG(sizeof(rcPolyMesh) == sizeof(void*) * 5 + sizeof(int) * 14, "rcPolyMesh data structure has changed");

    m_pNavMeshPolygons = PL_DEFAULT_NEW(rcPolyMesh);

    auto& mesh = *m_pNavMeshPolygons;

    inout_stream >> mesh.nverts;
    inout_stream >> mesh.npolys;
    inout_stream >> mesh.maxpolys;
    inout_stream >> mesh.nvp;
    inout_stream >> mesh.bmin[0];
    inout_stream >> mesh.bmin[1];
    inout_stream >> mesh.bmin[2];
    inout_stream >> mesh.bmax[0];
    inout_stream >> mesh.bmax[1];
    inout_stream >> mesh.bmax[2];
    inout_stream >> mesh.cs;
    inout_stream >> mesh.ch;
    inout_stream >> mesh.borderSize;
    inout_stream >> mesh.maxEdgeError;

    PL_ASSERT_DEBUG(mesh.maxpolys >= mesh.npolys, "Invalid navmesh polygon count");

    mesh.verts = (plUInt16*)rcAlloc(sizeof(plUInt16) * mesh.nverts * 3, RC_ALLOC_PERM);
    mesh.polys = (plUInt16*)rcAlloc(sizeof(plUInt16) * mesh.maxpolys * mesh.nvp * 2, RC_ALLOC_PERM);
    mesh.regs = (plUInt16*)rcAlloc(sizeof(plUInt16) * mesh.maxpolys, RC_ALLOC_PERM);
    mesh.areas = (plUInt8*)rcAlloc(sizeof(plUInt8) * mesh.maxpolys, RC_ALLOC_PERM);

    inout_stream.ReadBytes(mesh.verts, sizeof(plUInt16) * mesh.nverts * 3);
    inout_stream.ReadBytes(mesh.polys, sizeof(plUInt16) * mesh.maxpolys * mesh.nvp * 2);
    inout_stream.ReadBytes(mesh.regs, sizeof(plUInt16) * mesh.maxpolys);
    inout_stream.ReadBytes(mesh.flags, sizeof(plUInt16) * mesh.maxpolys);
    inout_stream.ReadBytes(mesh.areas, sizeof(plUInt8) * mesh.maxpolys);
  }

  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

plRecastNavMeshResource::plRecastNavMeshResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(plRecastNavMeshResource);
}

plRecastNavMeshResource::~plRecastNavMeshResource()
{
  PL_DEFAULT_DELETE(m_pNavMeshPolygons);
  PL_DEFAULT_DELETE(m_pNavMesh);
}

plResourceLoadDesc plRecastNavMeshResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  m_DetourNavmeshData.Clear();
  m_DetourNavmeshData.Compact();
  PL_DEFAULT_DELETE(m_pNavMesh);
  PL_DEFAULT_DELETE(m_pNavMeshPolygons);

  return res;
}

plResourceLoadDesc plRecastNavMeshResource::UpdateContent(plStreamReader* Stream)
{
  PL_LOG_BLOCK("plRecastNavMeshResource::UpdateContent", GetResourceIdOrDescription());

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

  plRecastNavMeshResourceDescriptor descriptor;
  descriptor.Deserialize(*Stream).IgnoreResult();

  return CreateResource(std::move(descriptor));
}

void plRecastNavMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plRecastNavMeshResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_DetourNavmeshData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_pNavMesh != nullptr ? sizeof(dtNavMesh) : 0;
  out_NewMemoryUsage.m_uiMemoryCPU += m_pNavMeshPolygons != nullptr ? sizeof(rcPolyMesh) : 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

plResourceLoadDesc plRecastNavMeshResource::CreateResource(plRecastNavMeshResourceDescriptor&& descriptor)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  m_pNavMeshPolygons = descriptor.m_pNavMeshPolygons;
  descriptor.m_pNavMeshPolygons = nullptr;

  m_DetourNavmeshData = std::move(descriptor.m_DetourNavmeshData);

  if (!m_DetourNavmeshData.IsEmpty())
  {
    m_pNavMesh = PL_DEFAULT_NEW(dtNavMesh);

    // the dtNavMesh does not need to free the data, the resource owns it
    const int dtMeshFlags = 0;
    m_pNavMesh->init(m_DetourNavmeshData.GetData(), m_DetourNavmeshData.GetCount(), dtMeshFlags);
  }

  return res;
}
