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
struct plMsgTransformChanged;
class plMeshResourceDescriptor;
using plMeshResourceHandle = plTypedResourceHandle<class plMeshResource>;
using plMaterialResourceHandle = plTypedResourceHandle<class plMaterialResource>;

typedef plComponentManager<class plGreyBoxComponent, plBlockStorageType::Compact> plGreyBoxComponentManager;

struct PLASMA_GAMEENGINE_DLL plGreyBoxShape
{
  typedef plUInt8 StorageType;

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

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plGreyBoxShape)

class PLASMA_GAMEENGINE_DLL plGreyBoxComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plGreyBoxComponent, plRenderComponent, plGreyBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

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

  void SetShape(plEnum<plGreyBoxShape> shape);                // [ property ]
  plEnum<plGreyBoxShape> GetShape() const { return m_Shape; } // [ property ]
  void SetMaterialFile(const char* szFile);                   // [ property ]
  const char* GetMaterialFile() const;                        // [ property ]
  void SetSizeNegX(float f);                                  // [ property ]
  float GetSizeNegX() const { return m_fSizeNegX; }           // [ property ]
  void SetSizePosX(float f);                                  // [ property ]
  float GetSizePosX() const { return m_fSizePosX; }           // [ property ]
  void SetSizeNegY(float f);                                  // [ property ]
  float GetSizeNegY() const { return m_fSizeNegY; }           // [ property ]
  void SetSizePosY(float f);                                  // [ property ]
  float GetSizePosY() const { return m_fSizePosY; }           // [ property ]
  void SetSizeNegZ(float f);                                  // [ property ]
  float GetSizeNegZ() const { return m_fSizeNegZ; }           // [ property ]
  void SetSizePosZ(float f);                                  // [ property ]
  float GetSizePosZ() const { return m_fSizePosZ; }           // [ property ]
  void SetDetail(plUInt32 uiDetail);                          // [ property ]
  plUInt32 GetDetail() const { return m_uiDetail; }           // [ property ]
  void SetCurvature(plAngle curvature);                       // [ property ]
  plAngle GetCurvature() const { return m_Curvature; }        // [ property ]
  void SetSlopedTop(bool b);                                  // [ property ]
  bool GetSlopedTop() const { return m_bSlopedTop; }          // [ property ]
  void SetSlopedBottom(bool b);                               // [ property ]
  bool GetSlopedBottom() const { return m_bSlopedBottom; }    // [ property ]
  void SetThickness(float f);                                 // [ property ]
  float GetThickness() const { return m_fThickness; }         // [ property ]

  void SetGenerateCollision(bool b);                                 // [ property ]
  bool GetGenerateCollision() const { return m_bGenerateCollision; } // [ property ]

  void SetIncludeInNavmesh(bool b);                                // [ property ]
  bool GetIncludeInNavmesh() const { return m_bIncludeInNavmesh; } // [ property ]

  void SetMaterial(const plMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  plMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

protected:
  void OnBuildStaticMesh(plMsgBuildStaticMesh& msg) const;
  void OnMsgExtractGeometry(plMsgExtractGeometry& msg) const;
  void OnMsgExtractOccluderData(plMsgExtractOccluderData& msg) const;

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
