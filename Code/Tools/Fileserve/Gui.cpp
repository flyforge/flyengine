#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>

#ifdef PL_USE_QT

#  include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Application/Application.h>
#  include <QTimer>

void CreateFileserveMainWindow(plApplication* pApp)
{
  plQtFileserveMainWnd* pMainWnd = new plQtFileserveMainWnd(pApp);
  pMainWnd->show();
}

plQtFileserveMainWnd::plQtFileserveMainWnd(plApplication* pApp, QWidget* pParent)
  : QMainWindow(pParent)
  , m_pApp(pApp)
{
  OnServerStopped();

  m_pFileserveWidget = new plQtFileserveWidget(this);
  QMainWindow::setCentralWidget(m_pFileserveWidget);
  resize(700, 650);

  connect(m_pFileserveWidget, &plQtFileserveWidget::ServerStarted, this, &plQtFileserveMainWnd::OnServerStarted);
  connect(m_pFileserveWidget, &plQtFileserveWidget::ServerStopped, this, &plQtFileserveMainWnd::OnServerStopped);

  show();

  QTimer::singleShot(0, this, &plQtFileserveMainWnd::UpdateNetworkSlot);

  setWindowIcon(m_pFileserveWidget->windowIcon());
}


void plQtFileserveMainWnd::UpdateNetworkSlot()
{
  if (m_pApp->Run() == plApplication::Execution::Continue)
  {
    QTimer::singleShot(0, this, &plQtFileserveMainWnd::UpdateNetworkSlot);
  }
  else
  {
    close();
  }
}

void plQtFileserveMainWnd::OnServerStarted(const QString& ip, plUInt16 uiPort)
{
  QString title = QString("plFileserve (Port %1)").arg(uiPort);

  setWindowTitle(title);
}

void plQtFileserveMainWnd::OnServerStopped()
{
  setWindowTitle("plFileserve (not running)");
}

#endif
