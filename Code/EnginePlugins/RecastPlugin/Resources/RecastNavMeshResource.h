#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RecastPlugin/RecastPluginDLL.h>

struct rcPolyMesh;
class dtNavMesh;

using plRecastNavMeshResourceHandle = plTypedResourceHandle<class plRecastNavMeshResource>;

struct PL_RECASTPLUGIN_DLL plRecastNavMeshResourceDescriptor
{
  plRecastNavMeshResourceDescriptor();
  plRecastNavMeshResourceDescriptor(const plRecastNavMeshResourceDescriptor& rhs) = delete;
  plRecastNavMeshResourceDescriptor(plRecastNavMeshResourceDescriptor&& rhs);
  ~plRecastNavMeshResourceDescriptor();
  void operator=(plRecastNavMeshResourceDescriptor&& rhs);
  void operator=(const plRecastNavMeshResourceDescriptor& rhs) = delete;

  /// \brief Data that was created by dtCreateNavMeshData() and will be used for dtNavMesh::init()
  plDataBuffer m_DetourNavmeshData;

  /// \brief Optional, if available the navmesh can be visualized at runtime
  rcPolyMesh* m_pNavMeshPolygons = nullptr;

  void Clear();

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

class PL_RECASTPLUGIN_DLL plRecastNavMeshResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plRecastNavMeshResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plRecastNavMeshResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plRecastNavMeshResource, plRecastNavMeshResourceDescriptor);

public:
  plRecastNavMeshResource();
  ~plRecastNavMeshResource();

  const dtNavMesh* GetNavMesh() const { return m_pNavMesh; }
  const rcPolyMesh* GetNavMeshPolygons() const { return m_pNavMeshPolygons; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plDataBuffer m_DetourNavmeshData;
  dtNavMesh* m_pNavMesh = nullptr;
  rcPolyMesh* m_pNavMeshPolygons = nullptr;
};
