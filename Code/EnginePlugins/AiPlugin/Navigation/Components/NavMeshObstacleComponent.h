#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

using plNavMeshObstacleComponentManager = plComponentManager<class plNavMeshObstacleComponent, plBlockStorageType::Compact>;

/// \brief Represents a dynamic obstacle on a navmesh.
///
/// Automatically notifies the navmesh that sectors overlapping this object must be rebuilt.
///
/// Attach this to objects that are dynamically spawned, but need to affect the navmesh, blocking or unblocking pathing.
/// For example: a wall - obstacle component will make sure the navmesh is carved around it, so all path queries will
/// be blocked by it or go around it.
/// Another example could be a bridge that connects two navmesh islands, allowing a direct path between the two.
///
/// Currently, only works with static game objects with relevant physics geometry.
class PL_AIPLUGIN_DLL plNavMeshObstacleComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plNavMeshObstacleComponent, plComponent, plNavMeshObstacleComponentManager);

public:
  plNavMeshObstacleComponent();
  ~plNavMeshObstacleComponent();

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

public:
  void InvalidateSectors();
};