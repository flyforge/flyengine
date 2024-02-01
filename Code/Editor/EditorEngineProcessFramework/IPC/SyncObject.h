#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plWorld;

class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plEditorEngineSyncObject : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plEditorEngineSyncObject, plReflectedClass);

public:
  plEditorEngineSyncObject();
  ~plEditorEngineSyncObject();

  void Configure(plUuid ownerGuid, plDelegate<void(plEditorEngineSyncObject*)> onDestruction);

  plUuid GetDocumentGuid() const;
  void SetModified(bool b = true) { m_bModified = b; }
  bool GetModified() const { return m_bModified; }

  plUuid GetGuid() const { return m_SyncObjectGuid; }

  // \brief One-time setup on the engine side.
  // \returns Whether the sync object is pickable via uiNextComponentPickingID.
  virtual bool SetupForEngine(plWorld* pWorld, plUInt32 uiNextComponentPickingID) { return false; }
  virtual void UpdateForEngine(plWorld* pWorld) {}

private:
  PL_ALLOW_PRIVATE_PROPERTIES(plEditorEngineSyncObject);

  friend class plAssetDocument;

  bool m_bModified;
  plUuid m_SyncObjectGuid;
  plUuid m_OwnerGuid;

  plDelegate<void(plEditorEngineSyncObject*)> m_OnDestruction;
};
