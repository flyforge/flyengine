#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ShortcutEditorDlg.h>
#include <QDialog>

struct plActionDescriptor;

class PLASMA_GUIFOUNDATION_DLL plQtShortcutEditorDlg : public QDialog, public Ui_ShortcutEditor
{
public:
  Q_OBJECT

public:
  plQtShortcutEditorDlg(QWidget* pParent);
  ~plQtShortcutEditorDlg();

  void UpdateTable();

private Q_SLOTS:
  void SlotSelectionChanged();
  void on_KeyEditor_editingFinished();

  void UpdateKeyEdit();

  void on_KeyEditor_keySequenceChanged(const QKeySequence& keySequence);
  void on_ButtonAssign_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonReset_clicked();

private:
  plInt32 m_iSelectedAction;
  plHybridArray<plActionDescriptor*, 32> m_ActionDescs;
};

