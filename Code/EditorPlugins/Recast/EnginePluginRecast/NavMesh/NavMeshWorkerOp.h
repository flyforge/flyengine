#include <EnginePluginRecast/EnginePluginRecastPCH.h>

#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

class plLongOpWorker_BuildNavMesh : public plLongOpWorker
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpWorker_BuildNavMesh, plLongOpWorker);

public:
  virtual plResult InitializeExecution(plStreamReader& config, const plUuid& DocumentGuid) override;
  virtual plResult Execute(plProgress& progress, plStreamWriter& proxydata) override;

  plString m_sOutputPath;
  plRecastConfig m_NavMeshConfig;
  plWorldGeoExtractionUtil::MeshObjectList m_ExtractedObjects;
};
