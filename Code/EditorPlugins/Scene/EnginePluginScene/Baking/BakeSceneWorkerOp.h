#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class plBakingScene;

class plLongOpWorker_BakeScene : public plLongOpWorker
{
  PL_ADD_DYNAMIC_REFLECTION(plLongOpWorker_BakeScene, plLongOpWorker);

public:
  virtual plResult InitializeExecution(plStreamReader& ref_config, const plUuid& documentGuid) override;
  virtual plResult Execute(plProgress& ref_progress, plStreamWriter& ref_proxydata) override;

  plString m_sOutputPath;
  plBakingScene* m_pScene;
};
