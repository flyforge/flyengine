#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/SyncObject.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorEngineSyncObject, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("SyncGuid", m_SyncObjectGuid),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plEditorEngineSyncObject::plEditorEngineSyncObject()
{
  m_SyncObjectGuid = plUuid::MakeUuid();
  m_bModified = true;
}

plEditorEngineSyncObject::~plEditorEngineSyncObject()
{
  if (m_OnDestruction.IsValid())
  {
    m_OnDestruction(this);
  }
}

void plEditorEngineSyncObject::Configure(plUuid ownerGuid, plDelegate<void(plEditorEngineSyncObject*)> onDestruction)
{
  m_OwnerGuid = ownerGuid;
  m_OnDestruction = onDestruction;
}

plUuid plEditorEngineSyncObject::GetDocumentGuid() const
{
  return m_OwnerGuid;
}
