#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plWorld;

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL PlasmaEditorEngineSyncObject : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(PlasmaEditorEngineSyncObject, plReflectedClass);

public:
  PlasmaEditorEngineSyncObject();
  ~PlasmaEditorEngineSyncObject();

  void Configure(plUuid ownerGuid, plDelegate<void(PlasmaEditorEngineSyncObject*)> onDestruction);

  plUuid GetDocumentGuid() const;
  void SetModified(bool b = true) { m_bModified = b; }
  bool GetModified() const { return m_bModified; }

  plUuid GetGuid() const { return m_SyncObjectGuid; }

  // \brief One-time setup on the engine side.
  // \returns Whether the sync object is pickable via uiNextComponentPickingID.
  virtual bool SetupForEngine(plWorld* pWorld, plUInt32 uiNextComponentPickingID) { return false; }
  virtual void UpdateForEngine(plWorld* pWorld) {}

private:
  PLASMA_ALLOW_PRIVATE_PROPERTIES(PlasmaEditorEngineSyncObject);

  friend class plAssetDocument;

  bool m_bModified;
  plUuid m_SyncObjectGuid;
  plUuid m_OwnerGuid;

  plDelegate<void(PlasmaEditorEngineSyncObject*)> m_OnDestruction;
};
