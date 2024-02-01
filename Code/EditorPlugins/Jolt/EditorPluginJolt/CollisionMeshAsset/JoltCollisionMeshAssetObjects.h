#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct plPropertyMetaStateEvent;

struct plJoltSurfaceResourceSlot
{
  plString m_sLabel;
  plString m_sResource;
  bool m_bExclude = false;
};

struct plJoltCollisionMeshType
{
  using StorageType = plInt8;

  enum Enum
  {
    ConvexHull,
    TriangleMesh,
    Cylinder,

    Default = TriangleMesh
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plJoltCollisionMeshType);

struct plJoltConvexCollisionMeshType
{
  using StorageType = plInt8;

  enum Enum
  {
    ConvexHull,
    Cylinder,
    ConvexDecomposition,

    Default = ConvexHull
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plJoltConvexCollisionMeshType);

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plJoltSurfaceResourceSlot);

class plJoltCollisionMeshAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plJoltCollisionMeshAssetProperties, plReflectedClass);

public:
  plJoltCollisionMeshAssetProperties();
  ~plJoltCollisionMeshAssetProperties();

  static void PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

  plString m_sMeshFile;
  float m_fUniformScaling = 1.0f;
  plString m_sConvexMeshSurface;

  plEnum<plBasisAxis> m_RightDir = plBasisAxis::PositiveX;
  plEnum<plBasisAxis> m_UpDir = plBasisAxis::PositiveY;
  bool m_bFlipForwardDir = false;
  bool m_bIsConvexMesh = false;
  plEnum<plJoltConvexCollisionMeshType> m_ConvexMeshType;
  plUInt16 m_uiMaxConvexPieces = 2;

  // Cylinder
  float m_fRadius = 0.5f;
  float m_fRadius2 = 0.5f;
  float m_fHeight = 1.0f;
  plUInt8 m_uiDetail = 1;

  plHybridArray<plJoltSurfaceResourceSlot, 8> m_Slots;

  plUInt32 m_uiVertices = 0;
  plUInt32 m_uiTriangles = 0;
};
