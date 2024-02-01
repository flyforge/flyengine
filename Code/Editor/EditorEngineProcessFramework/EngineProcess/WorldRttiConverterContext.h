#pragma once

#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/GuidHandleMap.h>
#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>

/// \brief The world rtti converter context tracks created objects and is capable of also handling
///  components / game objects. Used by the plIPCObjectMirror to create / destroy objects.
///
/// Atm it does not remove owner ptr when a parent is deleted, so it will accumulate zombie entries.
/// As requests to dead objects shouldn't generally happen this is for the time being not a problem.
class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plWorldRttiConverterContext : public plRttiConverterContext
{
public:
  virtual void Clear() override;
  void DeleteExistingObjects();

  virtual plInternal::NewInstance<void> CreateObject(const plUuid& guid, const plRTTI* pRtti) override;
  virtual void DeleteObject(const plUuid& guid) override;

  virtual void RegisterObject(const plUuid& guid, const plRTTI* pRtti, void* pObject) override;
  virtual void UnregisterObject(const plUuid& guid) override;

  virtual plRttiConverterObject GetObjectByGUID(const plUuid& guid) const override;
  virtual plUuid GetObjectGUID(const plRTTI* pRtti, const void* pObject) const override;

  virtual void OnUnknownTypeError(plStringView sTypeName) override;

  plWorld* m_pWorld = nullptr;
  plEditorGuidEngineHandleMap<plGameObjectHandle> m_GameObjectMap;
  plEditorGuidEngineHandleMap<plComponentHandle> m_ComponentMap;

  plEditorGuidEngineHandleMap<plUInt32> m_OtherPickingMap;
  plEditorGuidEngineHandleMap<plUInt32> m_ComponentPickingMap;
  plUInt32 m_uiNextComponentPickingID = 1;
  plUInt32 m_uiHighlightID = 1;

  struct Event
  {
    enum class Type
    {
      GameObjectCreated,
      GameObjectDeleted,
    };

    Type m_Type;
    plUuid m_ObjectGuid;
  };

  plEvent<const Event&> m_Events;

  plSet<plString> m_UnknownTypes;
};
