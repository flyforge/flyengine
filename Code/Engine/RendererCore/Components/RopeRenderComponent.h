#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <memory>

struct plMsgExtractRenderData;
struct plMsgSetColor;
struct plMsgSetMeshMaterial;
struct plMsgRopePoseUpdated;
class plShaderTransform;

using plRopeRenderComponentManager = plComponentManager<class plRopeRenderComponent, plBlockStorageType::Compact>;

/// \brief Used to render a rope or cable.
///
/// This is needed to visualize the plFakeRopeComponent or plJoltRopeComponent.
/// The component handles the message plMsgRopePoseUpdated to generate an animated mesh and apply the pose.
/// The component has to be attached to the same object as the rope simulation component.
class PL_RENDERERCORE_DLL plRopeRenderComponent : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plRopeRenderComponent, plRenderComponent, plRopeRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

protected:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const; // [ msg handler ]

  //////////////////////////////////////////////////////////////////////////
  // plRopeRenderComponent

public:
  plRopeRenderComponent();
  ~plRopeRenderComponent();

  plColor m_Color = plColor::White; // [ property ]

  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  void SetMaterial(const plMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  plMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

  /// \brief Changes how thick the rope visualization is. This is independent of the simulated rope thickness.
  void SetThickness(float fThickness);                // [ property ]
  float GetThickness() const { return m_fThickness; } // [ property ]

  /// \brief Sets how round the rope shall be.
  void SetDetail(plUInt32 uiDetail);                // [ property ]
  plUInt32 GetDetail() const { return m_uiDetail; } // [ property ]

  /// \brief If enabled, the rendered mesh will be slightly more detailed along the rope.
  void SetSubdivide(bool bSubdivide);                // [ property ]
  bool GetSubdivide() const { return m_bSubdivide; } // [ property ]

  /// \brief How often to repeat the U texture coordinate along the rope's length.
  void SetUScale(float fUScale);                // [ property ]
  float GetUScale() const { return m_fUScale; } // [ property ]

  void OnMsgSetColor(plMsgSetColor& ref_msg);               // [ msg handler ]
  void OnMsgSetMeshMaterial(plMsgSetMeshMaterial& ref_msg); // [ msg handler ]

private:
  void OnRopePoseUpdated(plMsgRopePoseUpdated& msg); // [ msg handler ]
  void GenerateRenderMesh(plUInt32 uiNumRopePieces);
  void UpdateSkinningTransformBuffer(plArrayPtr<const plTransform> skinningTransforms);

  plBoundingBoxSphere m_LocalBounds;

  plSkinningState m_SkinningState;

  plMeshResourceHandle m_hMesh;
  plMaterialResourceHandle m_hMaterial;

  float m_fThickness = 0.05f;
  plUInt32 m_uiDetail = 6;
  bool m_bSubdivide = false;

  float m_fUScale = 1.0f;
};
