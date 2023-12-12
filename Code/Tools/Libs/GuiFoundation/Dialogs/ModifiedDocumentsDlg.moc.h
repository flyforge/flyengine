#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ModifiedDocumentsDlg.h>
#include <QDialog>
#include <ToolsFoundation/Document/Document.h>

class PLASMA_GUIFOUNDATION_DLL plQtModifiedDocumentsDlg : public QDialog, public Ui_DocumentList
{
public:
  Q_OBJECT

public:
  plQtModifiedDocumentsDlg(QWidget* parent, const plHybridArray<plDocument*, 32>& ModifiedDocs);


private Q_SLOTS:
  void on_ButtonSaveSelected_clicked();
  void on_ButtonDontSave_clicked();
  void SlotSaveDocument();
  void SlotSelectionChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

private:
  plResult SaveDocument(plDocument* pDoc);

  plHybridArray<plDocument*, 32> m_ModifiedDocs;
};

