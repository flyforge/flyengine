#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/InputConfigDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

void UpdateInputDynamicEnumValues()
{
  plHybridArray<plGameAppInputConfig, 32> Actions;

  plStringBuilder sPath = plToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/InputConfig.ddl");

#if PLASMA_ENABLED(PLASMA_MIGRATE_RUNTIMECONFIGS)
  plStringBuilder sOldPath = plToolsProject::GetSingleton()->GetProjectDirectory();
  sOldPath.AppendPath("InputConfig.ddl");
  sPath = plFileSystem::MigrateFileLocation(sOldPath, sPath);
#endif

  plFileReader file;
  if (file.Open(sPath).Failed())
    return;

  plGameAppInputConfig::ReadFromDDL(file, Actions);

  auto& dynEnum = plDynamicStringEnum::CreateDynamicEnum("InputSet");

  for (const auto& a : Actions)
  {
    dynEnum.AddValidValue(a.m_sInputSet, true);
  }
}

plQtInputConfigDlg::plQtInputConfigDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  LoadActions();

  plQtEditorApp::GetSingleton()->GetKnownInputSlots(m_AllInputSlots);

  // make sure existing slots are always in the list
  // to prevent losing data when some plugin is not loaded
  {
    for (const auto& action : m_Actions)
    {
      for (int i = 0; i < 3; ++i)
      {
        if (m_AllInputSlots.IndexOf(action.m_sInputSlotTrigger[i]) == plInvalidIndex)
          m_AllInputSlots.PushBack(action.m_sInputSlotTrigger[i]);
      }
    }
  }

  FillList();

  on_TreeActions_itemSelectionChanged();
}

