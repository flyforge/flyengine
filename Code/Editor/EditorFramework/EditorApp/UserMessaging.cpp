#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>

void plQtEditorApp::AddRestartRequiredReason(const char* szReason)
{
  if (!m_RestartRequiredReasons.Find(szReason).IsValid())
  {
    m_RestartRequiredReasons.Insert(szReason);
    UpdateGlobalStatusBarMessage();
  }

  plStringBuilder s;
  s.SetFormat("The editor process must be restarted.\nReason: '{0}'\n\nDo you want to restart now?", szReason);

  if (plQtUiServices::MessageBoxQuestion(s, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
  {
    if (plToolsProject::CanCloseProject())
    {
      LaunchEditor(plToolsProject::GetSingleton()->GetProjectFile(), false);

      QApplication::closeAllWindows();
      return;
    }
  }
}

void plQtEditorApp::AddReloadProjectRequiredReason(const char* szReason)
{
  if (!m_ReloadProjectRequiredReasons.Find(szReason).IsValid())
  {
    m_ReloadProjectRequiredReasons.Insert(szReason);

    plStringBuilder s;
    s.SetFormat("The project must be reloaded.\nReason: '{0}'", szReason);

    plQtUiServices::MessageBoxInformation(s);

    UpdateGlobalStatusBarMessage();
  }
}

void plQtEditorApp::UpdateGlobalStatusBarMessage()
{
  plStringBuilder sText;

  if (!m_RestartRequiredReasons.IsEmpty())
    sText.Append("Restart the editor to apply changes.   ");

  if (!m_ReloadProjectRequiredReasons.IsEmpty())
    sText.Append("Reload the project to apply changes.   ");

  plQtUiServices::ShowGlobalStatusBarMessage(sText);
}
