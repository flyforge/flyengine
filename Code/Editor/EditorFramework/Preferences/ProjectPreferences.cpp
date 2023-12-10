#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/Profiling/Profiling.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProjectPreferencesUser, 1, plRTTIDefaultAllocator<plProjectPreferencesUser>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Players", m_PlayerApps)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("ExportFolder", m_sExportFolder)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plProjectPreferencesUser::plProjectPreferencesUser()
  : plPreferences(Domain::Project, "General")
{
}


void plQtEditorApp::LoadProjectPreferences()
{
  PLASMA_PROFILE_SCOPE("LoadProjectPreferences");
  plPreferences::QueryPreferences<plProjectPreferencesUser>();
}
