#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetImportDlg.h>
#include <QDialog>

class plQtAssetImportDlg : public QDialog, public Ui_AssetImportDlg
{
  Q_OBJECT

public:
  plQtAssetImportDlg(QWidget* pParent, plDynamicArray<plAssetDocumentGenerator::ImportGroupOptions>& ref_allImports);
  ~plQtAssetImportDlg();

private Q_SLOTS:
  void SelectedOptionChanged(int index);
  void on_ButtonImport_clicked();

private:
  void InitRow(plUInt32 uiRow);

  plDynamicArray<plAssetDocumentGenerator::ImportGroupOptions>& m_AllImports;
};

