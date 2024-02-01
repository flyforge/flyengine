#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class plLongOpProxy_BakeScene : public plLongOpProxy
{
  PL_ADD_DYNAMIC_REFLECTION(plLongOpProxy_BakeScene, plLongOpProxy);

public:
  virtual void InitializeRegistered(const plUuid& documentGuid, const plUuid& componentGuid) override;
  virtual const char* GetDisplayName() const override { return "Bake Scene"; }
  virtual void GetReplicationInfo(plStringBuilder& out_sReplicationOpType, plStreamWriter& ref_description) override;
  virtual void Finalize(plResult result, const plDataBuffer& resultData) override;

private:
  plUuid m_DocumentGuid;
  plUuid m_ComponentGuid;
};
