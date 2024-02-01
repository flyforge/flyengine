#include <EditorPluginRecast/EditorPluginRecastPCH.h>

#include <EditorPluginRecast/NavMesh/NavMeshProxyOp.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpProxy_BuildNavMesh, 1, plRTTIDefaultAllocator<plLongOpProxy_BuildNavMesh>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plLongOpProxy_BuildNavMesh::InitializeRegistered(const plUuid& documentGuid, const plUuid& componentGuid)
{
  m_DocumentGuid = documentGuid;
  m_ComponentGuid = componentGuid;
}

void plLongOpProxy_BuildNavMesh::GetReplicationInfo(plStringBuilder& out_sReplicationOpType, plStreamWriter& ref_description)
{
  out_sReplicationOpType = "plLongOpWorker_BuildNavMesh";

  {
    plStringBuilder sComponentGuid, sOutputFile;
    plConversionUtils::ToString(m_ComponentGuid, sComponentGuid);

    sOutputFile.SetFormat(":project/AssetCache/Generated/{0}.plRecastNavMesh", sComponentGuid);

    ref_description << sOutputFile;
  }

  const plDocument* pDoc = plDocumentManager::GetDocumentByGuid(m_DocumentGuid);
  const plDocumentObject* pObject = pDoc->GetObjectManager()->GetObject(m_ComponentGuid);
  plVariant configGuid = pObject->GetTypeAccessor().GetValue("NavMeshConfig");

  const plDocumentObject* pConfig = pDoc->GetObjectManager()->GetObject(configGuid.Get<plUuid>());
  auto& cfg = pConfig->GetTypeAccessor();

  plRecastConfig rcCfg;
  rcCfg.m_fAgentHeight = cfg.GetValue("AgentHeight").Get<float>();
  rcCfg.m_fAgentRadius = cfg.GetValue("AgentRadius").Get<float>();
  rcCfg.m_fAgentClimbHeight = cfg.GetValue("AgentClimbHeight").Get<float>();
  rcCfg.m_WalkableSlope = cfg.GetValue("WalkableSlope").Get<plAngle>();
  rcCfg.m_fCellSize = cfg.GetValue("CellSize").Get<float>();
  rcCfg.m_fCellHeight = cfg.GetValue("CellHeight").Get<float>();
  rcCfg.m_fMinRegionSize = cfg.GetValue("MinRegionSize").Get<float>();
  rcCfg.m_fRegionMergeSize = cfg.GetValue("RegionMergeSize").Get<float>();
  rcCfg.m_fDetailMeshSampleDistanceFactor = cfg.GetValue("SampleDistanceFactor").Get<float>();
  rcCfg.m_fDetailMeshSampleErrorFactor = cfg.GetValue("SampleErrorFactor").Get<float>();
  rcCfg.m_fMaxSimplificationError = cfg.GetValue("MaxSimplification").Get<float>();
  rcCfg.m_fMaxEdgeLength = cfg.GetValue("MaxEdgeLength").Get<float>();
  rcCfg.Serialize(ref_description).IgnoreResult();
}

void plLongOpProxy_BuildNavMesh::Finalize(plResult result, const plDataBuffer& resultData)
{
  if (result.Succeeded())
  {
    plQtEditorApp::GetSingleton()->ReloadEngineResources();
  }
}
