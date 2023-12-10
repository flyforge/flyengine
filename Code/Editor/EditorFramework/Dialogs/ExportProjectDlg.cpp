#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QFileDialog>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>


bool plQtExportProjectDlg::s_bTransformAll = true;

plQtExportProjectDlg::plQtExportProjectDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  plProjectPreferencesUser* pPref = plPreferences::QueryPreferences<plProjectPreferencesUser>();

  Destination->setText(pPref->m_sExportFolder.GetData());
}

void plQtExportProjectDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  TransformAll->setChecked(s_bTransformAll);

  if (!plCppProject::ExistsProjectCMakeListsTxt())
  {
    CompileCpp->setEnabled(false);
    CompileCpp->setToolTip("This project doesn't have a C++ plugin.");
    CompileCpp->setChecked(false);
  }
  else
  {
    CompileCpp->setChecked(true);
  }
}

void plQtExportProjectDlg::on_BrowseDestination_clicked()
{
  QString sPath = QFileDialog::getExistingDirectory(this, QLatin1String("Select output directory"), Destination->text());

  if (!sPath.isEmpty())
  {
    Destination->setText(sPath);
    plProjectPreferencesUser* pPref = plPreferences::QueryPreferences<plProjectPreferencesUser>();
    pPref->m_sExportFolder = sPath.toUtf8().data();
  }
}

void plQtExportProjectDlg::on_ExportProjectButton_clicked()
{
  // TODO:
  // filter out unused runtime/game plugins
  // select asset profile for export
  // copy inputs into resource: RML files

  if (CompileCpp->isChecked())
  {
    if (plCppProject::EnsureCppPluginReady().Failed())
      return;
  }

  if (TransformAll->isChecked())
  {
    plStatus stat = plAssetCurator::GetSingleton()->TransformAllAssets(plTransformFlags::TriggeredManually);

    if (stat.Failed())
    {
      plQtUiServices::GetSingleton()->MessageBoxStatus(stat, "Asset transform failed");
      return;
    }
  }

  const plString szDstFolder = Destination->text().toUtf8().data();

  plLogSystemToBuffer logFile;

  auto WriteLogFile = [&]() {
    plStringBuilder sTemp;
    sTemp.Set(szDstFolder, "/ExportLog.txt");

    ExportLog->setPlainText(logFile.m_sBuffer.GetData());

    plOSFile file;

    if (file.Open(sTemp, plFileOpenMode::Write).Failed())
    {
      plQtUiServices::GetSingleton()->MessageBoxWarning(plFmt("Failed to write export log '{0}'", sTemp));
      return;
    }

    file.Write(logFile.m_sBuffer.GetData(), logFile.m_sBuffer.GetElementCount()).AssertSuccess();
  };

  plLogSystemScope logScope(&logFile);
  PLASMA_SCOPE_EXIT(WriteLogFile());

  if (plProjectExport::ExportProject(szDstFolder, plAssetCurator::GetSingleton()->GetActiveAssetProfile(), plQtEditorApp::GetSingleton()->GetFileSystemConfig()).Failed())
  {
    plQtUiServices::GetSingleton()->MessageBoxWarning("Project export failed. See log for details.");
  }
  else
  {
    plQtUiServices::GetSingleton()->MessageBoxInformation("Project export successful.");
    plQtUiServices::GetSingleton()->OpenInExplorer(szDstFolder, false);
  }
}
