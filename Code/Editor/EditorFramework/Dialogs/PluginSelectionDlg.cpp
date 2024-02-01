#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/PluginSelectionDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OpenDdlWriter.h>

plQtPluginSelectionDlg::plQtPluginSelectionDlg(plPluginBundleSet* pPluginSet, QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  m_pPluginSet = pPluginSet;
  m_LocalPluginSet = *pPluginSet;

  PluginSelectionWidget->SetPluginSet(&m_LocalPluginSet);
}

plQtPluginSelectionDlg::~plQtPluginSelectionDlg() = default;

void plQtPluginSelectionDlg::on_Buttons_clicked(QAbstractButton* pButton)
{
  if (Buttons->standardButton(pButton) == QDialogButtonBox::Ok)
  {
    PluginSelectionWidget->SyncStateToSet();

    if (!m_pPluginSet->IsStateEqual(m_LocalPluginSet))
    {
      *m_pPluginSet = m_LocalPluginSet;

      plQtEditorApp::GetSingleton()->WritePluginSelectionStateDDL();
      plQtEditorApp::GetSingleton()->AddRestartRequiredReason("The set of active plugins has changed.");
    }

    accept();
  }
  else
  {
    reject();
  }
}
