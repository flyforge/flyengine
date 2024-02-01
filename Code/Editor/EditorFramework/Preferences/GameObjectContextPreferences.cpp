#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Preferences/GameObjectContextPreferences.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectContextPreferencesUser, 1, plRTTIDefaultAllocator<plGameObjectContextPreferencesUser>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ContextDocument", m_ContextDocument)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("ContextObject", m_ContextObject)->AddAttributes(new plHiddenAttribute),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plGameObjectContextPreferencesUser::plGameObjectContextPreferencesUser()
  : plPreferences(Domain::Document, "GameObjectContext")
{
}

plUuid plGameObjectContextPreferencesUser::GetContextDocument() const
{
  return m_ContextDocument;
}


void plGameObjectContextPreferencesUser::SetContextDocument(plUuid val)
{
  m_ContextDocument = val;
  TriggerPreferencesChangedEvent();
}

plUuid plGameObjectContextPreferencesUser::GetContextObject() const
{
  return m_ContextObject;
}

void plGameObjectContextPreferencesUser::SetContextObject(plUuid val)
{
  m_ContextObject = val;
  TriggerPreferencesChangedEvent();
}
