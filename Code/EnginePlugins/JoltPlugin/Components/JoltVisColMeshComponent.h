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
  typedef plComponentManager<plJoltVisColMeshComponent, plBlockStorageType::Compact> SUPER;

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

  void SetMesh(const plJoltMeshResourceHandle& hMesh);
  PLASMA_ALWAYS_INLINE const plJoltMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void CreateCollisionRenderMesh();

  plJoltMeshResourceHandle m_hCollisionMesh;
  mutable plMeshResourceHandle m_hMesh;
};
