#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Messages/EventMessage.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

using plBeamComponentManager = plComponentManagerSimple<class plBeamComponent, plComponentUpdateType::Always>;

struct plMsgExtractRenderData;
class plGeometry;
class plMeshResourceDescriptor;

/// \brief A beam component
class PLASMA_RENDERERCORE_DLL plBeamComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plBeamComponent, plRenderComponent, plBeamComponentManager);

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

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // plBeamComponent

public:
  plBeamComponent();
  ~plBeamComponent();

  void SetTargetObject(const char* szReference); // [ property ]

  void SetWidth(float fWidth); // [ property ]
  float GetWidth() const;      // [ property ]

  void SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit); // [ property ]
  float GetUVUnitsPerWorldUnit() const;                    // [ property ]

  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  plMaterialResourceHandle GetMaterial() const;

  plGameObjectHandle m_hTargetObject; // [ property ]

  plColor m_Color; // [ property ]

protected:
  void Update();

  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  float m_fWidth = 0.1f;               // [ property ]
  float m_fUVUnitsPerWorldUnit = 1.0f; // [ property ]

  plMaterialResourceHandle m_hMaterial; // [ property ]

  const float m_fDistanceUpdateEpsilon = 0.02f;

  // State
  plMeshResourceHandle m_hMesh;

  plVec3 m_vLastOwnerPosition = plVec3::MakeZero();
  plVec3 m_vLastTargetPosition = plVec3::MakeZero();

  void CreateMeshes();
  void BuildMeshResourceFromGeometry(plGeometry& Geometry, plMeshResourceDescriptor& MeshDesc) const;
  void ReinitMeshes();
  void Cleanup();

  const char* DummyGetter() const { return nullptr; }
};
