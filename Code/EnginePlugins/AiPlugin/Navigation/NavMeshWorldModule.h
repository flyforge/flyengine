#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/Implementation/NavMeshGeneration.h>
#include <Core/World/WorldModule.h>

class plAiNavMesh;
class dtNavMesh;

/// This world module keeps track of all the configured navmeshes (for different character types)
/// and makes sure to build their sectors in the background.
///
/// Through this you can get access to one of the available navmeshes.
/// Additionally, it also provides access to the different path search filters.
class PL_AIPLUGIN_DLL plAiNavMeshWorldModule final : public plWorldModule
{
  PL_DECLARE_WORLD_MODULE();
  PL_ADD_DYNAMIC_REFLECTION(plAiNavMeshWorldModule, plWorldModule);

public:
  plAiNavMeshWorldModule(plWorld* pWorld);
  ~plAiNavMeshWorldModule();

  virtual void Initialize() override;

  plAiNavMesh* GetNavMesh(plStringView sName);
  const plAiNavMesh* GetNavMesh(plStringView sName) const;

  const dtQueryFilter& GetPathSearchFilter(plStringView sName) const;

private:
  void Update(const UpdateContext& ctxt);

  plMap<plString, plAiNavMesh*> m_WorldNavMeshes;

  // TODO: this is a hacky solution to delay the navmesh generation until after Physics has been set up.
  plUInt32 m_uiUpdateDelay = 10;
  plTaskGroupID m_GenerateSectorTaskID;
  plSharedPtr<plNavMeshSectorGenerationTask> m_pGenerateSectorTask;

  plAiNavigationConfig m_Config;

  plMap<plString, dtQueryFilter> m_PathSearchFilters;
};

/* TODO:

Navmesh Generation
==================

* fix navmesh on hills
* collision group filtering
* invalidate sectors, re-generate
* Invalidate path searches after sector changes
* sector usage tracking
* unload unused sectors

Path Search
===========

* callback for touched sectors
* on-demand sector generation ???
* use max edge-length + poly flags for 'dynamic' obstacles

Steering
========

* movement with ineratia
* decoupled position and rotation
* avoid dynamic obstacles

*/
