#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/ui_PluginSelectionDlg.h>
#include <QDialog>

class PLASMA_EDITORFRAMEWORK_DLL plQtPluginSelectionDlg : public QDialog, public Ui_PluginSelectionDlg
{
public:
  Q_OBJECT

public:
  plQtPluginSelectionDlg(plPluginBundleSet* pPluginSet, QWidget* pParent = nullptr);
  ~plQtPluginSelectionDlg();


private Q_SLOTS:
  void on_Buttons_clicked(QAbstractButton* pButton);

private:
  plPluginBundleSet m_LocalPluginSet;
  plPluginBundleSet* m_pPluginSet = nullptr;
};
