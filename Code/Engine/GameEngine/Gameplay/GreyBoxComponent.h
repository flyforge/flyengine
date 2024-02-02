#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>

class plMeshRenderData;
class plGeometry;
struct plMsgExtractRenderData;
struct plMsgBuildStaticMesh;
struct plMsgExtractGeometry;
struct plMsgExtractOccluderData;
struct plMsgSetMeshMaterial;
struct plMsgSetColor;
struct plMsgTransformChanged;
class plMeshResourceDescriptor;
using plMeshResourceHandle = plTypedResourceHandle<class plMeshResource>;
using plMaterialResourceHandle = plTypedResourceHandle<class plMaterialResource>;

using plGreyBoxComponentManager = plComponentManager<class plGreyBoxComponent, plBlockStorageType::Compact>;

struct PL_GAMEENGINE_DLL plGreyBoxShape
{
  using StorageType = plUInt8;

  enum Enum
  {
    Box,
    RampX,
    RampY,
    Column,
    StairsX,
    StairsY,
    ArchX,
    ArchY,
    SpiralStairs,

    Default = Box
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plGreyBoxShape)

/// \brief Creates basic geometry for prototyping levels.
///
/// It automatically creates physics collision geometry and also sets up rendering occluders to improve performance.
class PL_GAMEENGINE_DLL plGreyBoxComponent : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plGreyBoxComponent, plRenderComponent, plGreyBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent
protected:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // plGreyBoxComponent

public:
  plGreyBoxComponent();
  ~plGreyBoxComponent();

  /// \brief The geometry type to build.
  void SetShape(plEnum<plGreyBoxShape> shape);                // [ property ]
  plEnum<plGreyBoxShape> GetShape() const { return m_Shape; } // [ property ]

  /// \brief The plMaterialResource file to use.
  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  /// \brief Sets the extent along the negative X axis of the bounding box.
  void SetSizeNegX(float f);                        // [ property ]
  float GetSizeNegX() const { return m_fSizeNegX; } // [ property ]

  /// \brief Sets the extent along the positive X axis of the bounding box.
  void SetSizePosX(float f);                        // [ property ]
  float GetSizePosX() const { return m_fSizePosX; } // [ property ]

  /// \brief Sets the extent along the negative Y axis of the bounding box.
  void SetSizeNegY(float f);                        // [ property ]
  float GetSizeNegY() const { return m_fSizeNegY; } // [ property ]

  /// \brief Sets the extent along the positive Y axis of the bounding box.
  void SetSizePosY(float f);                        // [ property ]
  float GetSizePosY() const { return m_fSizePosY; } // [ property ]

  /// \brief Sets the extent along the negative Z axis of the bounding box.
  void SetSizeNegZ(float f);                        // [ property ]
  float GetSizeNegZ() const { return m_fSizeNegZ; } // [ property ]

  /// \brief Sets the extent along the positive Z axis of the bounding box.
  void SetSizePosZ(float f);                        // [ property ]
  float GetSizePosZ() const { return m_fSizePosZ; } // [ property ]

  /// \brief Sets the detail of the geometry. The meaning is geometry type specific, e.g. for cylinders this is the number of polygons around the perimeter.
  void SetDetail(plUInt32 uiDetail);                // [ property ]
  plUInt32 GetDetail() const { return m_uiDetail; } // [ property ]

  /// \brief Geometry type specific: Sets an angle, used to curve stairs, etc.
  void SetCurvature(plAngle curvature);                // [ property ]
  plAngle GetCurvature() const { return m_Curvature; } // [ property ]

  /// \brief For curved stairs to make the top smooth.
  void SetSlopedTop(bool b);                         // [ property ]
  bool GetSlopedTop() const { return m_bSlopedTop; } // [ property ]

  /// \brief For curved stairs to make the bottom smooth.
  void SetSlopedBottom(bool b);                            // [ property ]
  bool GetSlopedBottom() const { return m_bSlopedBottom; } // [ property ]

  /// \brief Geometry type specific: Sets a thickness, e.g. for curved stairs.
  void SetThickness(float f);                         // [ property ]
  float GetThickness() const { return m_fThickness; } // [ property ]

  /// \brief Whether the mesh should be used as a collider.
  void SetGenerateCollision(bool b);                                 // [ property ]
  bool GetGenerateCollision() const { return m_bGenerateCollision; } // [ property ]

  /// \brief Whether the mesh should be an obstacle in the navmesh.
  /// \note This may or may not work, depending on how the navmesh generation works.
  /// Dynamic navmesh generation at runtime usually uses the physics colliders and thus this flag would have no effect there.
  void SetIncludeInNavmesh(bool b);                                // [ property ]
  bool GetIncludeInNavmesh() const { return m_bIncludeInNavmesh; } // [ property ]

  /// \brief Sets the plMaterialResource to use for rendering.
  void SetMaterial(const plMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  plMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

protected:
  void OnBuildStaticMesh(plMsgBuildStaticMesh& msg) const;
  void OnMsgExtractGeometry(plMsgExtractGeometry& msg) const;
  void OnMsgExtractOccluderData(plMsgExtractOccluderData& msg) const;
  void OnMsgSetMeshMaterial(plMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(plMsgSetColor& ref_msg);               // [ msg handler ]

  plEnum<plGreyBoxShape> m_Shape;
  plMaterialResourceHandle m_hMaterial;
  plColor m_Color = plColor::White;
  float m_fSizeNegX = 0;
  float m_fSizePosX = 0;
  float m_fSizeNegY = 0;
  float m_fSizePosY = 0;
  float m_fSizeNegZ = 0;
  float m_fSizePosZ = 0;
  plUInt32 m_uiDetail = 16;
  plAngle m_Curvature;
  float m_fThickness = 0.5f;
  bool m_bSlopedTop = false;
  bool m_bSlopedBottom = false;
  bool m_bGenerateCollision = true;
  bool m_bIncludeInNavmesh = true;
  bool m_bUseAsOccluder = true;

  void InvalidateMesh();
  void BuildGeometry(plGeometry& geom, plEnum<plGreyBoxShape> shape, bool bOnlyRoughDetails) const;

  template <typename ResourceType>
  plTypedResourceHandle<ResourceType> GenerateMesh() const;

  void GenerateMeshName(plStringBuilder& out_sName) const;
  void GenerateMeshResourceDescriptor(plMeshResourceDescriptor& desc) const;

  plMeshResourceHandle m_hMesh;

  mutable plSharedPtr<const plRasterizerObject> m_pOccluderObject;
};
