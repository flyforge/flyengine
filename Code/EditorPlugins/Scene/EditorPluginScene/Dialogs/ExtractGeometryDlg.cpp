#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Dialogs/ExtractGeometryDlg.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

#include <QFileDialog>

QString plQtExtractGeometryDlg::s_sDestinationFile;
bool plQtExtractGeometryDlg::s_bOnlySelection = false;
int plQtExtractGeometryDlg::s_iExtractionMode = (int)plWorldGeoExtractionUtil::ExtractionMode::RenderMesh;
int plQtExtractGeometryDlg::s_iCoordinateSystem = 1;

plQtExtractGeometryDlg::plQtExtractGeometryDlg(QWidget* pParent)

  : QDialog(pParent)
{
  setupUi(this);

  ExtractionMode->clear();
  ExtractionMode->addItem("Render Mesh");
  ExtractionMode->addItem("Collision Mesh");
  ExtractionMode->addItem("Navmesh Obstacles");

  CoordinateSystem->clear();
  CoordinateSystem->addItem("Forward: +X, Right: +Y, Up: +Z (pl)");
  CoordinateSystem->addItem("Forward: -Z, Right: +X, Up: +Y (OpenGL/Maya)");
  CoordinateSystem->addItem("Forward: +Z, Right: +X, Up: +Y (D3D)");

  UpdateUI();
}

void plQtExtractGeometryDlg::UpdateUI()
{
  DestinationFile->setText(s_sDestinationFile);
  ExtractOnlySelection->setChecked(s_bOnlySelection);
  ExtractionMode->setCurrentIndex(s_iExtractionMode);
  CoordinateSystem->setCurrentIndex(s_iCoordinateSystem);
}

void plQtExtractGeometryDlg::QueryUI()
{
  s_sDestinationFile = DestinationFile->text();
  s_bOnlySelection = ExtractOnlySelection->isChecked();
  s_iExtractionMode = ExtractionMode->currentIndex();
  s_iCoordinateSystem = CoordinateSystem->currentIndex();
}

void plQtExtractGeometryDlg::on_ButtonBox_clicked(QAbstractButton* button)
{
  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Ok))
  {
    QueryUI();

    if (!plPathUtils::IsAbsolutePath(s_sDestinationFile.toUtf8().data()))
    {
      plQtUiServices::GetSingleton()->MessageBoxWarning("Only absolute paths are allowed for the destination file.");
      return;
    }

    accept();
    return;
  }

  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Cancel))
  {
    reject();
    return;
  }
}

void plQtExtractGeometryDlg::on_BrowseButton_clicked()
{
  QString allFilters = "OBJ (*.obj)";
  QString sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Destination file"), s_sDestinationFile, allFilters,
    nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  DestinationFile->setText(sFile);
}

plMat3 plQtExtractGeometryDlg::GetCoordinateSystemTransform()
{
  plMat3 m;
  m.SetIdentity();

  switch (s_iCoordinateSystem)
  {
    case 0:
      break;

    case 1:
      m.SetRow(2, plVec3(-1, 0, 0)); // forward
      m.SetRow(1, plVec3(0, 0, 1));  // up
      m.SetRow(0, plVec3(0, 1, 0));  // right
      break;

    case 2:
      m.SetRow(2, plVec3(1, 0, 0)); // forward
      m.SetRow(1, plVec3(0, 0, 1)); // up
      m.SetRow(0, plVec3(0, 1, 0)); // right
      break;
  }

  return m;
}
