#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetImportDlg.h>
#include <QDialog>

class plQtAssetImportDlg : public QDialog, public Ui_AssetImportDlg
{
  Q_OBJECT

public:
  plQtAssetImportDlg(QWidget* parent, plDynamicArray<plAssetDocumentGenerator::ImportData>& allImports, bool& bImported);
  ~plQtAssetImportDlg();

private Q_SLOTS:
  void SelectedOptionChanged(int index);
  void on_ButtonImport_clicked();
  void TableCellChanged(int row, int column);
  void BrowseButtonClicked(bool);

private:
  void InitRow(plUInt32 uiRow);
  void UpdateRow(plUInt32 uiRow);
  void QueryRow(plUInt32 uiRow);
  void UpdateAllRows();

  bool* m_Imported;
  plDynamicArray<plAssetDocumentGenerator::ImportData>& m_AllImports;
};

