#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/Profiling/Profiling.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProjectPreferencesUser, 1, plRTTIDefaultAllocator<plProjectPreferencesUser>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("Players", m_PlayerApps)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("ExportFolder", m_sExportFolder)->AddAttributes(new plHiddenAttribute()),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plProjectPreferencesUser::plProjectPreferencesUser()
  : plPreferences(Domain::Project, "General")
{
}


void plQtEditorApp::LoadProjectPreferences()
{
  PL_PROFILE_SCOPE("LoadProjectPreferences");
  plPreferences::QueryPreferences<plProjectPreferencesUser>();
}
