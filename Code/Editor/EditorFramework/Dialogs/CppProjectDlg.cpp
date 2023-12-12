#include <EditorFramework/EditorFrameworkPCH.h>

#include "Foundation/Logging/Log.h"
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

plQtCppProjectDlg::plQtCppProjectDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  m_OldCppSettings.Load().IgnoreResult();
  m_CppSettings.Load().IgnoreResult();

  {
    plQtScopedBlockSignals _1(PluginName);
    PluginName->setPlaceholderText(plToolsProject::GetSingleton()->GetProjectName(true).GetData());
    PluginName->setText(m_CppSettings.m_sPluginName.GetData());
  }

  {
    plQtScopedBlockSignals _1(Generator);
    Generator->addItem("None");
    Generator->addItem("Visual Studio 2019");
    Generator->addItem("Visual Studio 2022");
    Generator->setCurrentIndex(0);

    if (m_CppSettings.m_Compiler == plCppSettings::Compiler::Vs2019)
    {
      Generator->setCurrentIndex(1);
    }
    else if (m_CppSettings.m_Compiler == plCppSettings::Compiler::Vs2022)
    {
      Generator->setCurrentIndex(2);
    }
  }

  UpdateUI();
}

void plQtCppProjectDlg::on_Result_rejected()
{
  reject();
}

void plQtCppProjectDlg::on_OpenPluginLocation_clicked()
{
  plQtUiServices::OpenInExplorer(PluginLocation->text().toUtf8().data(), false);
}

void plQtCppProjectDlg::on_OpenBuildFolder_clicked()
{
  plQtUiServices::OpenInExplorer(BuildFolder->text().toUtf8().data(), false);
}

void plQtCppProjectDlg::on_Generator_currentIndexChanged(int)
{
  switch (Generator->currentIndex())
  {
    case 0:
      m_CppSettings.m_Compiler = plCppSettings::Compiler::None;
      break;
    case 1:
      m_CppSettings.m_Compiler = plCppSettings::Compiler::Vs2019;
      break;
    case 2:
      m_CppSettings.m_Compiler = plCppSettings::Compiler::Vs2022;
      break;
  }

  UpdateUI();
}

void plQtCppProjectDlg::on_OpenSolution_clicked()
{
  if (!plQtUiServices::OpenFileInDefaultProgram(plCppProject::GetSolutionPath(m_CppSettings)))
  {
    plQtUiServices::GetSingleton()->MessageBoxWarning("Opening the solution failed.");
  }
}

void plQtCppProjectDlg::on_PluginName_textEdited(const QString& text)
{
  plStringBuilder name = PluginName->text().toUtf8().data();

  if (name.EndsWith_NoCase("Plugin"))
  {
    name.Shrink(0, 6);
  }

  m_CppSettings.m_sPluginName = name;

  UpdateUI();
}

void plQtCppProjectDlg::UpdateUI()
{
  PluginLocation->setText(plCppProject::GetTargetSourceDir().GetData());
  BuildFolder->setText(plCppProject::GetBuildDir(m_CppSettings).GetData());

  GenerateSolution->setEnabled(m_CppSettings.m_Compiler != plCppSettings::Compiler::None);
  OpenPluginLocation->setEnabled(plOSFile::ExistsDirectory(PluginLocation->text().toUtf8().data()));
  OpenBuildFolder->setEnabled(plOSFile::ExistsDirectory(BuildFolder->text().toUtf8().data()));
  OpenSolution->setEnabled(plCppProject::ExistsSolution(m_CppSettings));
}

class plForwardToQTextEdit : public plLogInterface
{
public:
  QTextEdit* m_pTextEdit = nullptr;

