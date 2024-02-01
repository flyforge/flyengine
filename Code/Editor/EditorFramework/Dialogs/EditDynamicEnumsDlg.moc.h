#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_EditDynamicEnumsDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class plDynamicStringEnum;

class PL_EDITORFRAMEWORK_DLL plQtEditDynamicEnumsDlg : public QDialog, public Ui_plQtEditDynamicEnumsDlg
{
public:
  Q_OBJECT

public:
  plQtEditDynamicEnumsDlg(plDynamicStringEnum* pEnum, QWidget* pParent);

  plInt32 GetSelectedItem() const { return m_iSelectedItem; }

private Q_SLOTS:
  void on_ButtonAdd_clicked();
  void on_ButtonRemove_clicked();
  void on_Buttons_clicked(QAbstractButton* button);
  void on_EnumValues_itemDoubleClicked(QListWidgetItem* item);

private:
  void FillList();
  bool EditItem(plString& item);

  bool m_bModified = false;
  plDynamicStringEnum* m_pEnum = nullptr;
  plDynamicArray<plString> m_Values;
  plInt32 m_iSelectedItem = -1;
};

