#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RecastPlugin/Components/AgentSteeringComponent.h>
#include <RecastPlugin/RecastPluginDLL.h>

class plDetourCrowdWorldModule;
class plDetourCrowdAgentComponent;
struct plDetourCrowdAgentParams;

//////////////////////////////////////////////////////////////////////////

struct plDetourCrowdAgentRotationMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    LookAtNextPathCorner,
    MatchVelocityDirection,
    
    Default = LookAtNextPathCorner
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_RECASTPLUGIN_DLL, plDetourCrowdAgentRotationMode);

//////////////////////////////////////////////////////////////////////////

class PL_RECASTPLUGIN_DLL plDetourCrowdAgentComponentManager : public plComponentManager<plDetourCrowdAgentComponent, plBlockStorageType::FreeList>
{
  using SUPER = plComponentManager<plDetourCrowdAgentComponent, plBlockStorageType::FreeList>;

public:
  explicit plDetourCrowdAgentComponentManager(plWorld* pWorld);
  ~plDetourCrowdAgentComponentManager();

  virtual void Initialize() override;

  plDetourCrowdWorldModule* GetDetourCrowdWorldModule() const { return m_pDetourCrowdModule; }

private:
  void Update(const plWorldModule::UpdateContext& ctx);

  plUInt32 m_uiNextOwnerId = 1;
  plDetourCrowdWorldModule* m_pDetourCrowdModule = nullptr;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Implements navigation, path following and obstacle avoidance. Requires Recast navmesh.
///
/// This component provides the ability to intelligently navigate around the level (using Recast navmesh)
/// while avoiding other agents.
///
/// Usage: call SetTargetPosition() to command the agent to go somewhere. Subscribe to m_SteeringEvents
/// to receive feedback (target reached, pathfinding failed, etc).
class PL_RECASTPLUGIN_DLL plDetourCrowdAgentComponent : public plAgentSteeringComponent
{
  PL_DECLARE_COMPONENT_TYPE(plDetourCrowdAgentComponent, plAgentSteeringComponent, plDetourCrowdAgentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

protected:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plAgentSteeringComponent

public:
  plDetourCrowdAgentComponent();
  ~plDetourCrowdAgentComponent();

  /// \brief Sets the position to navigate to. 
  ///
  /// If the position is not on navmesh, the nearest point on navmesh (with some threshold)
  /// will be the actual target. You can get it by calling GetActualTargetPosition().
  virtual void SetTargetPosition(const plVec3& vPosition) override;
  virtual plVec3 GetTargetPosition() const override { return m_vTargetPosition; }
  virtual void ClearTargetPosition() override;
  virtual plAgentPathFindingState::Enum GetPathToTargetState() const override { return m_PathToTargetState; }

  //////////////////////////////////////////////////////////////////////////
  // Properties
protected:
  float m_fRadius = 0.3f; // [ property ]
  float m_fHeight = 1.8f; // [ property ]
  float m_fMaxSpeed = 3.5f; // [ property ]
  float m_fMaxAcceleration = 3.5f; // [property]
  float m_fStoppingDistance = 1.0f; // [property]
  plAngle m_MaxAngularSpeed = plAngle::MakeFromDegree(360.0f); // [property]
  plEnum<plDetourCrowdAgentRotationMode> m_RotationMode; // [property]
  float m_fPushiness = 2.0f; // [property]

public:
  float GetRadius() const { return m_fRadius; }
  float GetHeight() const { return m_fHeight; }
  float GetMaxSpeed() const { return m_fMaxSpeed; }
  float GetMaxAcceleration() const { return m_fMaxAcceleration; }
  float GetStoppingDistance() const { return m_fStoppingDistance; }
  plAngle GetMaxAngularSpeed() const { return m_MaxAngularSpeed; }
  /// \brief While GetTargetPosition() returns the requested target position,
  /// this one will return the actual point on navmesh that the agent is trying to reach
  plVec3 GetActualTargetPosition() const { return m_vActualTargetPosition; }
  plDetourCrowdAgentRotationMode::Enum GetRotationMode() const { return m_RotationMode; }
  float GetPushiness() const { return m_fPushiness; }

  void SetRadius(float fRadius);
  void SetHeight(float fHeight);
  void SetMaxSpeed(float fMaxSpeed);
  void SetMaxAcceleration(float fMaxAcceleration);
  /// \brief If distance to the target is less than the stopping distance, the target is reached.
  void SetStoppingDistance(float fStoppingDistance);
  void SetMaxAngularSpeed(plAngle maxAngularSpeed);
  void SetRotationMode(plDetourCrowdAgentRotationMode::Enum rotationMode) { m_RotationMode = rotationMode; }
  /// \brief The agent will push other agents with lower pushiness and will get pushed by agents 
  /// with higher pushiness. 
  void SetPushiness(float fPushiness);

  plVec3 GetVelocity() const { return m_vVelocity; }
  plAngle GetAngularSpeed() const { return m_AngularSpeed; }

  //////////////////////////////////////////////////////////////////////////
  // Other
protected:
  void FillAgentParams(plDetourCrowdAgentParams& out_params) const;

  virtual void OnDeactivated() override;

  plQuat RotateTowardsDirection(const plQuat& qCurrentRot, const plVec3& vTargetDir, plAngle& out_angularSpeed) const;
  plVec3 GetDirectionToNextPathCorner(const plVec3& vCurrentPos, const struct dtCrowdAgent* pDtAgent) const;
  bool SyncRotation(const plVec3& vPosition, plQuat& inout_qRotation, const plVec3& vVelocity, const struct dtCrowdAgent* pDtAgent);

  void SyncTransform(const struct dtCrowdAgent* pDtAgent, bool bTeleport);

  plUInt8 m_uiTargetDirtyBit : 1;
  plUInt8 m_uiSteeringFailedBit : 1;
  plUInt8 m_uiErrorBit : 1;
  plUInt8 m_uiParamsDirtyBit : 1;

  plInt32 m_iAgentId = -1;
  plUInt32 m_uiOwnerId = 0;
  plVec3 m_vVelocity;
  plAngle m_AngularSpeed;
  plVec3 m_vTargetPosition;
  plVec3 m_vActualTargetPosition;
  plEnum<plAgentPathFindingState> m_PathToTargetState;
};
