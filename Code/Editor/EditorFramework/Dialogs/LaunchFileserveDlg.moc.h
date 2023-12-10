#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_LaunchFileserveDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class PLASMA_EDITORFRAMEWORK_DLL plQtLaunchFileserveDlg : public QDialog, public Ui_plQtLaunchFileserveDlg
{
public:
  Q_OBJECT

public:
  plQtLaunchFileserveDlg(QWidget* pParent);
  ~plQtLaunchFileserveDlg();

  plString m_sFileserveCmdLine;

private Q_SLOTS:
  void on_ButtonLaunch_clicked();

private:
  virtual void showEvent(QShowEvent* event) override;
};

