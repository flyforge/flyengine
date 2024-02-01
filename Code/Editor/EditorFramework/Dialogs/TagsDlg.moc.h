#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_TagsDlg.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <QDialog>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

class PL_EDITORFRAMEWORK_DLL plQtTagsDlg : public QDialog, public Ui_plQtTagsDlg
{
public:
  Q_OBJECT

public:
  plQtTagsDlg(QWidget* pParent);

private Q_SLOTS:
  void on_ButtonNewCategory_clicked();
  void on_ButtonNewTag_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonReset_clicked();
  void on_TreeTags_itemSelectionChanged();

private:
  void LoadTags();
  void SaveTags();
  void FillList();
  void GetTagsFromList();

  QTreeWidgetItem* CreateTagItem(QTreeWidgetItem* pParentItem, const QString& tag, bool bBuiltIn);

  plHybridArray<plToolsTag, 32> m_Tags;
  plMap<plString, QTreeWidgetItem*> m_CategoryToItem;
};

