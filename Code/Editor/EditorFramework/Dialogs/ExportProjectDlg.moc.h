#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_ExportProjectDlg.h>
#include <QDialog>

class plQtExportProjectDlg : public QDialog, public Ui_ExportProjectDlg
{
  Q_OBJECT

public:
  plQtExportProjectDlg(QWidget* pParent);

  static bool s_bTransformAll;

private Q_SLOTS:
  void on_ExportProjectButton_clicked();
  void on_BrowseDestination_clicked();


protected:
  virtual void showEvent(QShowEvent* e) override;
};
