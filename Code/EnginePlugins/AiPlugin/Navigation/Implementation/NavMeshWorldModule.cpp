#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <DetourNavMesh.h>
#include <Foundation/Configuration/CVar.h>

plCVarInt cvar_NavMeshVisualize("AI.Navmesh.Visualize", -1, plCVarFlags::None, "Visualize the n-th navmesh.");

// clang-format off
PL_IMPLEMENT_WORLD_MODULE(plAiNavMeshWorldModule);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAiNavMeshWorldModule, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAiNavMeshWorldModule::plAiNavMeshWorldModule(plWorld* pWorld)
  : plWorldModule(pWorld)
{
  m_Config.Load().IgnoreResult();

  {
    // add a default filter
    auto& cfg = m_Config.m_PathSearchConfigs.ExpandAndGetRef();
  }

  for (const auto& cfg : m_Config.m_PathSearchConfigs)
  {
    auto& filter = m_PathSearchFilters[cfg.m_sName];

    plUInt32 groundMask = 0;
    for (plUInt32 gt = 0; gt < plAiNumGroundTypes; ++gt)
    {
      if (cfg.m_bGroundTypeAllowed[gt])
        groundMask |= (1 << gt);

      filter.setAreaCost((int)gt, cfg.m_fGroundTypeCost[gt]);
    }

    filter.setIncludeAreaBits(groundMask);
  }

  if (m_Config.m_NavmeshConfigs.IsEmpty())
  {
    // insert a default navmesh config
    m_Config.m_NavmeshConfigs.ExpandAndGetRef();
  }
}

plAiNavMeshWorldModule::~plAiNavMeshWorldModule()
{
  for (const auto& cfg : m_Config.m_NavmeshConfigs)
  {
    PL_DEFAULT_DELETE(m_WorldNavMeshes[cfg.m_sName]);
  }
}

void plAiNavMeshWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plAiNavMeshWorldModule::Update, this);
    updateDesc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostTransform;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }

  m_WorldNavMeshes.Clear();

  for (const auto& cfg : m_Config.m_NavmeshConfigs)
  {
    m_WorldNavMeshes[cfg.m_sName] = PL_DEFAULT_NEW(plAiNavMesh, 64, 64, 16.0f, cfg);
  }

  m_pGenerateSectorTask = PL_DEFAULT_NEW(plNavMeshSectorGenerationTask);
  m_pGenerateSectorTask->ConfigureTask("Generate Navmesh Sector", plTaskNesting::Maybe);
}

plAiNavMesh* plAiNavMeshWorldModule::GetNavMesh(plStringView sName)
{
  auto it = m_WorldNavMeshes.Find(sName);
  if (it.IsValid())
    return it.Value();

  return nullptr;
}

const plAiNavMesh* plAiNavMeshWorldModule::GetNavMesh(plStringView sName) const
{
  auto it = m_WorldNavMeshes.Find(sName);
  if (it.IsValid())
    return it.Value();

  return nullptr;
}

void plAiNavMeshWorldModule::Update(const UpdateContext& ctxt)
{
  if (m_uiUpdateDelay > 0)
  {
    --m_uiUpdateDelay;
    return;
  }

  for (auto& nm : m_WorldNavMeshes)
  {
    nm.Value()->FinalizeSectorUpdates();
  }

  if (cvar_NavMeshVisualize >= 0)
  {
    plInt32 i = cvar_NavMeshVisualize;
    for (auto it = m_WorldNavMeshes.GetIterator(); it.IsValid(); ++it)
    {
      if (i-- == 0)
      {
        it.Value()->DebugDraw(GetWorld(), m_Config);
        break;
      }
    }
  }

  if (!plTaskSystem::IsTaskGroupFinished(m_GenerateSectorTaskID))
    return;

  auto pPhysics = GetWorld()->GetModule<plPhysicsWorldModuleInterface>();
  if (pPhysics == nullptr)
    return;

  for (auto& nm : m_WorldNavMeshes)
  {
    auto sectorID = nm.Value()->RetrieveRequestedSector();
    if (sectorID == plInvalidIndex)
      continue;

    m_pGenerateSectorTask->m_pWorldNavMesh = nm.Value();
    m_pGenerateSectorTask->m_SectorID = sectorID;
    m_pGenerateSectorTask->m_pPhysics = pPhysics;

    m_GenerateSectorTaskID = plTaskSystem::StartSingleTask(m_pGenerateSectorTask, plTaskPriority::LongRunning);

    break;
  }
}

const dtQueryFilter& plAiNavMeshWorldModule::GetPathSearchFilter(plStringView sName) const
{
  auto it = m_PathSearchFilters.Find(sName);
  if (it.IsValid())
    return it.Value();

  it = m_PathSearchFilters.Find("");
  plLog::Warning("Ai Path Search Filter '{}' does not exist.", sName);
  return it.Value();
}
