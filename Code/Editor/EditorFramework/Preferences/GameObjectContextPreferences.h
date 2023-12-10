#pragma once

#include <EditorFramework/Preferences/Preferences.h>

class plGameObjectContextPreferencesUser : public plPreferences
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameObjectContextPreferencesUser, plPreferences);

public:
  plGameObjectContextPreferencesUser();

  plUuid GetContextDocument() const;
  void SetContextDocument(plUuid val);
  plUuid GetContextObject() const;
  void SetContextObject(plUuid val);

protected:
  plUuid m_ContextDocument;
  plUuid m_ContextObject;
};
