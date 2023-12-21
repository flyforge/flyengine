#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

plQtDashboardDlg::plQtDashboardDlg(QWidget* parent, DashboardTab activeTab)
  : QDialog(parent)
{
  setupUi(this);

  TabArea->tabBar()->hide();

  ProjectsTab->setFlat(true);
  SamplesTab->setFlat(true);
  DocumentationTab->setFlat(true);

  if (PlasmaEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<PlasmaEditorPreferencesUser>())
  {
    LoadLastProject->setChecked(pPreferences->m_bLoadLastProjectAtStartup);
  }

  {
    SamplesList->setResizeMode(QListView::ResizeMode::Adjust);
    SamplesList->setIconSize(QSize(220, 220));
    SamplesList->setItemAlignment(Qt::AlignHCenter | Qt::AlignBottom);
  }

  FillRecentProjectsList();
  FillSampleProjectsList();

  ProjectsList->installEventFilter(this);

  if (ProjectsList->rowCount() > 0)
  {
    ProjectsList->setFocus();
    ProjectsList->clearSelection();
    ProjectsList->selectRow(0);
  }
  else
  {
    if (activeTab == DashboardTab::Projects)
    {
      activeTab = DashboardTab::Samples;
    }
  }

  SetActiveTab(activeTab);
}

void plQtDashboardDlg::SetActiveTab(DashboardTab activeTab)
{
  TabArea->setCurrentIndex((int)activeTab);

  ProjectsTab->setChecked(activeTab == DashboardTab::Projects);
  SamplesTab->setChecked(activeTab == DashboardTab::Samples);
  DocumentationTab->setChecked(activeTab == DashboardTab::Documentation);
}

void plQtDashboardDlg::FillRecentProjectsList()
{
  const auto& list = plQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList();

  ProjectsList->clear();
  ProjectsList->setColumnCount(2);
  ProjectsList->setRowCount(list.GetCount());

  plStringBuilder tmp;

  for (plUInt32 r = 0; r < list.GetCount(); ++r)
  {
    const auto& path = list[r];

    QTableWidgetItem* pItemProjectName = new QTableWidgetItem();
    QTableWidgetItem* pItemProjectPath = new QTableWidgetItem();

    pItemProjectName->setData(Qt::UserRole, path.m_File.GetData());

    tmp = path.m_File;
    tmp.MakeCleanPath();
    tmp.PathParentDirectory(1); // remove '/plProject'
    tmp.Trim("/");

    pItemProjectPath->setText(tmp.GetData());
    pItemProjectName->setText(tmp.GetFileName().GetStartPointer());

    ProjectsList->setItem(r, 0, pItemProjectName);
    ProjectsList->setItem(r, 1, pItemProjectPath);
  }

  ProjectsList->resizeColumnToContents(0);
}

void plQtDashboardDlg::FillSampleProjectsList()
{
  plHybridArray<plString, 32> samples;
  FindSampleProjects(samples);

  SamplesList->clear();

  plStringBuilder tmp, iconPath;

  plStringBuilder samplesIcon = plApplicationServices::GetSingleton()->GetSampleProjectsFolder();
  samplesIcon.AppendPath("Thumbnail.jpg");

  QIcon fallbackIcon;

  if (plOSFile::ExistsFile(samplesIcon))
  {
    fallbackIcon.addFile(samplesIcon.GetData());
  }

  for (const plString& path : samples)
  {
    tmp = path;
    const bool bIsLocal = tmp.TrimWordEnd("/plProject");
    const bool bIsRemote = tmp.TrimWordEnd("/plRemoteProject");


    QIcon projectIcon;

    iconPath = tmp;
    iconPath.AppendPath("Thumbnail.jpg");

    if (plOSFile::ExistsFile(iconPath))
    {
      projectIcon.addFile(iconPath.GetData());
    }
    else
    {
      projectIcon = fallbackIcon;
    }

    QListWidgetItem* pItem = new QListWidgetItem();
    pItem->setText(tmp.GetFileName().GetStartPointer());
    pItem->setData(Qt::UserRole, path.GetData());

    pItem->setIcon(projectIcon);

    SamplesList->addItem(pItem);
  }
}

