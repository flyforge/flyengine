#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using plJoltShapeCapsuleComponentManager = plComponentManager<class plJoltShapeCapsuleComponent, plBlockStorageType::FreeList>;

/// \brief Adds a Jolt capsule shape to a Jolt actor.
class PL_JOLTPLUGIN_DLL plJoltShapeCapsuleComponent : public plJoltShapeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJoltShapeCapsuleComponent, plJoltShapeComponent, plJoltShapeCapsuleComponentManager);

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
  // plJoltShapeCapsuleComponent

public:
  plJoltShapeCapsuleComponent();
  ~plJoltShapeCapsuleComponent();

  void SetRadius(float f);                      // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

  void SetHeight(float f);                      // [ property ]
  float GetHeight() const { return m_fHeight; } // [ property ]

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
  float m_fHeight = 0.5f;
};
