#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Baking/BakeSceneWorkerOp.h>

#ifdef BUILDSYSTEM_ENABLE_EMBREE_SUPPORT

#  include <BakingPlugin/BakingScene.h>
#  include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#  include <Foundation/Utilities/Progress.h>
#  include <ToolsFoundation/Document/DocumentManager.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpWorker_BakeScene, 1, plRTTIDefaultAllocator<plLongOpWorker_BakeScene>);
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plLongOpWorker_BakeScene::InitializeExecution(plStreamReader& config, const plUuid& DocumentGuid)
{
  PlasmaEngineProcessDocumentContext* pDocContext = PlasmaEngineProcessDocumentContext::GetDocumentContext(DocumentGuid);

  if (pDocContext == nullptr)
    return PLASMA_FAILURE;

  config >> m_sOutputPath;

  {
    m_pScene = plBaking::GetSingleton()->GetOrCreateScene(*pDocContext->GetWorld());

    PLASMA_SUCCEED_OR_RETURN(m_pScene->Extract());
  }

  return PLASMA_SUCCESS;
}

plResult plLongOpWorker_BakeScene::Execute(plProgress& progress, plStreamWriter& proxydata)
{
  PLASMA_SUCCEED_OR_RETURN(m_pScene->Bake(m_sOutputPath, progress));

  return PLASMA_SUCCESS;
}

#endif
