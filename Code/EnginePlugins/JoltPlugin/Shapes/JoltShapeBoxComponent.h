#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using plJoltShapeBoxComponentManager = plComponentManager<class plJoltShapeBoxComponent, plBlockStorageType::FreeList>;

/// \brief Adds a Jolt box shape to a Jolt actor.
class PL_JOLTPLUGIN_DLL plJoltShapeBoxComponent : public plJoltShapeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJoltShapeBoxComponent, plJoltShapeComponent, plJoltShapeBoxComponentManager);

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
  // plJoltShapeBoxComponent

public:
  plJoltShapeBoxComponent();
  ~plJoltShapeBoxComponent();

  void SetHalfExtents(const plVec3& value);                       // [ property ]
  const plVec3& GetHalfExtents() const { return m_vHalfExtents; } // [ property ]

  virtual void ExtractGeometry(plMsgExtractGeometry& ref_msg) const override;

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const;

  plVec3 m_vHalfExtents = plVec3(0.5f);
};
