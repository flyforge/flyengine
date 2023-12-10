#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

struct plMsgExtractGeometry;

using plJoltStaticActorComponentManager = plComponentManager<class plJoltStaticActorComponent, plBlockStorageType::FreeList>;

class PLASMA_JOLTPLUGIN_DLL plJoltStaticActorComponent : public plJoltActorComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltStaticActorComponent, plJoltActorComponent, plJoltStaticActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  void PullSurfacesFromGraphicsMesh(plDynamicArray<const plJoltMaterial*>& ref_materials);

  //////////////////////////////////////////////////////////////////////////
  // plJoltActorComponent
protected:
  virtual void CreateShapes(plDynamicArray<plJoltSubShape>& out_Shapes, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial) override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltStaticActorComponent

public:
  plJoltStaticActorComponent();
  ~plJoltStaticActorComponent();

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

  void SetMesh(const plJoltMeshResourceHandle& hMesh);
  PLASMA_ALWAYS_INLINE const plJoltMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  bool m_bIncludeInNavmesh = true;              // [ property ]
  bool m_bPullSurfacesFromGraphicsMesh = false; // [ property ]
  plSurfaceResourceHandle m_hSurface;           // [ property ]

protected:
  void OnMsgExtractGeometry(plMsgExtractGeometry& msg) const;
  const plJoltMaterial* GetJoltMaterial() const;

  plJoltMeshResourceHandle m_hCollisionMesh;

  // array to keep surfaces alive, in case they are pulled from the materials of the render mesh
  plDynamicArray<plSurfaceResourceHandle> m_UsedSurfaces;
};
