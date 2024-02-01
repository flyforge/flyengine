#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using plJoltShapeCylinderComponentManager = plComponentManager<class plJoltShapeCylinderComponent, plBlockStorageType::FreeList>;

/// \brief Adds a Jolt cylinder shape to a Jolt actor.
///
/// Be aware that the cylinder shape is not as stable in simulation as other shapes.
/// If possible use capsule shapes instead. In some cases even using a convex hull shape may provide better results,
/// but this has to be tried out case by case.
class PL_JOLTPLUGIN_DLL plJoltShapeCylinderComponent : public plJoltShapeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJoltShapeCylinderComponent, plJoltShapeComponent, plJoltShapeCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // plJoltShapeComponent

protected:
  virtual void CreateShapes(plDynamicArray<plJoltSubShape>& out_Shapes, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial) override;


  //////////////////////////////////////////////////////////////////////////
  // plJoltShapeCylinderComponent

public:
  plJoltShapeCylinderComponent();
  ~plJoltShapeCylinderComponent();

  void SetRadius(float f);                      // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

  void SetHeight(float f);                      // [ property ]
  float GetHeight() const { return m_fHeight; } // [ property ]

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
  float m_fHeight = 0.5f;
};
