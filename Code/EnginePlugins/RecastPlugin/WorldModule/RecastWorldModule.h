#pragma once

#include <RecastPlugin/RecastPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshPointsOfInterest.h>

class dtCrowd;
class dtNavMesh;
struct plResourceEvent;

using plRecastNavMeshResourceHandle = plTypedResourceHandle<class plRecastNavMeshResource>;

class PL_RECASTPLUGIN_DLL plRecastWorldModule : public plWorldModule
{
  PL_DECLARE_WORLD_MODULE();
  PL_ADD_DYNAMIC_REFLECTION(plRecastWorldModule, plWorldModule);

public:
  plRecastWorldModule(plWorld* pWorld);
  ~plRecastWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void SetNavMeshResource(const plRecastNavMeshResourceHandle& hNavMesh);
  const plRecastNavMeshResourceHandle& GetNavMeshResource() { return m_hNavMesh; }

  const dtNavMesh* GetDetourNavMesh() const { return m_pDetourNavMesh; }
  const plNavMeshPointOfInterestGraph* GetNavMeshPointsOfInterestGraph() const { return m_pNavMeshPointsOfInterest.Borrow(); }
  plNavMeshPointOfInterestGraph* AccessNavMeshPointsOfInterestGraph() const { return m_pNavMeshPointsOfInterest.Borrow(); }

private:
  void UpdateNavMesh(const UpdateContext& ctxt);
  void ResourceEventHandler(const plResourceEvent& e);

  const dtNavMesh* m_pDetourNavMesh = nullptr;
  plRecastNavMeshResourceHandle m_hNavMesh;
  plUniquePtr<plNavMeshPointOfInterestGraph> m_pNavMeshPointsOfInterest;
};
