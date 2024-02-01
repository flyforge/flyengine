#pragma once

#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

using plMaterialResourceHandle = plTypedResourceHandle<class plMaterialResource>;

class PL_RENDERERCORE_DLL plMeshResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plMeshResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plMeshResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plMeshResource, plMeshResourceDescriptor);

public:
  plMeshResource();

  /// \brief Returns the array of sub-meshes in this mesh.
  plArrayPtr<const plMeshResourceDescriptor::SubMesh> GetSubMeshes() const { return m_SubMeshes; }

  /// \brief Returns the mesh buffer that is used by this resource.
  const plMeshBufferResourceHandle& GetMeshBuffer() const { return m_hMeshBuffer; }

  /// \brief Returns the default materials for this mesh.
  plArrayPtr<const plMaterialResourceHandle> GetMaterials() const { return m_Materials; }

  /// \brief Returns the bounds of this mesh.
  const plBoundingBoxSphere& GetBounds() const { return m_Bounds; }

  // TODO: clean up
  plSkeletonResourceHandle m_hDefaultSkeleton;
  plHashTable<plHashedString, plMeshResourceDescriptor::BoneData> m_Bones;
  float m_fMaxBoneVertexOffset = 0.0f; // the maximum distance between any vertex and its influencing bones, can be used for adjusting the bounding box of a pose

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plDynamicArray<plMeshResourceDescriptor::SubMesh> m_SubMeshes;
  plMeshBufferResourceHandle m_hMeshBuffer;
  plDynamicArray<plMaterialResourceHandle> m_Materials;

  plBoundingBoxSphere m_Bounds;

  static plUInt32 s_uiMeshBufferNameSuffix;
};

using plMeshResourceHandle = plTypedResourceHandle<class plMeshResource>;
