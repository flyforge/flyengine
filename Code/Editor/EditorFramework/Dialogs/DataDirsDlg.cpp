#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/DataDirsDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

plQtDataDirsDlg::plQtDataDirsDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  m_Config = plQtEditorApp::GetSingleton()->GetFileSystemConfig();
  m_iSelection = -1;
  FillList();
}

void plQtDataDirsDlg::FillList()
{
  if (m_Config.m_DataDirs.IsEmpty())
    m_iSelection = -1;

  if (m_iSelection != -1)
    m_iSelection = plMath::Clamp<plInt32>(m_iSelection, 0, m_Config.m_DataDirs.GetCount() - 1);

  ListDataDirs->blockSignals(true);

  ListDataDirs->clear();

  for (auto dd : m_Config.m_DataDirs)
  {
    QListWidgetItem* pItem = new QListWidgetItem(ListDataDirs);
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable /*| Qt::ItemFlag::ItemIsUserCheckable*/);

    QString sPath = QString::fromUtf8(dd.m_sDataDirSpecialPath.GetData());

    pItem->setText(sPath);
    ListDataDirs->addItem(pItem);

    if (dd.m_bHardCodedDependency)
    {
      QColor col;
      col.setNamedColor("Orange");
      pItem->setForeground(col);
      pItem->setToolTip("This data directory is a hard dependency and cannot be removed.");
      pItem->setData(Qt::UserRole + 1, false); // can remove ?
    }
    else
    {
      pItem->setData(Qt::UserRole + 1, true); // can remove ?
    }
  }

  ListDataDirs->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  if (m_iSelection == -1)
    ListDataDirs->clearSelection();
  else
    ListDataDirs->item(m_iSelection)->setSelected(true);


  ListDataDirs->blockSignals(false);

  on_ListDataDirs_itemSelectionChanged();
}

void plQtDataDirsDlg::on_ButtonOK_clicked()
{
  if (m_Config.CreateDataDirStubFiles().Failed())
  {
    plQtUiServices::MessageBoxWarning("Failed to create all data dir stub files ('DataDir.plManifest'). Please review the selected "
                                      "folders, some might not be accessible. See the log for more details.");
    return;
  }

  plQtEditorApp::GetSingleton()->SetFileSystemConfig(m_Config);
  accept();
}

void plQtDataDirsDlg::on_ButtonCancel_clicked()
{
  reject();
}

void plQtDataDirsDlg::on_ButtonUp_clicked()
{
  plMath::Swap(m_Config.m_DataDirs[m_iSelection - 1], m_Config.m_DataDirs[m_iSelection]);
  --m_iSelection;

  FillList();
}

void plQtDataDirsDlg::on_ButtonDown_clicked()
{
  plMath::Swap(m_Config.m_DataDirs[m_iSelection], m_Config.m_DataDirs[m_iSelection + 1]);
  ++m_iSelection;

  FillList();
}

void plQtDataDirsDlg::on_ButtonAdd_clicked()
{
  static QString sPreviousFolder;
  if (sPreviousFolder.isEmpty())
  {
    sPreviousFolder = QString::fromUtf8(plToolsProject::GetSingleton()->GetProjectFile().GetData());
  }

  QString sFolder = QFileDialog::getExistingDirectory(this, QLatin1String("Select Directory"), sPreviousFolder, QFileDialog::Option::ShowDirsOnly | QFileDialog::Option::DontResolveSymlinks);

  if (sFolder.isEmpty())
    return;

  sPreviousFolder = sFolder;

  plStringBuilder sRootPath = plFileSystem::GetSdkRootDirectory();

  plStringBuilder sRelPath = sFolder.toUtf8().data();
  sRelPath.MakeRelativeTo(sRootPath).IgnoreResult();
  sRelPath.Prepend(">sdk/");
  sRelPath.MakeCleanPath();

  plApplicationFileSystemConfig::DataDirConfig dd;
  dd.m_sDataDirSpecialPath = sRelPath;
  dd.m_bWritable = false;
  m_Config.m_DataDirs.PushBack(dd);

  m_iSelection = m_Config.m_DataDirs.GetCount() - 1;

  FillList();
}

void plQtDataDirsDlg::on_ButtonRemove_clicked()
{
  m_Config.m_DataDirs.RemoveAtAndCopy(m_iSelection);

  FillList();
}

void plQtDataDirsDlg::on_ListDataDirs_itemSelectionChanged()
{
  if (ListDataDirs->selectedItems().isEmpty())
    m_iSelection = -1;
  else
    m_iSelection = ListDataDirs->selectionModel()->selectedIndexes()[0].row();

  const bool bCanRemove = m_iSelection >= 0 && ListDataDirs->item(m_iSelection)->data(Qt::UserRole + 1).toBool();

  ButtonRemove->setEnabled(bCanRemove);
  ButtonUp->setEnabled(m_iSelection > 0);
  ButtonDown->setEnabled(m_iSelection != -1 && m_iSelection < (plInt32)m_Config.m_DataDirs.GetCount() - 1);
}

void plQtDataDirsDlg::on_ButtonOpenFolder_clicked()
{
  if (m_iSelection < 0)
    return;

  plStringBuilder sPath;
  plFileSystem::ResolveSpecialDirectory(m_Config.m_DataDirs[m_iSelection].m_sDataDirSpecialPath, sPath).IgnoreResult();

  plQtUiServices::OpenInExplorer(sPath.GetData(), true);
}

void plQtDataDirsDlg::on_ListDataDirs_itemDoubleClicked(QListWidgetItem* pItem)
{
  on_ButtonOpenFolder_clicked();
}
