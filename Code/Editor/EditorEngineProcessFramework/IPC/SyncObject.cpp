#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/SyncObject.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(PlasmaEditorEngineSyncObject, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("SyncGuid", m_SyncObjectGuid),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PlasmaEditorEngineSyncObject::PlasmaEditorEngineSyncObject()
{
  m_SyncObjectGuid.CreateNewUuid();
  m_bModified = true;
}

PlasmaEditorEngineSyncObject::~PlasmaEditorEngineSyncObject()
{
  if (m_OnDestruction.IsValid())
  {
    m_OnDestruction(this);
  }
}

void PlasmaEditorEngineSyncObject::Configure(plUuid ownerGuid, plDelegate<void(PlasmaEditorEngineSyncObject*)> onDestruction)
{
  m_OwnerGuid = ownerGuid;
  m_OnDestruction = onDestruction;
}

plUuid PlasmaEditorEngineSyncObject::GetDocumentGuid() const
{
  return m_OwnerGuid;
}
