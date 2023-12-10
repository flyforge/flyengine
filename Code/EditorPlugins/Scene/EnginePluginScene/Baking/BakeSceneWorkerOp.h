#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class plBakingScene;

class plLongOpWorker_BakeScene : public plLongOpWorker
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpWorker_BakeScene, plLongOpWorker);

public:
  virtual plResult InitializeExecution(plStreamReader& config, const plUuid& DocumentGuid) override;
  virtual plResult Execute(plProgress& progress, plStreamWriter& proxydata) override;

  plString m_sOutputPath;
  plBakingScene* m_pScene;
};
