#include <EnginePluginRecast/EnginePluginRecastPCH.h>

#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

class plLongOpWorker_BuildNavMesh : public plLongOpWorker
{
  PL_ADD_DYNAMIC_REFLECTION(plLongOpWorker_BuildNavMesh, plLongOpWorker);

public:
  virtual plResult InitializeExecution(plStreamReader& ref_config, const plUuid& documentGuid) override;
  virtual plResult Execute(plProgress& ref_progress, plStreamWriter& ref_proxydata) override;

  plString m_sOutputPath;
  plRecastConfig m_NavMeshConfig;
  plWorldGeoExtractionUtil::MeshObjectList m_ExtractedObjects;
};
