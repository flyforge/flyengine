#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <QCheckBox>
#include <QInputDialog>

void UpdateCollisionLayerDynamicEnumValues();

plQtJoltProjectSettingsDlg::plQtJoltProjectSettingsDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  ButtonRemoveLayer->setEnabled(false);
  ButtonRenameLayer->setEnabled(false);

  EnsureConfigFileExists();

  Load().IgnoreResult();
  SetupTable();
}


void plQtJoltProjectSettingsDlg::EnsureConfigFileExists()
{
  plStringView sConfigFile = plCollisionFilterConfig::s_sConfigFile;

#if PL_ENABLED(PL_MIGRATE_RUNTIMECONFIGS)
  sConfigFile = plFileSystem::MigrateFileLocation(":project/CollisionLayers.cfg", sConfigFile);
#endif

  if (plFileSystem::ExistsFile(sConfigFile))
    return;

  plCollisionFilterConfig cfg;

  cfg.SetGroupName(0, "Default");
  cfg.SetGroupName(1, "Transparent");
  cfg.SetGroupName(2, "Debris");
  cfg.SetGroupName(3, "Water");
  cfg.SetGroupName(4, "Foliage");
  cfg.SetGroupName(5, "AI");
  cfg.SetGroupName(6, "Player");
  cfg.SetGroupName(7, "Visibility Raycast");
  cfg.SetGroupName(8, "Interaction Raycast");

  cfg.EnableCollision(0, 0);
  cfg.EnableCollision(0, 1);
  cfg.EnableCollision(0, 2);
  cfg.EnableCollision(0, 3, false);
  cfg.EnableCollision(0, 4, false);
  cfg.EnableCollision(0, 5);
  cfg.EnableCollision(0, 6);
  cfg.EnableCollision(0, 7);
  cfg.EnableCollision(0, 8);

  cfg.EnableCollision(1, 1);
  cfg.EnableCollision(1, 2);
  cfg.EnableCollision(1, 3, false);
  cfg.EnableCollision(1, 4, false);
  cfg.EnableCollision(1, 5);
  cfg.EnableCollision(1, 6);
  cfg.EnableCollision(1, 7, false);
  cfg.EnableCollision(1, 8);

  cfg.EnableCollision(2, 2, false);
  cfg.EnableCollision(2, 3, false);
  cfg.EnableCollision(2, 4, false);
  cfg.EnableCollision(2, 5, false);
  cfg.EnableCollision(2, 6, false);
  cfg.EnableCollision(2, 7, false);
  cfg.EnableCollision(2, 8, false);

  cfg.EnableCollision(3, 3, false);
  cfg.EnableCollision(3, 4, false);
  cfg.EnableCollision(3, 5, false);
  cfg.EnableCollision(3, 6, false);
  cfg.EnableCollision(3, 7, false);
  cfg.EnableCollision(3, 8);

  cfg.EnableCollision(4, 4, false);
  cfg.EnableCollision(4, 5, false);
  cfg.EnableCollision(4, 6, false);
  cfg.EnableCollision(4, 7);
  cfg.EnableCollision(4, 8, false);

  cfg.EnableCollision(5, 5);
  cfg.EnableCollision(5, 6);
  cfg.EnableCollision(5, 7);
  cfg.EnableCollision(5, 8);

  cfg.EnableCollision(6, 6);
  cfg.EnableCollision(6, 7);
  cfg.EnableCollision(6, 8);

  cfg.EnableCollision(7, 7, false);
  cfg.EnableCollision(7, 8, false);

  cfg.EnableCollision(8, 8, false);

  cfg.Save().IgnoreResult();

  UpdateCollisionLayerDynamicEnumValues();
}

void plQtJoltProjectSettingsDlg::SetupTable()
{
  plQtScopedBlockSignals s1(FilterTable);
  plQtScopedUpdatesDisabled s2(FilterTable);

  const plUInt32 uiLayers = m_Config.GetNumNamedGroups();

  FilterTable->setRowCount(uiLayers);
  FilterTable->setColumnCount(uiLayers);
  FilterTable->horizontalHeader()->setHighlightSections(false);

  QStringList headers;
  plStringBuilder tmp;

  for (plUInt32 r = 0; r < uiLayers; ++r)
  {
    m_IndexRemap[r] = m_Config.GetNamedGroupIndex(r);

    headers.push_back(QString::fromUtf8(m_Config.GetGroupName(m_IndexRemap[r]).GetData(tmp)));
  }

  FilterTable->setVerticalHeaderLabels(headers);
  FilterTable->setHorizontalHeaderLabels(headers);

  for (plUInt32 r = 0; r < uiLayers; ++r)
  {
    for (plUInt32 c = 0; c < uiLayers; ++c)
    {
      QCheckBox* pCheck = new QCheckBox();
      pCheck->setText(QString());
      pCheck->setChecked(m_Config.IsCollisionEnabled(m_IndexRemap[r], m_IndexRemap[c]));
      pCheck->setProperty("column", c);
      pCheck->setProperty("row", r);
      connect(pCheck, &QCheckBox::clicked, this, &plQtJoltProjectSettingsDlg::onCheckBoxClicked);

      QWidget* pWidget = new QWidget();
      QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
      pLayout->addWidget(pCheck);
      pLayout->setAlignment(Qt::AlignCenter);
      pLayout->setContentsMargins(0, 0, 0, 0);
      pWidget->setLayout(pLayout);

      FilterTable->setCellWidget(r, c, pWidget);
    }
  }
}

