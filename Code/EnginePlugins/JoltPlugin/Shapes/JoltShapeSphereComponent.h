#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using plJoltShapeSphereComponentManager = plComponentManager<class plJoltShapeSphereComponent, plBlockStorageType::FreeList>;

/// \brief Adds a Jolt sphere shape to a Jolt actor.
class PL_JOLTPLUGIN_DLL plJoltShapeSphereComponent : public plJoltShapeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJoltShapeSphereComponent, plJoltShapeComponent, plJoltShapeSphereComponentManager);

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
  // plJoltShapeSphereComponent

public:
  plJoltShapeSphereComponent();
  ~plJoltShapeSphereComponent();

  void SetRadius(float f);                      // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
};
