#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

enum Columns
{
  Method,
  InputFile,
  GeneratedDoc,
  Browse,
  Status,
  ENUM_COUNT
};

plQtAssetImportDlg::plQtAssetImportDlg(QWidget* parent, plDynamicArray<plAssetDocumentGenerator::ImportData>& allImports, bool& bImported)
  : QDialog(parent)
  , m_AllImports(allImports)
  , m_Imported(&bImported)
{
  setupUi(this);
  *m_Imported = false;
  QStringList headers;
  headers.push_back(QString::fromUtf8("Import Method"));
  headers.push_back(QString::fromUtf8("Input File"));
  headers.push_back(QString::fromUtf8("Generated Document"));
  headers.push_back(QString());
  headers.push_back(QString("Status"));

  QTableWidget* table = AssetTable;
  table->setColumnCount(headers.size());
  table->setRowCount(m_AllImports.GetCount());
  table->setHorizontalHeaderLabels(headers);

  {
    plQtScopedBlockSignals _1(table);

    for (plUInt32 i = 0; i < m_AllImports.GetCount(); ++i)
    {
      InitRow(i);
    }
  }

  table->horizontalHeader()->setSectionResizeMode(Columns::Method, QHeaderView::ResizeMode::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(Columns::InputFile, QHeaderView::ResizeMode::Interactive);
  table->horizontalHeader()->setSectionResizeMode(Columns::GeneratedDoc, QHeaderView::ResizeMode::Interactive);
  table->horizontalHeader()->setSectionResizeMode(Columns::Browse, QHeaderView::ResizeMode::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(Columns::Status, QHeaderView::ResizeMode::Stretch);

  table->resizeColumnToContents(Columns::GeneratedDoc);
  table->resizeColumnToContents(Columns::InputFile);
  table->setColumnWidth(Columns::InputFile, table->columnWidth(Columns::InputFile) + 30);
  table->setColumnWidth(Columns::GeneratedDoc, table->columnWidth(Columns::GeneratedDoc) + 30);
}

plQtAssetImportDlg::~plQtAssetImportDlg() {}

void plQtAssetImportDlg::InitRow(plUInt32 uiRow)
{
  QTableWidget* table = AssetTable;
  const auto& data2 = m_AllImports[uiRow];

  table->setItem(uiRow, Columns::InputFile, new QTableWidgetItem(data2.m_sInputFileParentRelative.GetData()));
  table->setItem(uiRow, Columns::Status, new QTableWidgetItem());
  table->item(uiRow, Columns::InputFile)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
  table->item(uiRow, Columns::Status)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);

  QComboBox* pCombo = new QComboBox();
  table->setCellWidget(uiRow, Columns::Method, pCombo);

  pCombo->addItem(plQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/NoEntry.svg"), "No Import");
  for (const auto& option : data2.m_ImportOptions)
  {
    QIcon icon = plQtUiServices::GetSingleton()->GetCachedIconResource(option.m_sIcon);
    pCombo->addItem(icon, plTranslate(option.m_sName.GetData()));
  }

  pCombo->setProperty("row", uiRow);
  pCombo->setCurrentIndex(data2.m_iSelectedOption + 1);
  connect(pCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(SelectedOptionChanged(int)));
  connect(table, &QTableWidget::cellChanged, this, &plQtAssetImportDlg::TableCellChanged);

  table->setItem(uiRow, Columns::GeneratedDoc, new QTableWidgetItem(QString()));
  table->item(uiRow, Columns::GeneratedDoc)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsEditable | Qt::ItemFlag::ItemIsSelectable);

  QToolButton* pBrowse = new QToolButton();
  pBrowse->setText("Browse...");
  pBrowse->setProperty("row", uiRow);
  connect(pBrowse, &QToolButton::clicked, this, &plQtAssetImportDlg::BrowseButtonClicked);
  table->setCellWidget(uiRow, Columns::Browse, pBrowse);

  UpdateRow(uiRow);
}

void plQtAssetImportDlg::UpdateRow(plUInt32 uiRow)
{
  QTableWidget* table = AssetTable;
  plQtScopedBlockSignals _1(table);

  const auto& data2 = m_AllImports[uiRow];

  QTableWidgetItem* pOutputItem = table->item(uiRow, Columns::GeneratedDoc);
  QTableWidgetItem* pStatusItem = table->item(uiRow, Columns::Status);

  if (data2.m_iSelectedOption < 0)
    pOutputItem->setText(QString());
  else
    pOutputItem->setText(data2.m_ImportOptions[data2.m_iSelectedOption].m_sOutputFileParentRelative.GetData());

  QToolButton* pBrowse = qobject_cast<QToolButton*>(table->cellWidget(uiRow, Columns::Browse));
  pBrowse->setEnabled(data2.m_iSelectedOption >= 0);

  pStatusItem->setForeground(QColor::fromRgba(qRgb(200, 0, 0)));
  pStatusItem->setText(data2.m_sImportMessage.GetData());

  if (data2.m_bDoNotImport)
  {
    pOutputItem->setFlags(Qt::ItemFlag::ItemIsSelectable);
    pBrowse->setEnabled(false);
    QComboBox* pCombo = qobject_cast<QComboBox*>(table->cellWidget(uiRow, Columns::Method));
    pCombo->setEnabled(false);

    pBrowse->setEnabled(true);
    pBrowse->setText("Open");
    pStatusItem->setForeground(QColor::fromRgba(qRgb(0, 200, 0)));

    if (data2.m_sImportMessage.IsEmpty())
    {
      pStatusItem->setText("Asset Imported");
    }
  }
}

void plQtAssetImportDlg::SelectedOptionChanged(int index)
{
  QComboBox* pCombo = qobject_cast<QComboBox*>(sender());
  const plUInt32 uiRow = pCombo->property("row").toInt();

  QueryRow(uiRow);
  m_AllImports[uiRow].m_iSelectedOption = index - 1;
  UpdateRow(uiRow);
}

void plQtAssetImportDlg::QueryRow(plUInt32 uiRow)
{
  QTableWidget* table = AssetTable;
  auto& data2 = m_AllImports[uiRow];

  if (data2.m_iSelectedOption < 0)
    return;

  auto& option = data2.m_ImportOptions[data2.m_iSelectedOption];

  plStringBuilder file = table->item(uiRow, Columns::GeneratedDoc)->text().toUtf8().data();
  file.ChangeFileExtension(option.m_pGenerator->GetDocumentExtension());
  option.m_sOutputFileParentRelative = file;
}

void plQtAssetImportDlg::UpdateAllRows()
{
  for (plUInt32 i = 0; i < m_AllImports.GetCount(); ++i)
  {
    UpdateRow(i);
  }
}

void plQtAssetImportDlg::on_ButtonImport_clicked()
{
  plAssetDocumentGenerator::ExecuteImport(m_AllImports);

  UpdateAllRows();
  *m_Imported = true;

  close();
}

void plQtAssetImportDlg::TableCellChanged(int row, int column)
{
  if (column == Columns::GeneratedDoc)
  {
    QueryRow(row);
    UpdateRow(row);
  }
}

void plQtAssetImportDlg::BrowseButtonClicked(bool)
{
  QToolButton* pButton = qobject_cast<QToolButton*>(sender());
  const plUInt32 uiRow = pButton->property("row").toInt();

  auto& data2 = m_AllImports[uiRow];
  if (data2.m_iSelectedOption < 0)
    return;

  auto& option = data2.m_ImportOptions[data2.m_iSelectedOption];

  if (data2.m_bDoNotImport)
  {
    // asset was already imported

    plQtEditorApp::GetSingleton()->OpenDocumentQueued(option.m_sOutputFileAbsolute);
  }
  else
  {
    plStringBuilder filter;
    filter.Format("{0} (*.{0})", option.m_pGenerator->GetDocumentExtension());
    QString result = QFileDialog::getSaveFileName(this, "Target Document", plToolsProject::GetSingleton()->GetProjectDirectory().GetData(),
      filter.GetData(), nullptr, QFileDialog::Option::DontResolveSymlinks);

    if (result.isEmpty())
      return;

    plStringBuilder tmp = result.toUtf8().data();
    if (!plQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(tmp))
    {
      plQtUiServices::GetSingleton()->MessageBoxWarning("The selected file is not located in any of the project's data directories.");
      return;
    }

    option.m_sOutputFileAbsolute = result.toUtf8().data();
    option.m_sOutputFileParentRelative = tmp;
    UpdateRow(uiRow);
  }
}
