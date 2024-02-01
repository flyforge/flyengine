#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Project/ToolsProject.h>

PL_IMPLEMENT_SINGLETON(plToolsProject);

plEvent<const plToolsProjectEvent&> plToolsProject::s_Events;
plEvent<plToolsProjectRequest&> plToolsProject::s_Requests;


plToolsProjectRequest::plToolsProjectRequest()
{
  m_Type = Type::CanCloseProject;
  m_bCanClose = true;
  m_iContainerWindowUniqueIdentifier = 0;
}

plToolsProject::plToolsProject(plStringView sProjectPath)
  : m_SingletonRegistrar(this)
{
  m_bIsClosing = false;

  m_sProjectPath = sProjectPath;
  PL_ASSERT_DEV(!m_sProjectPath.IsEmpty(), "Path cannot be empty.");
}

plToolsProject::~plToolsProject() = default;

plStatus plToolsProject::Create()
{
  {
    plOSFile ProjectFile;
    if (ProjectFile.Open(m_sProjectPath, plFileOpenMode::Write).Failed())
    {
      return plStatus(plFmt("Could not open/create the project file for writing: '{0}'", m_sProjectPath));
    }
    else
    {
      plStringView szToken = "plEditor Project File";

      PL_SUCCEED_OR_RETURN(ProjectFile.Write(szToken.GetStartPointer(), szToken.GetElementCount() + 1));
      ProjectFile.Close();
    }
  }

  plToolsProjectEvent e;
  e.m_pProject = this;
  e.m_Type = plToolsProjectEvent::Type::ProjectCreated;
  s_Events.Broadcast(e);

  return Open();
}

plStatus plToolsProject::Open()
{
  plOSFile ProjectFile;
  if (ProjectFile.Open(m_sProjectPath, plFileOpenMode::Read).Failed())
  {
    return plStatus(plFmt("Could not open the project file for reading: '{0}'", m_sProjectPath));
  }

  ProjectFile.Close();

  plToolsProjectEvent e;
  e.m_pProject = this;
  e.m_Type = plToolsProjectEvent::Type::ProjectOpened;
  s_Events.Broadcast(e);

  return plStatus(PL_SUCCESS);
}

void plToolsProject::CreateSubFolder(plStringView sFolder) const
{
  plStringBuilder sPath;

  sPath = m_sProjectPath;
  sPath.PathParentDirectory();
  sPath.AppendPath(sFolder);

  plOSFile::CreateDirectoryStructure(sPath).IgnoreResult();
}

void plToolsProject::CloseProject()
{
  if (GetSingleton())
  {
    GetSingleton()->m_bIsClosing = true;

    plToolsProjectEvent e;
    e.m_pProject = GetSingleton();
    e.m_Type = plToolsProjectEvent::Type::ProjectClosing;
    s_Events.Broadcast(e);

    plDocumentManager::CloseAllDocuments();

    delete GetSingleton();

    e.m_Type = plToolsProjectEvent::Type::ProjectClosed;
    s_Events.Broadcast(e);
  }
}

void plToolsProject::SaveProjectState()
{
  if (GetSingleton())
  {
    GetSingleton()->m_bIsClosing = true;

    plToolsProjectEvent e;
    e.m_pProject = GetSingleton();
    e.m_Type = plToolsProjectEvent::Type::ProjectSaveState;
    s_Events.Broadcast(e, 1);
  }
}

bool plToolsProject::CanCloseProject()
{
  if (GetSingleton() == nullptr)
    return true;

  plToolsProjectRequest e;
  e.m_Type = plToolsProjectRequest::Type::CanCloseProject;
  e.m_bCanClose = true;
  s_Requests.Broadcast(e, 1); // when the save dialog pops up and the user presses 'Save' we need to allow one more recursion

  return e.m_bCanClose;
}

bool plToolsProject::CanCloseDocuments(plArrayPtr<plDocument*> documents)
{
  if (GetSingleton() == nullptr)
    return true;

  plToolsProjectRequest e;
  e.m_Type = plToolsProjectRequest::Type::CanCloseDocuments;
  e.m_bCanClose = true;
  e.m_Documents = documents;
  s_Requests.Broadcast(e);

  return e.m_bCanClose;
}

plInt32 plToolsProject::SuggestContainerWindow(plDocument* pDoc)
{
  if (pDoc == nullptr)
  {
    return 0;
  }
  plToolsProjectRequest e;
  e.m_Type = plToolsProjectRequest::Type::SuggestContainerWindow;
  e.m_Documents.PushBack(pDoc);
  s_Requests.Broadcast(e);

  return e.m_iContainerWindowUniqueIdentifier;
}

