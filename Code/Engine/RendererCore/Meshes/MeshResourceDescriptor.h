#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

class PLASMA_RENDERERCORE_DLL plMeshResourceDescriptor
{
public:
  struct SubMesh
  {
    PLASMA_DECLARE_POD_TYPE();

    plUInt32 m_uiPrimitiveCount;
    plUInt32 m_uiFirstPrimitive;
    plUInt32 m_uiMaterialIndex;

    plBoundingBoxSphere m_Bounds;
  };

  struct Material
  {
    plString m_sPath;
  };

  plMeshResourceDescriptor();

  void Clear();

  plMeshBufferResourceDescriptor& MeshBufferDesc();

  const plMeshBufferResourceDescriptor& MeshBufferDesc() const;

  void UseExistingMeshBuffer(const plMeshBufferResourceHandle& hBuffer);

  void AddSubMesh(plUInt32 uiPrimitiveCount, plUInt32 uiFirstPrimitive, plUInt32 uiMaterialIndex);

  void SetMaterial(plUInt32 uiMaterialIndex, const char* szPathToMaterial);

  void Save(plStreamWriter& inout_stream);
  plResult Save(const char* szFile);

  plResult Load(plStreamReader& inout_stream);
  plResult Load(const char* szFile);

  const plMeshBufferResourceHandle& GetExistingMeshBuffer() const;

  plArrayPtr<const Material> GetMaterials() const;

  plArrayPtr<const SubMesh> GetSubMeshes() const;

  /// \brief Merges all submeshes into just one.
  void CollapseSubMeshes();

  void ComputeBounds();
  const plBoundingBoxSphere& GetBounds() const;
  void SetBounds(const plBoundingBoxSphere& bounds) { m_Bounds = bounds; }

  struct BoneData
  {
    plMat4 m_GlobalInverseRestPoseMatrix;
    plUInt16 m_uiBoneIndex = plInvalidJointIndex;

    plResult Serialize(plStreamWriter& inout_stream) const;
    plResult Deserialize(plStreamReader& inout_stream);
  };

  plSkeletonResourceHandle m_hDefaultSkeleton;
  plHashTable<plHashedString, BoneData> m_Bones;
  float m_fMaxBoneVertexOffset = 0.0f; // the maximum distance between any vertex and its influencing bones, can be used for adjusting the bounding box of a pose

private:
  plHybridArray<Material, 8> m_Materials;
  plHybridArray<SubMesh, 8> m_SubMeshes;
  plMeshBufferResourceDescriptor m_MeshBufferDescriptor;
  plMeshBufferResourceHandle m_hMeshBuffer;
  plBoundingBoxSphere m_Bounds;
};
