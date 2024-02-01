#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/ui_ExtractGeometryDlg.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Math/Mat3.h>

#include <QPushButton>

class plQtExtractGeometryDlg : public QDialog, public Ui_ExtractGeometryDlg
{
  Q_OBJECT

public:
  plQtExtractGeometryDlg(QWidget* pParent);

  static QString s_sDestinationFile;
  static bool s_bOnlySelection;
  static int s_iExtractionMode;
  static plMat3 GetCoordinateSystemTransform();

private Q_SLOTS:
  void on_ButtonBox_clicked(QAbstractButton* button);
  void on_BrowseButton_clicked();

private:
  void UpdateUI();
  void QueryUI();

  static int s_iCoordinateSystem;
};

