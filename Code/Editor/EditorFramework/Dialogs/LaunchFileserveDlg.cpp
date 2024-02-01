#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/LaunchFileserveDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

plQtLaunchFileserveDlg::plQtLaunchFileserveDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);
}

plQtLaunchFileserveDlg::~plQtLaunchFileserveDlg() = default;

void plQtLaunchFileserveDlg::showEvent(QShowEvent* event)
{
  plStringBuilder sCmdLine = plQtEditorApp::GetSingleton()->BuildFileserveCommandLine();
  EditFileserve->setPlainText(sCmdLine.GetData());

  QDialog::showEvent(event);
}

void plQtLaunchFileserveDlg::on_ButtonLaunch_clicked()
{
  plQtEditorApp::GetSingleton()->RunFileserve();
  accept();
}