plResult plQtJoltProjectSettingsDlg::Save()
{
  if (m_Config.Save().Failed())
  {
    plStringBuilder sError;
    sError.SetFormat("Failed to save the Collision Layer file\n'{0}'", plCollisionFilterConfig::s_sConfigFile);

    plQtUiServices::GetSingleton()->MessageBoxWarning(sError);

    return PL_FAILURE;
  }

  UpdateCollisionLayerDynamicEnumValues();

  return PL_SUCCESS;
}

plResult plQtJoltProjectSettingsDlg::Load()
{
  auto res = m_Config.Load();

  m_ConfigReset = m_Config;
  return res;
}

void plQtJoltProjectSettingsDlg::onCheckBoxClicked(bool checked)
{
  QCheckBox* pCheck = qobject_cast<QCheckBox*>(sender());

  const plInt32 c = pCheck->property("column").toInt();
  const plInt32 r = pCheck->property("row").toInt();

  m_Config.EnableCollision(m_IndexRemap[c], m_IndexRemap[r], pCheck->isChecked());

  if (r != c)
  {
    QCheckBox* pCheck2 = qobject_cast<QCheckBox*>(FilterTable->cellWidget(c, r)->layout()->itemAt(0)->widget());
    pCheck2->setChecked(pCheck->isChecked());
  }
}

void plQtJoltProjectSettingsDlg::on_DefaultButtons_clicked(QAbstractButton* pButton)
{
  if (pButton == DefaultButtons->button(QDialogButtonBox::Ok))
  {
    if (Save().Failed())
      return;

    accept();
    return;
  }

  if (pButton == DefaultButtons->button(QDialogButtonBox::Cancel))
  {
    reject();
    return;
  }

  if (pButton == DefaultButtons->button(QDialogButtonBox::Reset))
  {
    m_Config = m_ConfigReset;
    SetupTable();
    return;
  }
}

void plQtJoltProjectSettingsDlg::on_ButtonAddLayer_clicked()
{
  const plUInt32 uiNewIdx = m_Config.FindUnnamedGroup();

  if (uiNewIdx == plInvalidIndex)
  {
    plQtUiServices::GetSingleton()->MessageBoxInformation("The maximum number of collision layers has been reached.");
    return;
  }

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Add Layer"), QStringLiteral("Name:"), QLineEdit::Normal, QString(), &ok);

    if (!ok)
      return;

    if (m_Config.GetFilterGroupByName(result.toUtf8().data()) != plInvalidIndex)
    {
      plQtUiServices::GetSingleton()->MessageBoxWarning("A Collision Layer with the given name already exists.");
      continue;
    }

    m_Config.SetGroupName(uiNewIdx, result.toUtf8().data());
    break;
  }

  SetupTable();
}

void plQtJoltProjectSettingsDlg::on_ButtonRemoveLayer_clicked()
{
  if (plQtUiServices::GetSingleton()->MessageBoxQuestion("Remove selected Collision Layer?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::No)
    return;

  const auto sel = FilterTable->selectionModel()->selectedRows();

  if (sel.isEmpty())
    return;

  const int iRow = sel[0].row();

  m_Config.SetGroupName(m_IndexRemap[iRow], "");

  SetupTable();

  FilterTable->clearSelection();
}

void plQtJoltProjectSettingsDlg::on_ButtonRenameLayer_clicked()
{
  const auto sel = FilterTable->selectionModel()->selectedRows();

  if (sel.isEmpty())
    return;

  const int iGroupIdx = m_IndexRemap[sel[0].row()];
  const plString sOldName = m_Config.GetGroupName(iGroupIdx);

  m_Config.SetGroupName(iGroupIdx, "");

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Rename Layer"), QStringLiteral("Name:"), QLineEdit::Normal, QString::fromUtf8(sOldName.GetData()), &ok);

    if (!ok)
    {
      m_Config.SetGroupName(iGroupIdx, sOldName);
      return;
    }

    if (m_Config.GetFilterGroupByName(result.toUtf8().data()) != plInvalidIndex)
    {
      plQtUiServices::GetSingleton()->MessageBoxWarning("A Collision Layer with the given name already exists.");
      continue;
    }

    m_Config.SetGroupName(iGroupIdx, result.toUtf8().data());
    SetupTable();

    return;
  }
}

void plQtJoltProjectSettingsDlg::on_FilterTable_itemSelectionChanged()
{
  const auto sel = FilterTable->selectionModel()->selectedRows();
  ButtonRemoveLayer->setEnabled(!sel.isEmpty());
  ButtonRenameLayer->setEnabled(!sel.isEmpty());
}
