#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <Recast/DetourNavMesh.h>
#include <Recast/Recast.h>
#include <Recast/RecastAlloc.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRecastNavMeshResource, 1, plRTTIDefaultAllocator<plRecastNavMeshResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plRecastNavMeshResource);
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
  PLASMA_DEFAULT_DELETE(m_pNavMeshPolygons);
}

//////////////////////////////////////////////////////////////////////////

plResult plRecastNavMeshResourceDescriptor::Serialize(plStreamWriter& stream) const
{
  stream.WriteVersion(1);
  PLASMA_SUCCEED_OR_RETURN(stream.WriteArray(m_DetourNavmeshData));

  const bool hasPolygons = m_pNavMeshPolygons != nullptr;
  stream << hasPolygons;

  if (hasPolygons)
  {
    PLASMA_CHECK_AT_COMPILETIME_MSG(sizeof(rcPolyMesh) == sizeof(void*) * 5 + sizeof(int) * 14, "rcPolyMesh data structure has changed");

    const auto& mesh = *m_pNavMeshPolygons;

    stream << (int)mesh.nverts;
    stream << (int)mesh.npolys;
    stream << (int)mesh.npolys; // do not use mesh.maxpolys
    stream << (int)mesh.nvp;
    stream << (float)mesh.bmin[0];
    stream << (float)mesh.bmin[1];
    stream << (float)mesh.bmin[2];
    stream << (float)mesh.bmax[0];
    stream << (float)mesh.bmax[1];
    stream << (float)mesh.bmax[2];
    stream << (float)mesh.cs;
    stream << (float)mesh.ch;
    stream << (int)mesh.borderSize;
    stream << (float)mesh.maxEdgeError;

    PLASMA_ASSERT_DEBUG(mesh.maxpolys >= mesh.npolys, "Invalid navmesh polygon count");

    PLASMA_SUCCEED_OR_RETURN(stream.WriteBytes(mesh.verts, sizeof(plUInt16) * mesh.nverts * 3));
    PLASMA_SUCCEED_OR_RETURN(stream.WriteBytes(mesh.polys, sizeof(plUInt16) * mesh.npolys * mesh.nvp * 2));
    PLASMA_SUCCEED_OR_RETURN(stream.WriteBytes(mesh.regs, sizeof(plUInt16) * mesh.npolys));
    PLASMA_SUCCEED_OR_RETURN(stream.WriteBytes(mesh.flags, sizeof(plUInt16) * mesh.npolys));
    PLASMA_SUCCEED_OR_RETURN(stream.WriteBytes(mesh.areas, sizeof(plUInt8) * mesh.npolys));
  }

  return PLASMA_SUCCESS;
}

plResult plRecastNavMeshResourceDescriptor::Deserialize(plStreamReader& stream)
{
  Clear();

  const plTypeVersion version = stream.ReadVersion(1);
  PLASMA_SUCCEED_OR_RETURN(stream.ReadArray(m_DetourNavmeshData));

  bool hasPolygons = false;
  stream >> hasPolygons;

  if (hasPolygons)
  {
    PLASMA_CHECK_AT_COMPILETIME_MSG(sizeof(rcPolyMesh) == sizeof(void*) * 5 + sizeof(int) * 14, "rcPolyMesh data structure has changed");

    m_pNavMeshPolygons = PLASMA_DEFAULT_NEW(rcPolyMesh);

    auto& mesh = *m_pNavMeshPolygons;

    stream >> mesh.nverts;
    stream >> mesh.npolys;
    stream >> mesh.maxpolys;
    stream >> mesh.nvp;
    stream >> mesh.bmin[0];
    stream >> mesh.bmin[1];
    stream >> mesh.bmin[2];
    stream >> mesh.bmax[0];
    stream >> mesh.bmax[1];
    stream >> mesh.bmax[2];
    stream >> mesh.cs;
    stream >> mesh.ch;
    stream >> mesh.borderSize;
    stream >> mesh.maxEdgeError;

    PLASMA_ASSERT_DEBUG(mesh.maxpolys >= mesh.npolys, "Invalid navmesh polygon count");

    mesh.verts = (plUInt16*)rcAlloc(sizeof(plUInt16) * mesh.nverts * 3, RC_ALLOC_PERM);
    mesh.polys = (plUInt16*)rcAlloc(sizeof(plUInt16) * mesh.maxpolys * mesh.nvp * 2, RC_ALLOC_PERM);
    mesh.regs = (plUInt16*)rcAlloc(sizeof(plUInt16) * mesh.maxpolys, RC_ALLOC_PERM);
    mesh.areas = (plUInt8*)rcAlloc(sizeof(plUInt8) * mesh.maxpolys, RC_ALLOC_PERM);

    stream.ReadBytes(mesh.verts, sizeof(plUInt16) * mesh.nverts * 3);
    stream.ReadBytes(mesh.polys, sizeof(plUInt16) * mesh.maxpolys * mesh.nvp * 2);
    stream.ReadBytes(mesh.regs, sizeof(plUInt16) * mesh.maxpolys);
    stream.ReadBytes(mesh.flags, sizeof(plUInt16) * mesh.maxpolys);
    stream.ReadBytes(mesh.areas, sizeof(plUInt8) * mesh.maxpolys);
  }

  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

plRecastNavMeshResource::plRecastNavMeshResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(plRecastNavMeshResource);
}

plRecastNavMeshResource::~plRecastNavMeshResource()
{
  PLASMA_DEFAULT_DELETE(m_pNavMeshPolygons);
  PLASMA_DEFAULT_DELETE(m_pNavMesh);
}

plResourceLoadDesc plRecastNavMeshResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  m_DetourNavmeshData.Clear();
  PLASMA_DEFAULT_DELETE(m_pNavMesh);
  PLASMA_DEFAULT_DELETE(m_pNavMeshPolygons);

  return res;
}

plResourceLoadDesc plRecastNavMeshResource::UpdateContent(plStreamReader* Stream)
{
  PLASMA_LOG_BLOCK("plRecastNavMeshResource::UpdateContent", GetResourceDescription().GetData());

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
    m_pNavMesh = PLASMA_DEFAULT_NEW(dtNavMesh);

    // the dtNavMesh does not need to free the data, the resource owns it
    const int dtMeshFlags = 0;
    m_pNavMesh->init(m_DetourNavmeshData.GetData(), m_DetourNavmeshData.GetCount(), dtMeshFlags);
  }

  return res;
}
