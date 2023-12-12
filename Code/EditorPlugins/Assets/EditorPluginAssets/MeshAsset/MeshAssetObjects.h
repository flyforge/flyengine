#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct plPropertyMetaStateEvent;

struct plMeshPrimitive
{
  typedef plInt8 StorageType;

  enum Enum
  {
    File,
    Box,
    Rect,
    Cylinder,
    Cone,
    Pyramid,
    Sphere,
    HalfSphere,
    GeodesicSphere,
    Capsule,
    Torus,

    Default = File
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plMeshPrimitive);

class plMeshAssetProperties : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMeshAssetProperties, plReflectedClass);

public:
  plMeshAssetProperties();
  ~plMeshAssetProperties();

  static void PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

  plString m_sMeshFile;
  float m_fUniformScaling = 1.0f;

  float m_fRadius = 0.5f;
  float m_fRadius2 = 0.5f;
  float m_fHeight = 1.0f;
  plAngle m_Angle = plAngle::Degree(360.0f);
  plUInt16 m_uiDetail = 0;
  plUInt16 m_uiDetail2 = 0;
  bool m_bCap = true;
  bool m_bCap2 = true;

  plEnum<plBasisAxis> m_RightDir = plBasisAxis::PositiveY;
  plEnum<plBasisAxis> m_UpDir = plBasisAxis::PositiveZ;
  bool m_bFlipForwardDir = false;

  plMeshPrimitive::Enum m_PrimitiveType = plMeshPrimitive::Default;

  bool m_bRecalculateNormals = false;
  bool m_bRecalculateTrangents = true;
  bool m_bImportMaterials = true;
  bool m_bOptimize = true;

  plEnum<plMeshNormalPrecision> m_NormalPrecision;
  plEnum<plMeshTexCoordPrecision> m_TexCoordPrecision;

  plHybridArray<plMaterialResourceSlot, 8> m_Slots;

  plUInt32 m_uiVertices = 0;
  plUInt32 m_uiTriangles = 0;
};
