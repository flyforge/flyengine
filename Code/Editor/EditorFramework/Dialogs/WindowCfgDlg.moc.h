#pragma once

#include <Core/System/Window.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_WindowCfgDlg.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <QDialog>

class PL_EDITORFRAMEWORK_DLL plQtWindowCfgDlg : public QDialog, public Ui_plQtWindowCfgDlg
{
public:
  Q_OBJECT

public:
  plQtWindowCfgDlg(QWidget* pParent);

private Q_SLOTS:
  void on_m_ButtonBox_clicked(QAbstractButton* button);
  void on_m_ComboWnd_currentIndexChanged(int index);
  void on_m_CheckOverrideDefault_stateChanged(int state);

private:
  void FillUI(const plWindowCreationDesc& desc);
  void GrabUI(plWindowCreationDesc& desc);
  void UpdateUI();
  void LoadDescs();
  void SaveDescs();

  plUInt8 m_uiCurDesc = 0;
  plWindowCreationDesc m_Descs[2];
  bool m_bOverrideProjectDefault[2];
};
