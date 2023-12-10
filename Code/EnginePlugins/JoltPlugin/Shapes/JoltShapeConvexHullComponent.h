#pragma once

#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using plJoltShapeConvexHullComponentManager = plComponentManager<class plJoltShapeConvexHullComponent, plBlockStorageType::FreeList>;

class PLASMA_JOLTPLUGIN_DLL plJoltShapeConvexHullComponent : public plJoltShapeComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltShapeConvexHullComponent, plJoltShapeComponent, plJoltShapeConvexHullComponentManager);

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
  // plConvexShapeConvexComponent

public:
  plJoltShapeConvexHullComponent();
  ~plJoltShapeConvexHullComponent();

  virtual void ExtractGeometry(plMsgExtractGeometry& ref_msg) const override;

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

  plJoltMeshResourceHandle GetMesh() const { return m_hCollisionMesh; }

protected:
  plJoltMeshResourceHandle m_hCollisionMesh;
};
