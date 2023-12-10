#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

/// \brief Stores project specific preferences for the current user
class PLASMA_EDITORFRAMEWORK_DLL plProjectPreferencesUser : public plPreferences
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProjectPreferencesUser, plPreferences);

public:
  plProjectPreferencesUser();

  // which apps to launch as external 'Players' (other than plPlayer.exe)
  plDynamicArray<plString> m_PlayerApps;

  // the directory where the project should be exported to
  plString m_sExportFolder;
};
