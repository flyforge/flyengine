#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <JoltPlugin/JoltPluginDLL.h>

using plJoltMeshResourceHandle = plTypedResourceHandle<class plJoltMeshResource>;
using plSurfaceResourceHandle = plTypedResourceHandle<class plSurfaceResource>;
using plCpuMeshResourceHandle = plTypedResourceHandle<class plCpuMeshResource>;

struct plMsgExtractGeometry;
class plJoltMaterial;

namespace JPH
{
  class MeshShape;
  class ConvexHullShape;
  class Shape;
} // namespace JPH

struct PLASMA_JOLTPLUGIN_DLL plJoltMeshResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class PLASMA_JOLTPLUGIN_DLL plJoltMeshResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plJoltMeshResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plJoltMeshResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plJoltMeshResource, plJoltMeshResourceDescriptor);

public:
  plJoltMeshResource();
  ~plJoltMeshResource();

  /// \brief Returns the bounds of the collision mesh
  const plBoundingBoxSphere& GetBounds() const { return m_Bounds; }

  /// \brief Returns the array of default surfaces to be used with this mesh.
  ///
  /// Note the array may contain less surfaces than the mesh does. It may also contain invalid surface handles.
  /// Use the default physics material as a fallback.
  const plDynamicArray<plSurfaceResourceHandle>& GetSurfaces() const { return m_Surfaces; }

  /// \brief Returns whether the mesh resource contains a triangle mesh. Triangle meshes and convex meshes are mutually exclusive.
  bool HasTriangleMesh() const { return m_pTriangleMeshInstance != nullptr || !m_TriangleMeshData.IsEmpty(); }

  /// \brief Creates a new instance (shape) of the triangle mesh.
  JPH::Shape* InstantiateTriangleMesh(plUInt64 uiUserData, const plDynamicArray<const plJoltMaterial*>& materials) const;

  /// \brief Returns the number of convex meshes. Triangle meshes and convex meshes are mutually exclusive.
  plUInt32 GetNumConvexParts() const { return !m_ConvexMeshInstances.IsEmpty() ? m_ConvexMeshInstances.GetCount() : m_ConvexMeshesData.GetCount(); }

  /// \brief Creates a new instance (shape) of the triangle mesh.
  JPH::Shape* InstantiateConvexPart(plUInt32 uiPartIdx, plUInt64 uiUserData, const plJoltMaterial* pMaterial, float fDensity) const;

  /// \brief Converts the geometry of the triangle or convex mesh to a CPU mesh resource
  plCpuMeshResourceHandle ConvertToCpuMesh() const;

  plUInt32 GetNumTriangles() const { return m_uiNumTriangles; }
  plUInt32 GetNumVertices() const { return m_uiNumVertices; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  plBoundingBoxSphere m_Bounds;
  plDynamicArray<plSurfaceResourceHandle> m_Surfaces;
  mutable plHybridArray<plDataBuffer*, 1> m_ConvexMeshesData;
  mutable plDataBuffer m_TriangleMeshData;
  mutable JPH::Shape* m_pTriangleMeshInstance = nullptr;
  mutable plHybridArray<JPH::Shape*, 1> m_ConvexMeshInstances;

  plUInt32 m_uiNumVertices = 0;
  plUInt32 m_uiNumTriangles = 0;
};
