#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

template <typename HandleType>
class plEditorGuidEngineHandleMap
{
public:
  void Clear()
  {
    m_GuidToHandle.Clear();
    m_HandleToGuid.Clear();
  }

  void RegisterObject(plUuid guid, HandleType handle)
  {
    auto it = m_GuidToHandle.Find(guid);
    if (it.IsValid())
    {
      // During undo/redo we may register the same object again. In that case, just use the new version.
      UnregisterObject(guid);
    }
    m_GuidToHandle[guid] = handle;
    m_HandleToGuid[handle] = guid;

    PL_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  void UnregisterObject(plUuid guid)
  {
    const HandleType handle = m_GuidToHandle[guid];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);

    PL_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  void UnregisterObject(HandleType handle)
  {
    const plUuid guid = m_HandleToGuid[handle];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);

    PL_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  HandleType GetHandle(plUuid guid) const
  {
    HandleType res = HandleType();
    m_GuidToHandle.TryGetValue(guid, res);
    return res;
  }

  plUuid GetGuid(HandleType handle) const { return m_HandleToGuid.GetValueOrDefault(handle, plUuid()); }

  const plMap<HandleType, plUuid>& GetHandleToGuidMap() const { return m_HandleToGuid; }

private:
  plHashTable<plUuid, HandleType> m_GuidToHandle;
  plMap<HandleType, plUuid> m_HandleToGuid;
};
