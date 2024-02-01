#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <KrautPlugin/KrautDeclarations.h>

using plMeshResourceHandle = plTypedResourceHandle<class plMeshResource>;
using plKrautTreeResourceHandle = plTypedResourceHandle<class plKrautTreeResource>;
using plMaterialResourceHandle = plTypedResourceHandle<class plMaterialResource>;
using plSurfaceResourceHandle = plTypedResourceHandle<class plSurfaceResource>;

struct PL_KRAUTPLUGIN_DLL plKrautTreeResourceDetails
{
  plBoundingBoxSphere m_Bounds;
  plVec3 m_vLeafCenter;
  float m_fStaticColliderRadius;
  plString m_sSurfaceResource;
};

struct PL_KRAUTPLUGIN_DLL plKrautTreeResourceDescriptor
{
  void Save(plStreamWriter& inout_stream) const;
  plResult Load(plStreamReader& inout_stream);

  struct VertexData
  {
    PL_DECLARE_POD_TYPE();

    plVec3 m_vPosition;
    plVec3 m_vTexCoord; // U,V and Q
    float m_fAmbientOcclusion = 1.0f;
    plVec3 m_vNormal;
    plVec3 m_vTangent;
    plUInt8 m_uiColorVariation = 0;

    // to compute wind
    plUInt8 m_uiBranchLevel = 0;  // 0 = trunk, 1 = main branches, 2 = twigs, ...
    plUInt8 m_uiFlutterPhase = 0; // phase shift for the flutter effect
    plVec3 m_vBendAnchor;
    float m_fAnchorBendStrength = 0;
    float m_fBendAndFlutterStrength = 0;
  };

  struct TriangleData
  {
    PL_DECLARE_POD_TYPE();

    plUInt32 m_uiVertexIndex[3];
  };

  struct SubMeshData
  {
    plUInt16 m_uiFirstTriangle = 0;
    plUInt16 m_uiNumTriangles = 0;
    plUInt8 m_uiMaterialIndex = 0xFF;
  };

  struct LodData
  {
    float m_fMinLodDistance = 0;
    float m_fMaxLodDistance = 0;
    plKrautLodType m_LodType = plKrautLodType::None;

    plDynamicArray<VertexData> m_Vertices;
    plDynamicArray<TriangleData> m_Triangles;
    plDynamicArray<SubMeshData> m_SubMeshes;
  };

  struct MaterialData
  {
    plKrautMaterialType m_MaterialType;
    plString m_sMaterial;
    plColorGammaUB m_VariationColor = plColor::White; // currently done through the material
  };

  plKrautTreeResourceDetails m_Details;
  plStaticArray<LodData, 5> m_Lods;
  plHybridArray<MaterialData, 8> m_Materials;
};

class PL_KRAUTPLUGIN_DLL plKrautTreeResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plKrautTreeResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plKrautTreeResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plKrautTreeResource, plKrautTreeResourceDescriptor);

public:
  plKrautTreeResource();

  const plKrautTreeResourceDetails& GetDetails() const { return m_Details; }

  struct TreeLod
  {
    plMeshResourceHandle m_hMesh;
    float m_fMinLodDistance = 0;
    float m_fMaxLodDistance = 0;
    plKrautLodType m_LodType = plKrautLodType::None;
  };

  plArrayPtr<const TreeLod> GetTreeLODs() const { return m_TreeLODs.GetArrayPtr(); }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plKrautTreeResourceDetails m_Details;
  plStaticArray<TreeLod, 5> m_TreeLODs;
};
