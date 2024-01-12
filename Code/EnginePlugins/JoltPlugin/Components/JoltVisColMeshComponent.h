#pragma once

#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

class plJoltVisColMeshComponentManager : public plComponentManager<class plJoltVisColMeshComponent, plBlockStorageType::Compact>
{
public:
  using SUPER = plComponentManager<plJoltVisColMeshComponent, plBlockStorageType::Compact>;

  plJoltVisColMeshComponentManager(plWorld* pWorld)
    : SUPER(pWorld)
  {
  }

  void Update(const plWorldModule::UpdateContext& context);
  void EnqueueUpdate(plComponentHandle hComponent);

private:
  void ResourceEventHandler(const plResourceEvent& e);

  mutable plMutex m_Mutex;
  plDeque<plComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

/// \brief Visualizes a Jolt collision mesh that is attached to the same game object.
///
/// When attached to a game object where a plJoltStaticActorComponent or a plJoltShapeConvexHullComponent is attached as well,
/// this component will retrieve the triangle mesh and turn it into a render mesh.
///
/// This is used for displaying the collision mesh of a single object.
/// It doesn't work for non-mesh shape types (sphere, box, capsule).
class PLASMA_JOLTPLUGIN_DLL plJoltVisColMeshComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltVisColMeshComponent, plRenderComponent, plJoltVisColMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltVisColMeshComponent

public:
  plJoltVisColMeshComponent();
  ~plJoltVisColMeshComponent();

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

  /// \brief If this is set directly, the mesh is not taken from the sibling components.
  void SetMesh(const plJoltMeshResourceHandle& hMesh);
  PLASMA_ALWAYS_INLINE const plJoltMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void CreateCollisionRenderMesh();

  plJoltMeshResourceHandle m_hCollisionMesh;
  mutable plMeshResourceHandle m_hMesh;
};
