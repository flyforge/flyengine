#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Preferences/GameObjectContextPreferences.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectContextPreferencesUser, 1, plRTTIDefaultAllocator<plGameObjectContextPreferencesUser>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ContextDocument", m_ContextDocument)->AddAttributes(new plHiddenAttribute),
    PLASMA_MEMBER_PROPERTY("ContextObject", m_ContextObject)->AddAttributes(new plHiddenAttribute),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
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
