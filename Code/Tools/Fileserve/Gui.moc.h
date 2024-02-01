#pragma once

#include <Fileserve/Fileserve.h>

#ifdef PL_USE_QT

#  include <QMainWindow>

class plApplication;
class plQtFileserveWidget;

class plQtFileserveMainWnd : public QMainWindow
{
  Q_OBJECT
public:
  plQtFileserveMainWnd(plApplication* pApp, QWidget* pParent = nullptr);

private Q_SLOTS:
  void UpdateNetworkSlot();
  void OnServerStarted(const QString& ip, plUInt16 uiPort);
  void OnServerStopped();

private:
  plApplication* m_pApp;
  plQtFileserveWidget* m_pFileserveWidget = nullptr;
};

void CreateFileserveMainWindow(plApplication* pApp);

#endif
