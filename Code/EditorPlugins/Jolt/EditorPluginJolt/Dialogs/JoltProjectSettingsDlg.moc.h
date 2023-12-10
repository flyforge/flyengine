#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <EditorPluginJolt/ui_JoltProjectSettingsDlg.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <QDialog>

class plQtJoltProjectSettingsDlg : public QDialog, public Ui_JoltProjectSettingsDlg
{
public:
  Q_OBJECT

public:
  plQtJoltProjectSettingsDlg(QWidget* pParent);

  static void EnsureConfigFileExists();

private Q_SLOTS:
  void onCheckBoxClicked(bool checked);
  void on_DefaultButtons_clicked(QAbstractButton* pButton);
  void on_ButtonAddLayer_clicked();
  void on_ButtonRemoveLayer_clicked();
  void on_ButtonRenameLayer_clicked();
  void on_FilterTable_itemSelectionChanged();

private:
  void SetupTable();
  plResult Save();
  plResult Load();

  plUInt32 m_IndexRemap[32];
  plCollisionFilterConfig m_Config;
  plCollisionFilterConfig m_ConfigReset;
};
