#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

plStatus plQtEditorApp::SaveTagRegistry()
{
  PLASMA_LOG_BLOCK("plQtEditorApp::SaveTagRegistry()");

  plStringBuilder sPath;
  sPath = plToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/Tags.ddl");

  plDeferredFileWriter file;
  file.SetOutput(sPath);

  plToolsTagRegistry::WriteToDDL(file);

  if (file.Close().Failed())
  {
    return plStatus(plFmt("Could not open tags config file '{0}' for writing", sPath));
  }
  return plStatus(PLASMA_SUCCESS);
}

void plQtEditorApp::ReadTagRegistry()
{
  PLASMA_LOG_BLOCK("plQtEditorApp::ReadTagRegistry");

  plToolsTagRegistry::Clear();

  plStringBuilder sPath;
  sPath = plToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/Tags.ddl");

#if PLASMA_ENABLED(PLASMA_MIGRATE_RUNTIMECONFIGS)
  plStringBuilder sOldPath;
  sOldPath = plToolsProject::GetSingleton()->GetProjectDirectory();
  sOldPath.AppendPath("Tags.ddl");
  sPath = plFileSystem::MigrateFileLocation(sOldPath, sPath);
#endif

  plFileReader file;
  if (file.Open(sPath).Failed())
  {
    plLog::Warning("Could not open tags config file '{0}'", sPath);

    plStatus res = SaveTagRegistry();
    if (res.m_Result.Failed())
    {
      plLog::Error("{0}", res.m_sMessage);
    }
  }
  else
  {
    plStatus res = plToolsTagRegistry::ReadFromDDL(file);
    if (res.m_Result.Failed())
    {
      plLog::Error("{0}", res.m_sMessage);
    }
  }


  // TODO: Add default tags
  plToolsTag tag;
  tag.m_sName = "EditorHidden";
  tag.m_sCategory = "Editor";
  plToolsTagRegistry::AddTag(tag);
}
