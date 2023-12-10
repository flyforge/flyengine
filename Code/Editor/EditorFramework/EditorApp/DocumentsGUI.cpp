#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

void plQtEditorApp::GuiCreateOrOpenDocument(bool bCreate)
{
  const plString sAllFilters = BuildDocumentTypeFileFilter(bCreate);

  if (sAllFilters.IsEmpty())
  {
    plQtUiServices::MessageBoxInformation("No file types are currently known. Load plugins to add file types.");
    return;
  }

  static QString sSelectedExt;
  const QString sDir = QString::fromUtf8(m_sLastDocumentFolder.GetData());

  plString sFile;

  if (bCreate)
    sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Document"), sDir,
      QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt, QFileDialog::Option::DontResolveSymlinks)
              .toUtf8()
              .data();
  else
    sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open Document"), sDir, QString::fromUtf8(sAllFilters.GetData()),
      &sSelectedExt, QFileDialog::Option::DontResolveSymlinks)
              .toUtf8()
              .data();

  if (sFile.IsEmpty())
    return;

  m_sLastDocumentFolder = plPathUtils::GetFileDirectory(sFile);

  const plDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (plDocumentManager::FindDocumentTypeFromPath(sFile, bCreate, pTypeDesc).Succeeded())
  {
    sSelectedExt = pTypeDesc->m_sDocumentTypeName;
  }

  if (bCreate)
    CreateDocument(sFile, plDocumentFlags::AddToRecentFilesList | plDocumentFlags::RequestWindow);
  else
    OpenDocument(sFile, plDocumentFlags::AddToRecentFilesList | plDocumentFlags::RequestWindow);
}

void plQtEditorApp::GuiCreateDocument()
{
  GuiCreateOrOpenDocument(true);
}

void plQtEditorApp::GuiOpenDocument()
{
  GuiCreateOrOpenDocument(false);
}


plString plQtEditorApp::BuildDocumentTypeFileFilter(bool bForCreation)
{
  plStringBuilder sAllFilters;
  const char* sepsep = "";

  if (!bForCreation)
  {
    sAllFilters = "All Files (*.*)";
    sepsep = ";;";
  }

  const auto& assetTypes = plDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  plMap<plString, const plDocumentTypeDescriptor*> allDesc;
  for (auto it : assetTypes)
  {
    allDesc[plTranslate(it.Key())] = it.Value();
  }

  for (auto it : allDesc)
  {
    auto desc = it.Value();

    if (bForCreation && !desc->m_bCanCreate)
      continue;

    if (desc->m_sFileExtension.IsEmpty())
      continue;

    sAllFilters.Append(sepsep, plTranslate(desc->m_sDocumentTypeName), " (*.", desc->m_sFileExtension, ")");
    sepsep = ";;";
  }

  return sAllFilters;
}


void plQtEditorApp::DocumentWindowEventHandler(const plQtDocumentWindowEvent& e)
{
  switch (e.m_Type)
  {
    case plQtDocumentWindowEvent::WindowClosed:
    {
      // if all windows are closed, show at least the settings window
      if (plQtDocumentWindow::GetAllDocumentWindows().GetCount() == 0)
      {
        ShowSettingsDocument();
      }
    }
    break;

    default:
      break;
  }
}
