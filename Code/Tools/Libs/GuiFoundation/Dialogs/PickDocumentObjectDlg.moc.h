#pragma once

#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_PickDocumentObjectDlg.h>
#include <QDialog>

class plDocumentObject;

class PL_GUIFOUNDATION_DLL plQtPickDocumentObjectDlg : public QDialog, public Ui_PickDocumentObjectDlg
{
  Q_OBJECT

public:
  struct Element
  {
    const plDocumentObject* m_pObject;
    plString m_sDisplayName;
  };

  plQtPickDocumentObjectDlg(QWidget* pParent, const plArrayPtr<Element>& objects, const plUuid& currentObject);

  /// Stores the result that the user picked
  const plDocumentObject* m_pPickedObject = nullptr;

private Q_SLOTS:
  void on_ObjectTree_itemDoubleClicked(QTreeWidgetItem* pItem, int column);

private:
  void UpdateTable();

  plArrayPtr<Element> m_Objects;
  plUuid m_CurrentObject;
};

