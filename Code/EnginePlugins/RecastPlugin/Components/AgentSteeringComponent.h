#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Communication/Event.h>
#include <RecastPlugin/RecastPluginDLL.h>

struct plAgentSteeringEvent
{
  enum Type
  {
    TargetReached,              ///< The agent reached the current target location
    TargetCleared,              ///< The agent's target location was cleared and it is now not moving further
    PathToTargetFound,          ///< Path-finding was successful, agent will follow the path now.
    ErrorInvalidTargetPosition, ///< The target position cannot be reached because it is not inside the navigation area
    ErrorNoPathToTarget,        ///< Path-finding failed, the target location cannot be reached.
    WarningNoFullPathToTarget,  ///< Path-finding resulted in a partial path, so one can get closer to it, but the target cannot be reached.
    ErrorOutsideNavArea,        ///< The current agent position is outside valid navigation area
    ErrorSteeringFailed,        ///< Some generic error
  };

  Type m_Type;
  class plAgentSteeringComponent* m_pComponent = nullptr;
};

struct plAgentPathFindingState
{
  using StorageType = plUInt8;

  enum Enum
  {
    HasNoTarget,
    HasTargetWaitingForPath,
    HasTargetPathFindingFailed,
    HasTargetAndValidPath,

    Default = HasNoTarget
  };
};

/// \brief Base class for components that implement 'agent steering' behavior.
/// If, moving from point A to point B using something like a navmesh.
class PL_RECASTPLUGIN_DLL plAgentSteeringComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plAgentSteeringComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plAgentSteeringComponent

public:
  plAgentSteeringComponent();
  ~plAgentSteeringComponent();

  virtual void SetTargetPosition(const plVec3& vPosition) = 0; // [ scriptable ]
  virtual plVec3 GetTargetPosition() const = 0;                // [ scriptable ]
  virtual void ClearTargetPosition() = 0;                      // [ scriptable ]
  virtual plAgentPathFindingState::Enum GetPathToTargetState() const = 0;

  plEvent<const plAgentSteeringEvent&> m_SteeringEvents;
};
