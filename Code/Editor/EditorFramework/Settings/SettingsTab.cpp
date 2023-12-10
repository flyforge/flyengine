#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <QDesktopServices>

PLASMA_IMPLEMENT_SINGLETON(plQtSettingsTab);

plString plQtSettingsTab::GetWindowIcon() const
{
  return ""; //:/GuiFoundation/PLASMA-logo.svg";
}

plString plQtSettingsTab::GetDisplayNameShort() const
{
  return "";
}

void plQtEditorApp::ShowSettingsDocument()
{
  plQtSettingsTab* pSettingsTab = plQtSettingsTab::GetSingleton();

  if (pSettingsTab == nullptr)
  {
    pSettingsTab = new plQtSettingsTab();
  }

  pSettingsTab->EnsureVisible();
}

void plQtEditorApp::CloseSettingsDocument()
{
  plQtSettingsTab* pSettingsTab = plQtSettingsTab::GetSingleton();

  if (pSettingsTab != nullptr)
  {
    pSettingsTab->CloseDocumentWindow();
  }
}

plQtSettingsTab::plQtSettingsTab()
  : plQtDocumentWindow("Settings")
  , m_SingletonRegistrar(this)
{
  setCentralWidget(new QWidget());
  PLASMA_ASSERT_DEV(centralWidget() != nullptr, "");

  setupUi(centralWidget());
  QMetaObject::connectSlotsByName(this);

  plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
  plActionContext context;
  context.m_sMapping = "SettingsTabMenuBar";
  context.m_pDocument = nullptr;
  pMenuBar->SetActionContext(context);

  FinishWindowCreation();

  plToolsProject::s_Events.AddEventHandler(plMakeDelegate(&plQtSettingsTab::ToolsProjectEventHandler, this));
}

plQtSettingsTab::~plQtSettingsTab()
{
  plToolsProject::s_Events.RemoveEventHandler(plMakeDelegate(&plQtSettingsTab::ToolsProjectEventHandler, this));
}

void plQtSettingsTab::on_OpenScene_clicked()
{
  plQtAssetBrowserDlg dlg(this, plUuid(), "Scene");
  if (dlg.exec() == 0)
    return;

  plQtEditorApp::GetSingleton()->OpenDocument(dlg.GetSelectedAssetPathAbsolute(), plDocumentFlags::RequestWindow | plDocumentFlags::AddToRecentFilesList);
}

void plQtSettingsTab::on_OpenProject_clicked()
{
  plQtDashboardDlg dlg(nullptr, plQtDashboardDlg::DashboardTab::Samples);
  dlg.exec();
}

void plQtSettingsTab::on_GettingStarted_clicked()
{
  QDesktopServices::openUrl(QUrl("https://plengine.net/pages/getting-started/editor-overview.html"));
}

bool plQtSettingsTab::InternalCanCloseWindow()
{
  // if this is the last window, prevent closing it
  return plQtDocumentWindow::GetAllDocumentWindows().GetCount() > 1;
}

void plQtSettingsTab::InternalCloseDocumentWindow()
{
  // make sure this instance isn't used anymore
  UnregisterSingleton();
}

void plQtSettingsTab::ToolsProjectEventHandler(const plToolsProjectEvent& e)
{
  if (e.m_Type == plToolsProjectEvent::Type::ProjectClosed || e.m_Type == plToolsProjectEvent::Type::ProjectCreated || e.m_Type == plToolsProjectEvent::Type::ProjectOpened)
  {
    plStringBuilder txt = "<html><head/><body><p align=\"center\"><span style=\" font-size:18pt;\">Open Project:</span></p><p align=\"center\"><span style=\" font-size:18pt;\">None</span></p></body></html>";

    if (plToolsProject::GetSingleton()->IsProjectOpen())
    {
      txt.ReplaceAll("None", plToolsProject::GetSingleton()->GetProjectName(false));
      OpenScene->setVisible(true);
    }
    else
    {
      txt = "<html><head/><body><p align=\"center\"><span style=\" font-size:18pt;\">No Project Open</span></p></body></html>";
      OpenScene->setVisible(false);
    }

    ProjectLabel->setText(txt.GetData());
  }
}