void plQtDashboardDlg::FindSampleProjects(plDynamicArray<plString>& out_Projects)
{
  out_Projects.Clear();

  const plString& sSampleProjects = plApplicationServices::GetSingleton()->GetSampleProjectsFolder();

  plFileSystemIterator fsIt;
  fsIt.StartSearch(sSampleProjects, plFileSystemIteratorFlags::ReportFoldersRecursive);

  plStringBuilder path;

  while (fsIt.IsValid())
  {
    fsIt.GetStats().GetFullPath(path);
    path.AppendPath("plProject");

    if (plOSFile::ExistsFile(path))
    {
      out_Projects.PushBack(path);
      // no need to go deeper
      fsIt.SkipFolder();
    }
    else
    {
      fsIt.GetStats().GetFullPath(path);
      path.AppendPath("plRemoteProject");

      if (plOSFile::ExistsFile(path))
      {
        out_Projects.PushBack(path);
      // no need to go deeper
        fsIt.SkipFolder();
      }
      else
      {
        fsIt.Next();
      }
    }
  }
}

void plQtDashboardDlg::on_ProjectsTab_clicked()
{
  SetActiveTab(DashboardTab::Projects);
}

void plQtDashboardDlg::on_SamplesTab_clicked()
{
  SetActiveTab(DashboardTab::Samples);
}

void plQtDashboardDlg::on_DocumentationTab_clicked()
{
  SetActiveTab(DashboardTab::Documentation);
}

void plQtDashboardDlg::on_NewProject_clicked()
{
  if (plQtEditorApp::GetSingleton()->GuiCreateProject(true))
  {
    accept();
  }
}

void plQtDashboardDlg::on_BrowseProject_clicked()
{
  if (plQtEditorApp::GetSingleton()->GuiOpenProject(true))
  {
    accept();
  }
}

void plQtDashboardDlg::on_ProjectsList_cellDoubleClicked(int row, int column)
{
  if (row < 0 || row >= ProjectsList->rowCount())
    return;

  QTableWidgetItem* pItem = ProjectsList->item(row, 0);

  QString sPath = pItem->data(Qt::UserRole).toString();

  if (plQtEditorApp::GetSingleton()->OpenProject(sPath.toUtf8().data(), true).Succeeded())
  {
    accept();
  }
}

void plQtDashboardDlg::on_OpenProject_clicked()
{
  on_ProjectsList_cellDoubleClicked(ProjectsList->currentRow(), 0);
}

void plQtDashboardDlg::on_OpenSample_clicked()
{
  on_SamplesList_itemDoubleClicked(SamplesList->currentItem());
}

void plQtDashboardDlg::on_LoadLastProject_stateChanged(int)
{
  if (PlasmaEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<PlasmaEditorPreferencesUser>())
  {
    pPreferences->m_bLoadLastProjectAtStartup = LoadLastProject->isChecked();
  }
}

void plQtDashboardDlg::on_SamplesList_itemDoubleClicked(QListWidgetItem* pItem)
{
  if (pItem == nullptr)
    return;

  QString sPath = pItem->data(Qt::UserRole).toString().toUtf8().data();

  if (plQtEditorApp::GetSingleton()->OpenProject(sPath.toUtf8().data(), true).Succeeded())
  {
    accept();
  }
}

void plQtDashboardDlg::on_OpenDocs_clicked()
{
  QDesktopServices::openUrl(QUrl("https://plengine.net"));
}

void plQtDashboardDlg::on_OpenApiDocs_clicked()
{
  QDesktopServices::openUrl(QUrl("https://plengine.github.io/api-docs/"));
}

void plQtDashboardDlg::on_GitHubDiscussions_clicked()
{
  QDesktopServices::openUrl(QUrl("https://github.com/PlasmaEngine/PlasmaEngine/discussions"));
}

void plQtDashboardDlg::on_ReportProblem_clicked()
{
  QDesktopServices::openUrl(QUrl("https://github.com/PlasmaEngine/PlasmaEngine/issues"));
}

void plQtDashboardDlg::on_OpenDiscord_clicked()
{
  QDesktopServices::openUrl(QUrl("https://discord.gg/rfJewc5khZ"));
}

void plQtDashboardDlg::on_OpenTwitter_clicked()
{
  QDesktopServices::openUrl(QUrl("https://twitter.com/PlasmaEngineProject"));
}

bool plQtDashboardDlg::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::Type::KeyPress)
  {
    QKeyEvent* key = static_cast<QKeyEvent*>(e);

    if ((key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return))
    {
      on_OpenProject_clicked();
      return true;
    }
  }

  return QObject::eventFilter(obj, e);
}
