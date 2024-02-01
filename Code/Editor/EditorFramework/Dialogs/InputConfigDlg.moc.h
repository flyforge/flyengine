#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_InputConfigDlg.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <QDialog>

class QTreeWidgetItem;

class PL_EDITORFRAMEWORK_DLL plQtInputConfigDlg : public QDialog, public Ui_InputConfigDialog
{
public:
  Q_OBJECT

public:
  plQtInputConfigDlg(QWidget* pParent);

private Q_SLOTS:
  void on_ButtonNewInputSet_clicked();
  void on_ButtonNewAction_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonReset_clicked();
  void on_TreeActions_itemSelectionChanged();

private:
  void LoadActions();
  void SaveActions();
  void FillList();
  void GetActionsFromList();

  QTreeWidgetItem* CreateActionItem(QTreeWidgetItem* pParentItem, const plGameAppInputConfig& action);

  plMap<plString, QTreeWidgetItem*> m_InputSetToItem;
  plHybridArray<plGameAppInputConfig, 32> m_Actions;
  plDynamicArray<plString> m_AllInputSlots;
};

