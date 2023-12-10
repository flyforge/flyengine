#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Delegate.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_QtWaitForOperationDlg.h>

class QWinTaskbarProgress;
class QWinTaskbarButton;

class PLASMA_GUIFOUNDATION_DLL plQtWaitForOperationDlg : public QDialog, public Ui_QtWaitForOperationDlg
{
  Q_OBJECT

public:
  plQtWaitForOperationDlg(QWidget* pParent);
  ~plQtWaitForOperationDlg();

  plDelegate<bool()> m_OnIdle;

private Q_SLOTS:
  void on_ButtonCancel_clicked();
  void onIdle();
};

