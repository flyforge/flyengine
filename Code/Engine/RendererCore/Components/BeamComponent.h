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

/// \brief Renders a thick line from its own location to the position of another game object.
///
/// This is meant for simple effects, like laser beams. The geometry is very low resolution and won't look good close up.
/// When possible, use a highly emissive material without any pattern, where the bloom will hide the simple geometry.
///
/// For doing dynamic laser beams, you can combine it with the plRaycastComponent, which will move the target component.
class PL_RENDERERCORE_DLL plBeamComponent : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plBeamComponent, plRenderComponent, plBeamComponentManager);

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

  /// \brief Sets the GUID of the target object to which to draw the beam.
  void SetTargetObject(const char* szReference); // [ property ]

  /// \brief How wide to make the beam geometry
  void SetWidth(float fWidth); // [ property ]
  float GetWidth() const;      // [ property ]

  /// \brief How many world units the texture coordinates should take up, for using a repeatable texture for the beam.
  void SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit); // [ property ]
  float GetUVUnitsPerWorldUnit() const;                    // [ property ]

  /// \brief Which material asset to use for rendering the beam geometry.
  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  plMaterialResourceHandle GetMaterial() const;

  /// \brief The object to which to draw the beam.
  plGameObjectHandle m_hTargetObject; // [ property ]

  /// \brief Optional color to tint the beam.
  plColor m_Color = plColor::White; // [ property ]

protected:
  void Update();

  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  float m_fWidth = 0.1f;               // [ property ]
  float m_fUVUnitsPerWorldUnit = 1.0f; // [ property ]

  plMaterialResourceHandle m_hMaterial; // [ property ]

  const float m_fDistanceUpdateEpsilon = 0.02f;

  plMeshResourceHandle m_hMesh;

  plVec3 m_vLastOwnerPosition = plVec3::MakeZero();
  plVec3 m_vLastTargetPosition = plVec3::MakeZero();

  void CreateMeshes();
  void BuildMeshResourceFromGeometry(plGeometry& Geometry, plMeshResourceDescriptor& MeshDesc) const;
  void ReinitMeshes();
  void Cleanup();

  const char* DummyGetter() const { return nullptr; }
};
