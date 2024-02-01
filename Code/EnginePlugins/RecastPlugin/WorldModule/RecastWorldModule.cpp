#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/World/World.h>
#include <DetourCrowd.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>

// clang-format off
PL_IMPLEMENT_WORLD_MODULE(plRecastWorldModule);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRecastWorldModule, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRecastWorldModule::plRecastWorldModule(plWorld* pWorld)
  : plWorldModule(pWorld)
{
}

plRecastWorldModule::~plRecastWorldModule() = default;

void plRecastWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plRecastWorldModule::UpdateNavMesh, this);
    updateDesc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    updateDesc.m_bOnlyUpdateWhenSimulating = false;
    updateDesc.m_fPriority = 0.0f;

    RegisterUpdateFunction(updateDesc);
  }

  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plRecastWorldModule::ResourceEventHandler, this));
}

void plRecastWorldModule::Deinitialize()
{
  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plRecastWorldModule::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void plRecastWorldModule::SetNavMeshResource(const plRecastNavMeshResourceHandle& hNavMesh)
{
  m_hNavMesh = hNavMesh;
  m_pDetourNavMesh = nullptr;
  m_pNavMeshPointsOfInterest.Clear();
}

void plRecastWorldModule::UpdateNavMesh(const UpdateContext& ctxt)
{
  if (m_pDetourNavMesh == nullptr && m_hNavMesh.IsValid())
  {
    plResourceLock<plRecastNavMeshResource> pNavMesh(m_hNavMesh, plResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pNavMesh.GetAcquireResult() != plResourceAcquireResult::Final)
      return;

    m_pDetourNavMesh = pNavMesh->GetNavMesh();

    if (m_pDetourNavMesh)
    {
      m_pNavMeshPointsOfInterest = PL_DEFAULT_NEW(plNavMeshPointOfInterestGraph);
      m_pNavMeshPointsOfInterest->ExtractInterestPointsFromMesh(*pNavMesh->GetNavMeshPolygons());
    }
  }

  if (m_pNavMeshPointsOfInterest)
  {
    m_pNavMeshPointsOfInterest->IncreaseCheckVisibiblityTimeStamp(GetWorld()->GetClock().GetAccumulatedTime());
  }
}

void plRecastWorldModule::ResourceEventHandler(const plResourceEvent& e)
{
  if (e.m_Type == plResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<plRecastNavMeshResource>())
  {
    // triggers a recreation in the next update
    m_pDetourNavMesh = nullptr;
  }
}
