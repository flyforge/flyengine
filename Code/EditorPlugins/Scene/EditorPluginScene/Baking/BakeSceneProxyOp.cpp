#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Baking/BakeSceneProxyOp.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpProxy_BakeScene, 1, plRTTIDefaultAllocator<plLongOpProxy_BakeScene>);
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plLongOpProxy_BakeScene::InitializeRegistered(const plUuid& documentGuid, const plUuid& componentGuid)
{
  m_DocumentGuid = documentGuid;
  m_ComponentGuid = componentGuid;
}

void plLongOpProxy_BakeScene::GetReplicationInfo(plStringBuilder& out_sReplicationOpType, plStreamWriter& ref_description)
{
  out_sReplicationOpType = "plLongOpWorker_BakeScene";

  plStringBuilder sOutputPath;
  sOutputPath.SetFormat(":project/AssetCache/Generated/{0}", m_ComponentGuid);
  ref_description << sOutputPath;
}

void plLongOpProxy_BakeScene::Finalize(plResult result, const plDataBuffer& resultData)
{
  if (result.Succeeded())
  {
    plQtEditorApp::GetSingleton()->ReloadEngineResources();
  }
}
