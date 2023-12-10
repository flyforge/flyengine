#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PreparePlacementTask.h>
#include <ProcGenPlugin/Tasks/Utils.h>

using namespace plProcGenInternal;

PreparePlacementTask::PreparePlacementTask(PlacementData* pData, const char* szName)
  : m_pData(pData)
{
  ConfigureTask(szName, plTaskNesting::Maybe);
}

PreparePlacementTask::~PreparePlacementTask() = default;

void PreparePlacementTask::Execute()
{
  const plWorld& world = *m_pData->m_pWorld;
  const plBoundingBox& box = m_pData->m_TileBoundingBox;
  const Output& output = *m_pData->m_pOutput;

  plProcGenInternal::ExtractVolumeCollections(world, box, output, m_pData->m_VolumeCollections, m_pData->m_GlobalData);
  plProcGenInternal::SetInstanceSeed(m_pData->m_uiTileSeed, m_pData->m_GlobalData);
}
