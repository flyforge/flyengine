#pragma once

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/ui_ExportAndRunDlg.h>
#include <QDialog>

class plSceneDocument;

class plQtExportAndRunDlg : public QDialog, public Ui_ExportAndRunDlg
{
  Q_OBJECT

public:
  plQtExportAndRunDlg(QWidget* parent);

  static bool s_bTransformAll;
  static bool s_bUpdateThumbnail;
  static bool s_bCompileCpp;
  bool m_bRunAfterExport = false;
  bool m_bShowThumbnailCheckbox = true;
  plString m_sCmdLine;
  plString m_sApplication;
  plCppSettings m_CppSettings;

private Q_SLOTS:
  void on_ExportOnly_clicked();
  void on_ExportAndRun_clicked();
  void on_AddToolButton_clicked();
  void on_RemoveToolButton_clicked();
  void on_ToolCombo_currentIndexChanged(int);

private:
  void PullFromUI();

protected:
  virtual void showEvent(QShowEvent* e) override;
};

