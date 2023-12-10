#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

void plQtEditorApp::GuiOpenDashboard()
{
  QMetaObject::invokeMethod(this, "SlotQueuedGuiOpenDashboard", Qt::ConnectionType::QueuedConnection);
}

void plQtEditorApp::GuiOpenDocsAndCommunity()
{
  QMetaObject::invokeMethod(this, "SlotQueuedGuiOpenDocsAndCommunity", Qt::ConnectionType::QueuedConnection);
}

bool plQtEditorApp::GuiCreateProject(bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return GuiCreateOrOpenProject(true);
  }
  else
  {
    QMetaObject::invokeMethod(this, "SlotQueuedGuiCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, true));
    return true;
  }
}

bool plQtEditorApp::GuiOpenProject(bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return GuiCreateOrOpenProject(false);
  }
  else
  {
    QMetaObject::invokeMethod(this, "SlotQueuedGuiCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, false));
    return true;
  }
}

void plQtEditorApp::SlotQueuedGuiOpenDashboard()
{
  plQtDashboardDlg dlg(nullptr, plQtDashboardDlg::DashboardTab::Projects);
  dlg.exec();
}

void plQtEditorApp::SlotQueuedGuiOpenDocsAndCommunity()
{
  plQtDashboardDlg dlg(nullptr, plQtDashboardDlg::DashboardTab::Documentation);
  dlg.exec();
}

void plQtEditorApp::SlotQueuedGuiCreateOrOpenProject(bool bCreate)
{
  GuiCreateOrOpenProject(bCreate);
}

bool plQtEditorApp::GuiCreateOrOpenProject(bool bCreate)
{
  const QString sDir = QString::fromUtf8(m_sLastProjectFolder.GetData());
  plStringBuilder sFile;

  const char* szFilter = "plProject (plProject)";

  if (bCreate)
    sFile = QFileDialog::getExistingDirectory(
      QApplication::activeWindow(), QLatin1String("Choose Folder for New Project"), sDir, QFileDialog::Option::DontResolveSymlinks)
              .toUtf8()
              .data();
  else
    sFile = QFileDialog::getOpenFileName(
      QApplication::activeWindow(), QLatin1String("Open Project"), sDir, QLatin1String(szFilter), nullptr, QFileDialog::Option::DontResolveSymlinks)
              .toUtf8()
              .data();

  if (sFile.IsEmpty())
    return false;

  if (bCreate)
    sFile.AppendPath("plProject");

  m_sLastProjectFolder = plPathUtils::GetFileDirectory(sFile);

  return CreateOrOpenProject(bCreate, sFile).Succeeded();
}
