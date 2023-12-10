#include <EditorPluginRecast/EditorPluginRecastPCH.h>

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class plLongOpProxy_BuildNavMesh : public plLongOpProxy
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpProxy_BuildNavMesh, plLongOpProxy);

public:
  virtual void InitializeRegistered(const plUuid& documentGuid, const plUuid& componentGuid) override;
  virtual const char* GetDisplayName() const override { return "Generate NavMesh"; }
  virtual void GetReplicationInfo(plStringBuilder& out_sReplicationOpType, plStreamWriter& description) override;
  virtual void Finalize(plResult result, const plDataBuffer& resultData) override;

private:
  plUuid m_DocumentGuid;
  plUuid m_ComponentGuid;
};
