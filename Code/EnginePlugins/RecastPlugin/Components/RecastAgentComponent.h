#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <DetourNavMeshQuery.h>
#include <DetourPathCorridor.h>
#include <RecastPlugin/Components/AgentSteeringComponent.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/RecastPluginDLL.h>

class plRecastWorldModule;
class plPhysicsWorldModuleInterface;
struct plResourceEvent;

//////////////////////////////////////////////////////////////////////////

class PLASMA_RECASTPLUGIN_DLL plRcAgentComponentManager : public plComponentManager<class plRcAgentComponent, plBlockStorageType::FreeList>
{
  using SUPER = plComponentManager<class plRcAgentComponent, plBlockStorageType::FreeList>;

public:
  plRcAgentComponentManager(plWorld* pWorld);
  ~plRcAgentComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  plRecastWorldModule* GetRecastWorldModule() const { return m_pWorldModule; }

private:
  void ResourceEventHandler(const plResourceEvent& e);
  void Update(const plWorldModule::UpdateContext& context);

  plPhysicsWorldModuleInterface* m_pPhysicsInterface = nullptr;
  plRecastWorldModule* m_pWorldModule = nullptr;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_RECASTPLUGIN_DLL plRcAgentComponent : public plAgentSteeringComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plRcAgentComponent, plAgentSteeringComponent, plRcAgentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

protected:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plAgentSteeringComponent

public:
  plRcAgentComponent();
  ~plRcAgentComponent();

  virtual void SetTargetPosition(const plVec3& vPosition) override;
  virtual plVec3 GetTargetPosition() const override;
  virtual void ClearTargetPosition() override;
  virtual plAgentPathFindingState::Enum GetPathToTargetState() const override;

  //////////////////////////////////////////////////////////////////////////
  // Helper Functions

public:
  plResult FindNavMeshPolyAt(const plVec3& vPosition, dtPolyRef& out_polyRef, plVec3* out_pAdjustedPosition = nullptr, float fPlaneEpsilon = 0.01f,
    float fHeightEpsilon = 1.0f) const;
  bool HasReachedPosition(const plVec3& vPos, float fMaxDistance) const;
  bool HasReachedGoal(float fMaxDistance) const;
  bool IsPositionVisible(const plVec3& vPos) const;

  //////////////////////////////////////////////////////////////////////////
  // Debug Visualization Functions

private:
  void VisualizePathCorridorPosition();
  void VisualizePathCorridor();
  void VisualizeCurrentPath();
  void VisualizeTargetPosition();

  //////////////////////////////////////////////////////////////////////////
  // Path Finding and Steering

private:
  plResult ComputePathToTarget();
  plResult ComputePathCorridor(dtPolyRef startPoly, dtPolyRef endPoly, bool& bFoundPartialPath);
  void ComputeSteeringDirection(float fMaxDistance);
  void ApplySteering(const plVec3& vDirection, float fSpeed);
  void SyncSteeringWithReality();
  void PlanNextSteps();

  plVec3 m_vTargetPosition;
  plEnum<plAgentPathFindingState> m_PathToTargetState;
  plVec3 m_vCurrentPositionOnNavmesh;      /// \todo ??? keep update ?
  plUniquePtr<dtNavMeshQuery> m_pQuery;    // careful, dtNavMeshQuery is not moveble
  plUniquePtr<dtPathCorridor> m_pCorridor; // careful, dtPathCorridor is not moveble
  dtQueryFilter m_QueryFilter;             /// \todo hard-coded filter
  plDynamicArray<dtPolyRef> m_PathCorridor;
  // path following
  plInt32 m_iFirstNextStep = 0;
  plInt32 m_iNumNextSteps = 0;
  plVec3 m_vNextSteps[16];
  plVec3 m_vCurrentSteeringDirection;


  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  float m_fWalkSpeed = 4.0f; // [ property ]
  /// \todo Expose and use
  // float m_fRadius = 0.2f;
  // float m_fHeight = 1.0f;


  //////////////////////////////////////////////////////////////////////////
  // Other
private:
  plResult InitializeRecast();
  void UninitializeRecast();
  virtual void OnSimulationStarted() override;
  void Update();

  bool m_bRecastInitialized = false;
  plComponentHandle m_hCharacterController;
};
