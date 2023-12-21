#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Project/ToolsProject.h>

PLASMA_IMPLEMENT_SINGLETON(plApplicationServices);

static plApplicationServices g_instance;

plApplicationServices::plApplicationServices()
  : m_SingletonRegistrar(this)
{
}

plString plApplicationServices::GetApplicationUserDataFolder() const
{
  plStringBuilder path = plOSFile::GetUserDataFolder();
  path.AppendPath("PlasmaEngine Project", plApplication::GetApplicationInstance()->GetApplicationName());
  path.MakeCleanPath();

  return path;
}

plString plApplicationServices::GetApplicationDataFolder() const
{
  plStringBuilder sAppDir(">sdk/Data/Tools/", plApplication::GetApplicationInstance()->GetApplicationName());

  plStringBuilder result;
  plFileSystem::ResolveSpecialDirectory(sAppDir, result).IgnoreResult();
  result.MakeCleanPath();

  return result;
}

plString plApplicationServices::GetApplicationPreferencesFolder() const
{
  return GetApplicationUserDataFolder();
}

plString plApplicationServices::GetProjectPreferencesFolder() const
{
  return GetProjectPreferencesFolder(plToolsProject::GetSingleton()->GetProjectDirectory());
}

plString plApplicationServices::GetProjectPreferencesFolder(plStringView sProjectFilePath) const
{
  plStringBuilder path = GetApplicationUserDataFolder();

  sProjectFilePath.TrimWordEnd("plProject");
  sProjectFilePath.TrimWordEnd("plRemoteProject");
  sProjectFilePath.Trim("/\\");

  plStringBuilder ProjectName = sProjectFilePath;

  plStringBuilder ProjectPath = ProjectName;
  ProjectPath.PathParentDirectory();

  const plUInt64 uiPathHash = plHashingUtils::StringHash(ProjectPath.GetData());

  ProjectName = ProjectName.GetFileName();

  path.AppendFormat("/Projects/{}_{}", uiPathHash, ProjectName);

  path.MakeCleanPath();
  return path;
}

plString plApplicationServices::GetDocumentPreferencesFolder(const plDocument* pDocument) const
{
  plStringBuilder path = GetProjectPreferencesFolder();

  plStringBuilder sGuid;
  plConversionUtils::ToString(pDocument->GetGuid(), sGuid);

  path.AppendPath(sGuid);

  path.MakeCleanPath();
  return path;
}

plString plApplicationServices::GetPrecompiledToolsFolder(bool bUsePrecompiledTools) const
{
  plStringBuilder sPath = plOSFile::GetApplicationDirectory();

  if (bUsePrecompiledTools)
  {
    sPath.AppendPath("../../../Data/Tools/Precompiled");
  }

  sPath.MakeCleanPath();

  return sPath;
}

plString plApplicationServices::GetSampleProjectsFolder() const
{
  plStringBuilder sPath = plOSFile::GetApplicationDirectory();

  sPath.AppendPath("../../../Data/Samples");

  sPath.MakeCleanPath();

  return sPath;
}
