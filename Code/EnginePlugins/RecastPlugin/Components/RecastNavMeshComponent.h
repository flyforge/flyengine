#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/RecastPluginDLL.h>

class plRecastWorldModule;
class plAbstractObjectNode;

using plRecastNavMeshResourceHandle = plTypedResourceHandle<class plRecastNavMeshResource>;

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for all Recast components
class PLASMA_RECASTPLUGIN_DLL plRcComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plRcComponent, plComponent);

public:
  plRcComponent();
  ~plRcComponent();
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_RECASTPLUGIN_DLL plRcNavMeshComponentManager : public plComponentManager<class plRcNavMeshComponent, plBlockStorageType::Compact>
{
  typedef plComponentManager<class plRcNavMeshComponent, plBlockStorageType::Compact> SUPER;

public:
  plRcNavMeshComponentManager(plWorld* pWorld);
  ~plRcNavMeshComponentManager();

  virtual void Initialize() override;

  plRecastWorldModule* GetRecastWorldModule() const { return m_pWorldModule; }

  void Update(const plWorldModule::UpdateContext& context);

private:
  plRecastWorldModule* m_pWorldModule;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_RECASTPLUGIN_DLL plRcNavMeshComponent : public plRcComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plRcNavMeshComponent, plRcComponent, plRcNavMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  //  plRcNavMeshComponent

public:
  plRcNavMeshComponent();
  ~plRcNavMeshComponent();

  bool m_bShowNavMesh = false; // [ property ]

  plRecastConfig m_NavMeshConfig; // [ property ]

protected:
  void Update();
  void VisualizeNavMesh();
  void VisualizePointsOfInterest();

  plRecastNavMeshResourceHandle m_hNavMesh;


  //////////////////////////////////////////////////////////////////////////
  // Editor

protected:
  void OnObjectCreated(const plAbstractObjectNode& node);
};
