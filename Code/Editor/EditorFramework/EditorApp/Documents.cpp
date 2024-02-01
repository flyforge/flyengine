#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Document/DocumentUtils.h>

void plQtEditorApp::OpenDocumentQueued(plStringView sDocument, const plDocumentObject* pOpenContext /*= nullptr*/)
{
  QMetaObject::invokeMethod(this, "SlotQueuedOpenDocument", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, plMakeQString(sDocument)), Q_ARG(void*, (void*)pOpenContext));
}

plDocument* plQtEditorApp::OpenDocument(plStringView sDocument, plBitflags<plDocumentFlags> flags, const plDocumentObject* pOpenContext)
{
  PL_PROFILE_SCOPE("OpenDocument");

  if (IsInHeadlessMode())
    flags.Remove(plDocumentFlags::RequestWindow);

  const plDocumentTypeDescriptor* pTypeDesc = nullptr;

  if (plDocumentManager::FindDocumentTypeFromPath(sDocument, false, pTypeDesc).Failed())
  {
    plStringBuilder sTemp;
    sTemp.SetFormat("The selected file extension '{0}' is not registered with any known type.\nCannot open file '{1}'", plPathUtils::GetFileExtension(sDocument), sDocument);
    plQtUiServices::MessageBoxWarning(sTemp);
    return nullptr;
  }

  // does the same document already exist and is open ?
  plDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(sDocument);
  if (!pDocument)
  {
    plStatus res = pTypeDesc->m_pManager->CanOpenDocument(sDocument);
    if (res.m_Result.Succeeded())
    {
      res = pTypeDesc->m_pManager->OpenDocument(pTypeDesc->m_sDocumentTypeName, sDocument, pDocument, flags, pOpenContext);
    }

    if (res.m_Result.Failed())
    {
      plStringBuilder s;
      s.SetFormat("Failed to open document: \n'{0}'", sDocument);
      plQtUiServices::MessageBoxStatus(res, s);
      return nullptr;
    }

    PL_ASSERT_DEV(pDocument != nullptr, "Opening of document type '{0}' succeeded, but returned pointer is nullptr", pTypeDesc->m_sDocumentTypeName);

    if (pDocument->GetUnknownObjectTypeInstances() > 0)
    {
      plStringBuilder s;
      s.SetFormat("The document contained {0} objects of an unknown type. Necessary plugins may be missing.\n\n\
If you save this document, all data for these objects is lost permanently!\n\n\
The following types are missing:\n",
        pDocument->GetUnknownObjectTypeInstances());

      for (auto it = pDocument->GetUnknownObjectTypes().GetIterator(); it.IsValid(); ++it)
      {
        s.AppendFormat(" '{0}' ", (*it));
      }
      plQtUiServices::MessageBoxWarning(s);
    }
  }

  if (flags.IsSet(plDocumentFlags::RequestWindow))
  {
    plQtContainerWindow::EnsureVisibleAnyContainer(pDocument).IgnoreResult();
  }

  return pDocument;
}

plDocument* plQtEditorApp::CreateDocument(plStringView sDocument, plBitflags<plDocumentFlags> flags, const plDocumentObject* pOpenContext)
{
  PL_PROFILE_SCOPE("CreateDocument");

  if (IsInHeadlessMode())
    flags.Remove(plDocumentFlags::RequestWindow);

  const plDocumentTypeDescriptor* pTypeDesc = nullptr;

  {
    plStatus res = plDocumentUtils::IsValidSaveLocationForDocument(sDocument, &pTypeDesc);
    if (res.Failed())
    {
      plStringBuilder s;
      s.SetFormat("Failed to create document: \n'{0}'", sDocument);
      plQtUiServices::MessageBoxStatus(res, s);
      return nullptr;
    }
  }

  plDocument* pDocument = nullptr;
  {
    plStatus result = pTypeDesc->m_pManager->CreateDocument(pTypeDesc->m_sDocumentTypeName, sDocument, pDocument, flags, pOpenContext);
    if (result.m_Result.Failed())
    {
      plStringBuilder s;
      s.SetFormat("Failed to create document: \n'{0}'", sDocument);
      plQtUiServices::MessageBoxStatus(result, s);
      return nullptr;
    }

    PL_ASSERT_DEV(pDocument != nullptr, "Creation of document type '{0}' succeeded, but returned pointer is nullptr", pTypeDesc->m_sDocumentTypeName);
    PL_ASSERT_DEV(pDocument->GetUnknownObjectTypeInstances() == 0, "Newly created documents should not contain unknown types.");
  }


  if (flags.IsSet(plDocumentFlags::RequestWindow))
  {
    plQtContainerWindow::EnsureVisibleAnyContainer(pDocument).IgnoreResult();
  }

  return pDocument;
}

void plQtEditorApp::SlotQueuedOpenDocument(QString sProject, void* pOpenContext)
{
  OpenDocument(sProject.toUtf8().data(), plDocumentFlags::RequestWindow | plDocumentFlags::AddToRecentFilesList, static_cast<const plDocumentObject*>(pOpenContext));
}

void plQtEditorApp::DocumentEventHandler(const plDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case plDocumentEvent::Type::DocumentSaved:
    {
      plPreferences::SaveDocumentPreferences(e.m_pDocument);
    }
    break;

    default:
      break;
  }
}


void plQtEditorApp::DocumentManagerEventHandler(const plDocumentManager::Event& r)
{
  switch (r.m_Type)
  {
    case plDocumentManager::Event::Type::AfterDocumentWindowRequested:
    {
      if (r.m_pDocument->GetAddToRecentFilesList())
      {
        m_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath(), 0);
        if (!m_bLoadingProjectInProgress)
        {
          SaveOpenDocumentsList();
        }
      }
    }
    break;

    case plDocumentManager::Event::Type::DocumentClosing2:
    {
      plPreferences::SaveDocumentPreferences(r.m_pDocument);
      plPreferences::ClearDocumentPreferences(r.m_pDocument);
    }
    break;

    case plDocumentManager::Event::Type::DocumentClosing:
    {
      if (r.m_pDocument->GetAddToRecentFilesList())
      {
        // again, insert it into the recent documents list, such that the LAST CLOSED document is the LAST USED
        m_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath(), 0);
      }
    }
    break;

    default:
      break;
  }
}



void plQtEditorApp::DocumentManagerRequestHandler(plDocumentManager::Request& r)
{
  switch (r.m_Type)
  {
    case plDocumentManager::Request::Type::DocumentAllowedToOpen:
    {
      // if someone else already said no, don't bother to check further
      if (r.m_RequestStatus.m_Result.Failed())
        return;

      if (!plToolsProject::IsProjectOpen())
      {
        // if no project is open yet, try to open the corresponding one

        plStringBuilder sProjectPath = plToolsProject::FindProjectDirectoryForDocument(r.m_sDocumentPath);

        // if no project could be located, just reject the request
        if (sProjectPath.IsEmpty())
        {
          r.m_RequestStatus = plStatus("No project could be opened");
          return;
        }
        else
        {
          // append the project file
          sProjectPath.AppendPath("plProject");

          // if a project could be found, try to open it
          plStatus res = plToolsProject::OpenProject(sProjectPath);

          // if project opening failed, relay that error message
          if (res.m_Result.Failed())
          {
            r.m_RequestStatus = res;
            return;
          }
        }
      }
      else
      {
        if (!plToolsProject::GetSingleton()->IsDocumentInAllowedRoot(r.m_sDocumentPath))
        {
          r.m_RequestStatus = plStatus("The document is not part of the currently open project");
          return;
        }
      }
    }
      return;
  }
}