plStringBuilder plToolsProject::GetPathForDocumentGuid(const plUuid& guid)
{
  plToolsProjectRequest e;
  e.m_Type = plToolsProjectRequest::Type::GetPathForDocumentGuid;
  e.m_documentGuid = guid;
  s_Requests.Broadcast(e, 1); // this can be sent while CanCloseProject is processed, so allow one additional recursion depth
  return e.m_sAbsDocumentPath;
}

plStatus plToolsProject::CreateOrOpenProject(plStringView sProjectPath, bool bCreate)
{
  CloseProject();

  new plToolsProject(sProjectPath);

  plStatus ret;

  if (bCreate)
  {
    ret = GetSingleton()->Create();
    plToolsProject::SaveProjectState();
  }
  else
    ret = GetSingleton()->Open();

  if (ret.m_Result.Failed())
  {
    delete GetSingleton();
    return ret;
  }

  return plStatus(PL_SUCCESS);
}

plStatus plToolsProject::OpenProject(plStringView sProjectPath)
{
  plStatus status = CreateOrOpenProject(sProjectPath, false);

  return status;
}

plStatus plToolsProject::CreateProject(plStringView sProjectPath)
{
  return CreateOrOpenProject(sProjectPath, true);
}

void plToolsProject::BroadcastSaveAll()
{
  plToolsProjectEvent e;
  e.m_pProject = GetSingleton();
  e.m_Type = plToolsProjectEvent::Type::SaveAll;

  s_Events.Broadcast(e);
}

void plToolsProject::BroadcastConfigChanged()
{
  plToolsProjectEvent e;
  e.m_pProject = GetSingleton();
  e.m_Type = plToolsProjectEvent::Type::ProjectConfigChanged;

  s_Events.Broadcast(e);
}

void plToolsProject::AddAllowedDocumentRoot(plStringView sPath)
{
  plStringBuilder s = sPath;
  s.MakeCleanPath();
  s.Trim("", "/");

  m_AllowedDocumentRoots.PushBack(s);
}


bool plToolsProject::IsDocumentInAllowedRoot(plStringView sDocumentPath, plString* out_pRelativePath) const
{
  for (plUInt32 i = m_AllowedDocumentRoots.GetCount(); i > 0; --i)
  {
    const auto& root = m_AllowedDocumentRoots[i - 1];

    plStringBuilder s = sDocumentPath;
    if (!s.IsPathBelowFolder(root))
      continue;

    if (out_pRelativePath)
    {
      plStringBuilder sText = sDocumentPath;
      sText.MakeRelativeTo(root).IgnoreResult();

      *out_pRelativePath = sText;
    }

    return true;
  }

  return false;
}

const plString plToolsProject::GetProjectName(bool bSanitize) const
{
  plStringBuilder sTemp = plToolsProject::GetSingleton()->GetProjectFile();
  sTemp.PathParentDirectory();
  sTemp.Trim("/");

  if (!bSanitize)
    return sTemp.GetFileName();

  const plStringBuilder sOrgName = sTemp.GetFileName();
  sTemp.Clear();

  bool bAnyAscii = false;

  for (plStringIterator it = sOrgName.GetIteratorFront(); it.IsValid(); ++it)
  {
    const plUInt32 c = it.GetCharacter();

    if (!plStringUtils::IsIdentifierDelimiter_C_Code(c))
    {
      bAnyAscii = true;

      // valid character to be used in C as an identifier
      sTemp.Append(c);
    }
    else if (c == ' ')
    {
      // skip
    }
    else
    {
      sTemp.AppendFormat("{}", plArgU(c, 1, false, 16));
    }
  }

  if (!bAnyAscii)
  {
    const plUInt32 uiHash = plHashingUtils::xxHash32String(sTemp);
    sTemp.SetFormat("Project{}", uiHash);
  }

  if (sTemp.IsEmpty())
  {
    sTemp = "Project";
  }

  if (sTemp.GetCharacterCount() > 20)
  {
    sTemp.Shrink(0, sTemp.GetCharacterCount() - 20);
  }

  return sTemp;
}

plString plToolsProject::GetProjectDirectory() const
{
  plStringBuilder s = GetProjectFile();

  s.PathParentDirectory();
  s.Trim("", "/\\");

  return s;
}

plString plToolsProject::GetProjectDataFolder() const
{
  plStringBuilder s = GetProjectFile();
  s.Append("_data");

  return s;
}

plString plToolsProject::FindProjectDirectoryForDocument(plStringView sDocumentPath)
{
  plStringBuilder sPath = sDocumentPath;
  sPath.PathParentDirectory();

  plStringBuilder sTemp;

  while (!sPath.IsEmpty())
  {
    sTemp = sPath;
    sTemp.AppendPath("plProject");

    if (plOSFile::ExistsFile(sTemp))
      return sPath;

    sPath.PathParentDirectory();
  }

  return "";
}