  void HandleLogMessage(const plLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case plLogMsgType::GlobalDefault:
      case plLogMsgType::Flush:
      case plLogMsgType::BeginGroup:
      case plLogMsgType::EndGroup:
      case plLogMsgType::None:
      case plLogMsgType::All:
      case plLogMsgType::ENUM_COUNT:
        return;

      case plLogMsgType::ErrorMsg:
      case plLogMsgType::SeriousWarningMsg:
      case plLogMsgType::WarningMsg:
      case plLogMsgType::SuccessMsg:
      case plLogMsgType::InfoMsg:
      case plLogMsgType::DevMsg:
      case plLogMsgType::DebugMsg:
      {
        plStringBuilder tmp(le.m_sText, "\n");

        QString s = m_pTextEdit->toPlainText();
        s.append(tmp);
        m_pTextEdit->setText(s);
        return;
      }

        PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }
};

void plQtCppProjectDlg::on_GenerateSolution_clicked()
{
  if (plCppProject::ExistsSolution(m_CppSettings))
  {
    if (plQtUiServices::MessageBoxQuestion("The solution already exists, do you want to recreate it?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
    {
      return;
    }
  }

  if (m_CppSettings.m_sPluginName.IsEmpty())
  {
    m_CppSettings.m_sPluginName = PluginName->placeholderText().toUtf8().data();
  }

  if (!m_OldCppSettings.m_sPluginName.IsEmpty() && m_OldCppSettings.m_sPluginName != m_CppSettings.m_sPluginName)
  {
    if (plQtUiServices::MessageBoxQuestion("You are attempting to change the name of the existing C++ plugin.\n\nTHIS IS A BAD IDEA.\n\nThe C++ sources and CMake files were already created with the old name in it. To not accidentally delete your work, PLASMA won't touch any of those files. Therefore this change won't have any effect, unless you have already deleted those files yourself and PLASMA can just create new ones. Only select YES if you have done the necessary steps and/or know what you are doing.", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
    {
      return;
    }
  }

  if (m_CppSettings.Save().Failed())
  {
    plQtUiServices::GetSingleton()->MessageBoxWarning("Saving new C++ project settings failed.");
    return;
  }

  m_OldCppSettings.Load().IgnoreResult();

  if (plSystemInformation::IsDebuggerAttached())
  {
    plQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, CMake usually fails with the error that no C/C++ compiler can be found.\n\nDetach the debugger now, then press OK to continue.");
  }

  OutputLog->clear();

  {
    plForwardToQTextEdit log;
    log.m_pTextEdit = OutputLog;
    plLogSystemScope _logScope(&log);

    plProgressRange progress("Generating Solution", 3, false);
    progress.SetStepWeighting(0, 0.1f);
    progress.SetStepWeighting(1, 0.1f);
    progress.SetStepWeighting(2, 0.8f);

    PLASMA_SCOPE_EXIT(UpdateUI());

    {
      progress.BeginNextStep("Clean Build Directory");

      if (plCppProject::CleanBuildDir(m_CppSettings).Failed())
      {
        plLog::Warning("Couldn't delete build output directory:\n{}\n\nProject is probably already open in Visual Studio.\n", plCppProject::GetBuildDir(m_CppSettings));
      }
    }

    {
      progress.BeginNextStep("Populate with Default Sources");
      if (plCppProject::PopulateWithDefaultSources(m_CppSettings).Failed())
      {
        plQtUiServices::GetSingleton()->MessageBoxWarning("Failed to populate the CppSource directory with the default files.\n\nCheck the log for details.");
        return;
      }
    }

    // run CMake
    {
      progress.BeginNextStep("Running CMake");

      if (plCppProject::RunCMake(m_CppSettings).Failed())
      {

        plQtUiServices::GetSingleton()->MessageBoxWarning("Generating the solution failed.\n\nCheck the log for details.");
        return;
      }
    }

    if (plCppProject::BuildCodeIfNecessary(m_CppSettings).Failed())
    {
      plLog::Error("Failed to compile the newly generated C++ solution.");
    }
  }

  plCppProject::UpdatePluginConfig(m_CppSettings);

  plQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);

  if (plQtUiServices::GetSingleton()->MessageBoxQuestion("The solution was generated successfully.\n\nDo you want to open it now?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
  {
    on_OpenSolution_clicked();
  }
}