void plQtInputConfigDlg::on_ButtonNewInputSet_clicked()
{
  QString sResult = QInputDialog::getText(this, "Input Set Name", "Name:");

  if (sResult.isEmpty())
    return;

  TreeActions->clearSelection();

  const plString sName = sResult.toUtf8().data();

  if (m_InputSetToItem.Find(sName).IsValid())
  {
    plQtUiServices::GetSingleton()->MessageBoxInformation("An Input Set with this name already exists.");
  }
  else
  {
    auto* pItem = new QTreeWidgetItem(TreeActions);
    pItem->setText(0, sResult);
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pItem->setIcon(0, plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Input.svg"));

    m_InputSetToItem[sName] = pItem;
  }

  m_InputSetToItem[sName]->setSelected(true);
}

void plQtInputConfigDlg::on_ButtonNewAction_clicked()
{
  if (TreeActions->selectedItems().isEmpty())
    return;

  auto pItem = TreeActions->selectedItems()[0];

  if (!pItem)
    return;

  if (TreeActions->indexOfTopLevelItem(pItem) < 0)
    pItem = pItem->parent();

  plGameAppInputConfig action;
  auto pNewItem = CreateActionItem(pItem, action);
  pItem->setExpanded(true);

  TreeActions->clearSelection();
  pNewItem->setSelected(true);
  TreeActions->editItem(pNewItem);
}

void plQtInputConfigDlg::on_ButtonRemove_clicked()
{
  if (TreeActions->selectedItems().isEmpty())
    return;

  auto pItem = TreeActions->selectedItems()[0];

  if (!pItem)
    return;

  if (TreeActions->indexOfTopLevelItem(pItem) >= 0)
  {
    if (plQtUiServices::GetSingleton()->MessageBoxQuestion(
          "Do you really want to remove the entire Input Set?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
      return;

    m_InputSetToItem.Remove(pItem->text(0).toUtf8().data());
  }
  else
  {
    if (plQtUiServices::GetSingleton()->MessageBoxQuestion(
          "Do you really want to remove this action?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
      return;
  }

  delete pItem;
}

void plQtInputConfigDlg::on_ButtonOk_clicked()
{
  GetActionsFromList();
  SaveActions();
  UpdateInputDynamicEnumValues();
  accept();
}

void plQtInputConfigDlg::on_ButtonCancel_clicked()
{
  reject();
}

void plQtInputConfigDlg::on_ButtonReset_clicked()
{
  LoadActions();
  FillList();
  on_TreeActions_itemSelectionChanged();
}

void plQtInputConfigDlg::on_TreeActions_itemSelectionChanged()
{
  const bool hasSelection = !TreeActions->selectedItems().isEmpty();

  ButtonRemove->setEnabled(hasSelection);
  ButtonNewAction->setEnabled(hasSelection);
}

void plQtInputConfigDlg::LoadActions()
{
  m_Actions.Clear();

  plStringBuilder sPath = plToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/InputConfig.ddl");

#if PLASMA_ENABLED(PLASMA_MIGRATE_RUNTIMECONFIGS)
  plStringBuilder sOldPath = plToolsProject::GetSingleton()->GetProjectDirectory();
  sOldPath.AppendPath("InputConfig.ddl");
  sPath = plFileSystem::MigrateFileLocation(sOldPath, sPath);
#endif

  plFileReader file;
  if (file.Open(sPath).Failed())
    return;

  plGameAppInputConfig::ReadFromDDL(file, m_Actions);
}

void plQtInputConfigDlg::SaveActions()
{
  plStringBuilder sPath = plToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/InputConfig.ddl");

  plDeferredFileWriter file;
  file.SetOutput(sPath);

  plGameAppInputConfig::WriteToDDL(file, m_Actions);

  if (file.Close().Failed())
    plLog::Error("Failed to save '{0}'.", sPath);
}

void plQtInputConfigDlg::FillList()
{
  plQtScopedBlockSignals bs(TreeActions);
  plQtScopedUpdatesDisabled bu(TreeActions);

  m_InputSetToItem.Clear();
  TreeActions->clear();

  plSet<plString> InputSets;

  for (const auto& action : m_Actions)
  {
    InputSets.Insert(action.m_sInputSet);
  }

  for (auto it = InputSets.GetIterator(); it.IsValid(); ++it)
  {
    auto* pItem = new QTreeWidgetItem(TreeActions);
    pItem->setText(0, it.Key().GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pItem->setIcon(0, plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Input.svg"));

    m_InputSetToItem[it.Key()] = pItem;
  }

  for (const auto& action : m_Actions)
  {
    QTreeWidgetItem* pParentItem = m_InputSetToItem[action.m_sInputSet];

    CreateActionItem(pParentItem, action);


    pParentItem->setExpanded(true);
  }

  TreeActions->resizeColumnToContents(0);
  TreeActions->resizeColumnToContents(1);
  TreeActions->resizeColumnToContents(2);
  TreeActions->resizeColumnToContents(3);
  TreeActions->resizeColumnToContents(4);
  TreeActions->resizeColumnToContents(5);
  TreeActions->resizeColumnToContents(6);
  TreeActions->resizeColumnToContents(7);
}

void plQtInputConfigDlg::GetActionsFromList()
{
  m_Actions.Clear();

  for (int sets = 0; sets < TreeActions->topLevelItemCount(); ++sets)
  {
    const auto* pSetItem = TreeActions->topLevelItem(sets);
    const plString sSetName = pSetItem->text(0).toUtf8().data();

    for (int children = 0; children < pSetItem->childCount(); ++children)
    {
      plGameAppInputConfig& cfg = m_Actions.ExpandAndGetRef();
      cfg.m_sInputSet = sSetName;

      auto* pActionItem = pSetItem->child(children);

      cfg.m_sInputAction = pActionItem->text(0).toUtf8().data();
      cfg.m_bApplyTimeScaling = qobject_cast<QCheckBox*>(TreeActions->itemWidget(pActionItem, 1))->isChecked();

      for (int i = 0; i < 3; ++i)
      {
        cfg.m_sInputSlotTrigger[i] = qobject_cast<QComboBox*>(TreeActions->itemWidget(pActionItem, 2 + i * 2))->currentText().toUtf8().data();
        cfg.m_fInputSlotScale[i] = qobject_cast<QDoubleSpinBox*>(TreeActions->itemWidget(pActionItem, 3 + i * 2))->value();
      }
    }
  }
}

QTreeWidgetItem* plQtInputConfigDlg::CreateActionItem(QTreeWidgetItem* pParentItem, const plGameAppInputConfig& action)
{
  auto* pItem = new QTreeWidgetItem(pParentItem);
  pItem->setText(0, action.m_sInputAction.GetData());
  pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEditable);

  QCheckBox* pTimeScale = new QCheckBox(TreeActions);
  pTimeScale->setChecked(action.m_bApplyTimeScaling);
  TreeActions->setItemWidget(pItem, 1, pTimeScale);

  for (int i = 0; i < 3; ++i)
  {
    QDoubleSpinBox* spin = new QDoubleSpinBox(TreeActions);
    spin->setDecimals(3);
    spin->setMinimum(0.0);
    spin->setMaximum(100.0);
    spin->setSingleStep(0.01);
    spin->setValue(action.m_fInputSlotScale[i]);

    TreeActions->setItemWidget(pItem, 3 + 2 * i, spin);

    QComboBox* combo = new QComboBox(TreeActions);
    combo->setEditable(true);

    QCompleter* completer = new QCompleter(this);
    completer->setModel(combo->model());
    combo->setCompleter(completer);
    combo->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    combo->setInsertPolicy(QComboBox::InsertAtBottom);
    combo->setMaxVisibleItems(15);

    for (plUInt32 it = 0; it < m_AllInputSlots.GetCount(); ++it)
    {
      combo->addItem(m_AllInputSlots[it].GetData());
    }

    int index = combo->findText(action.m_sInputSlotTrigger[i].GetData());

    if (index > 0)
      combo->setCurrentIndex(index);
    else
      combo->setCurrentIndex(0);

    TreeActions->setItemWidget(pItem, 2 + 2 * i, combo);
  }

  return pItem;
}
