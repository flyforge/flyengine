#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorPluginScene/Dialogs/ExportAndRunDlg.moc.h>
#include <Foundation/IO/OSFile.h>
#include <QFileDialog>

bool plQtExportAndRunDlg::s_bTransformAll = true;
bool plQtExportAndRunDlg::s_bUpdateThumbnail = false;
bool plQtExportAndRunDlg::s_bCompileCpp = true;

static int s_iLastPlayerApp = 0;

plQtExportAndRunDlg::plQtExportAndRunDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  ToolCombo->addItem("plPlayer", "Player.exe");
#else
  ToolCombo->addItem("plPlayer", "Player");
#endif

  plProjectPreferencesUser* pPref = plPreferences::QueryPreferences<plProjectPreferencesUser>();

  for (const auto& app : pPref->m_PlayerApps)
  {
    plStringBuilder name = plPathUtils::GetFileName(app);

    ToolCombo->addItem(name.GetData(), QString::fromUtf8(app.GetData()));
  }

  ToolCombo->setCurrentIndex(s_iLastPlayerApp);

  m_CppSettings.Load().IgnoreResult();
}

void plQtExportAndRunDlg::PullFromUI()
{
  s_bTransformAll = TransformAll->isChecked();
  s_bUpdateThumbnail = UpdateThumbnail->isChecked();
  s_iLastPlayerApp = ToolCombo->currentIndex();
  s_bCompileCpp = CompileCpp->isChecked();

  plProjectPreferencesUser* pPref = plPreferences::QueryPreferences<plProjectPreferencesUser>();
  pPref->m_PlayerApps.Clear();

  for (int i = 1; i < ToolCombo->count(); ++i)
  {
    plStringBuilder path = ToolCombo->itemData(i).toString().toUtf8().data();
    path.MakeCleanPath();

    pPref->m_PlayerApps.PushBack(path);
  }
}

void plQtExportAndRunDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  UpdateThumbnail->setVisible(m_bShowThumbnailCheckbox);
  TransformAll->setChecked(s_bTransformAll);
  UpdateThumbnail->setChecked(s_bUpdateThumbnail);
  PlayerCmdLine->setPlainText(m_sCmdLine.GetData());

  if (!plCppProject::ExistsProjectCMakeListsTxt())
  {
    CompileCpp->setEnabled(false);
    CompileCpp->setToolTip("This project doesn't have a C++ plugin.");
    CompileCpp->setChecked(false);
  }
  else
  {
    CompileCpp->setChecked(s_bCompileCpp);
  }
}

void plQtExportAndRunDlg::on_ExportOnly_clicked()
{
  PullFromUI();
  m_bRunAfterExport = false;
  accept();
}

void plQtExportAndRunDlg::on_ExportAndRun_clicked()
{
  PullFromUI();
  m_bRunAfterExport = true;
  m_sApplication = ToolCombo->currentData().toString().toUtf8().data();
  accept();
}

void plQtExportAndRunDlg::on_AddToolButton_clicked()
{
  plStringBuilder appDir = plOSFile::GetApplicationDirectory();
  appDir.MakeCleanPath();
  static QString sLastPath = appDir.GetData();

  const QString sFile = QFileDialog::getOpenFileName(this, "Select Program", sLastPath, "Applicaation (*.exe)", nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  sLastPath = sFile;

  plStringBuilder path = sFile.toUtf8().data();
  path.MakeCleanPath();
  path.TrimWordStart(appDir);
  path.Trim("/", "");

  plStringBuilder tmp;
  ToolCombo->addItem(QString::fromUtf8(path.GetFileName().GetData(tmp)), QString::fromUtf8(path.GetData()));
  ToolCombo->setCurrentIndex(ToolCombo->count() - 1);
}

void plQtExportAndRunDlg::on_RemoveToolButton_clicked()
{
  ToolCombo->removeItem(ToolCombo->currentIndex());
}

void plQtExportAndRunDlg::on_ToolCombo_currentIndexChanged(int idx)
{
  RemoveToolButton->setEnabled(idx != 0);
}
