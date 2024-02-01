#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_DataDirsDlg.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <QDialog>

class PL_EDITORFRAMEWORK_DLL plQtDataDirsDlg : public QDialog, public Ui_plQtDataDirsDlg
{
public:
  Q_OBJECT

public:
  plQtDataDirsDlg(QWidget* pParent);

private Q_SLOTS:
  void on_ButtonOK_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonUp_clicked();
  void on_ButtonDown_clicked();
  void on_ButtonAdd_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOpenFolder_clicked();
  void on_ListDataDirs_itemSelectionChanged();
  void on_ListDataDirs_itemDoubleClicked(QListWidgetItem* pItem);

private:
  void FillList();

  plInt32 m_iSelection;
  plApplicationFileSystemConfig m_Config;
};

