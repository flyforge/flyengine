#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/QtWaitForOperationDlg.moc.h>
#include <QTimer>

plQtWaitForOperationDlg::plQtWaitForOperationDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  QTimer::singleShot(10, this, &plQtWaitForOperationDlg::onIdle);
}

plQtWaitForOperationDlg::~plQtWaitForOperationDlg() = default;

void plQtWaitForOperationDlg::on_ButtonCancel_clicked()
{
  reject();
}

void plQtWaitForOperationDlg::onIdle()
{
  if (m_OnIdle())
  {
    QTimer::singleShot(10, this, &plQtWaitForOperationDlg::onIdle);
  }
  else
  {
    accept();
  }
}
