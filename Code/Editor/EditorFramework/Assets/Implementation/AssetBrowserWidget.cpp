#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetBrowserWidget.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QFile>

plQtAssetBrowserWidget::plQtAssetBrowserWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  ButtonListMode->setVisible(false);
  ButtonIconMode->setVisible(false);

  m_pFilter = new plQtAssetBrowserFilter(this);
  TreeFolderFilter->SetFilter(m_pFilter);

  m_pModel = new plQtAssetBrowserModel(this, m_pFilter);
  SearchWidget->setPlaceholderText("Search Assets");

  IconSizeSlider->setValue(50);

  ListAssets->setModel(m_pModel);
  ListAssets->SetIconScale(IconSizeSlider->value());
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  on_ButtonIconMode_clicked();

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  // Tool Bar
  {
    m_pToolbar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "AssetBrowserToolBar";
    context.m_pDocument = nullptr;
    m_pToolbar->SetActionContext(context);
    m_pToolbar->setObjectName("AssetBrowserToolBar");
    ToolBarLayout->insertWidget(0, m_pToolbar);
  }



  PL_VERIFY(connect(m_pFilter, SIGNAL(TextFilterChanged()), this, SLOT(OnTextFilterChanged())) != nullptr, "signal/slot connection failed");
  PL_VERIFY(connect(m_pFilter, SIGNAL(TypeFilterChanged()), this, SLOT(OnTypeFilterChanged())) != nullptr, "signal/slot connection failed");
  PL_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
  PL_VERIFY(connect(m_pModel, SIGNAL(modelReset()), this, SLOT(OnModelReset())) != nullptr, "signal/slot connection failed");
  PL_VERIFY(connect(m_pModel, &plQtAssetBrowserModel::editingFinished, this, &plQtAssetBrowserWidget::OnFileEditingFinished, Qt::QueuedConnection), "signal/slot connection failed");

  PL_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(OnAssetSelectionChanged(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  PL_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(OnAssetSelectionCurrentChanged(const QModelIndex&, const QModelIndex&))) != nullptr, "signal/slot connection failed");
  connect(SearchWidget, &plQtSearchWidget::textChanged, this, &plQtAssetBrowserWidget::OnSearchWidgetTextChanged);

  UpdateAssetTypes();

  plAssetCurator::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plQtAssetBrowserWidget::AssetCuratorEventHandler, this));
  plToolsProject::s_Events.AddEventHandler(plMakeDelegate(&plQtAssetBrowserWidget::ProjectEventHandler, this));
}

plQtAssetBrowserWidget::~plQtAssetBrowserWidget()
{
  plToolsProject::s_Events.RemoveEventHandler(plMakeDelegate(&plQtAssetBrowserWidget::ProjectEventHandler, this));
  plAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtAssetBrowserWidget::AssetCuratorEventHandler, this));


  ListAssets->setModel(nullptr);
}

void plQtAssetBrowserWidget::UpdateAssetTypes()
{
  const auto& assetTypes0 = plAssetDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  plMap<plString, const plDocumentTypeDescriptor*> assetTypes;
  for (auto it : assetTypes0)
  {
    assetTypes[plTranslate(it.Key())] = it.Value();
  }

  {
    plQtScopedBlockSignals block(TypeFilter);

    TypeFilter->clear();

    if (m_Mode == Mode::Browser)
    {
      // '<All Files>' Filter
      TypeFilter->addItem(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("<All Files>"));

      // '<Importable Files>' Filter
      TypeFilter->addItem(QIcon(QLatin1String(":/EditorFramework/Icons/ImportableFileType.svg")), QLatin1String("<Importable Files>"));
    }

    // '<All Assets>' Filter
    TypeFilter->addItem(QIcon(QLatin1String(":/AssetIcons/Icons/AllAssets.svg")), QLatin1String("<All Assets>"));

    for (const auto& it : assetTypes)
    {
      TypeFilter->addItem(plQtUiServices::GetCachedIconResource(it.Value()->m_sIcon, plColorScheme::GetCategoryColor(it.Value()->m_sAssetCategory, plColorScheme::CategoryColorUsage::AssetMenuIcon)), QString::fromUtf8(it.Key(), it.Key().GetElementCount()));
      TypeFilter->setItemData(TypeFilter->count() - 1, QString::fromUtf8(it.Value()->m_sDocumentTypeName, it.Value()->m_sDocumentTypeName.GetElementCount()), Qt::UserRole);
    }
  }

  // make sure to apply the previously active type filter settings to the UI
  if (m_Mode == Mode::Browser)
  {
    plSet<plString> importExtensions;
    plAssetDocumentGenerator::GetSupportsFileTypes(importExtensions);
    m_pFilter->UpdateImportExtensions(importExtensions);
  }

  OnTypeFilterChanged();
}

void plQtAssetBrowserWidget::SetMode(Mode mode)
{
  if (m_Mode == mode)
    return;

  m_Mode = mode;

  switch (m_Mode)
  {
    case Mode::Browser:
      m_pToolbar->show();
      TreeFolderFilter->SetDialogMode(false);
      ListAssets->SetDialogMode(false);
      break;
    case Mode::FilePicker:
      TypeFilter->setVisible(false);
      [[fallthrough]];
    case Mode::AssetPicker:
      m_pToolbar->hide();
      TreeFolderFilter->SetDialogMode(true);
      ListAssets->SetDialogMode(true);
      break;
  }

  UpdateAssetTypes();
}

void plQtAssetBrowserWidget::SaveState(const char* szSettingsName)
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String(szSettingsName));
  {
    Settings.setValue("SplitterGeometry", splitter->saveGeometry());
    Settings.setValue("SplitterState", splitter->saveState());
    Settings.setValue("IconSize", IconSizeSlider->value());
    Settings.setValue("IconMode", ListAssets->viewMode() == QListView::ViewMode::IconMode);
  }
  Settings.endGroup();
}

void plQtAssetBrowserWidget::RestoreState(const char* szSettingsName)
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String(szSettingsName));
  {
    splitter->restoreGeometry(Settings.value("SplitterGeometry", splitter->saveGeometry()).toByteArray());
    splitter->restoreState(Settings.value("SplitterState", splitter->saveState()).toByteArray());
    IconSizeSlider->setValue(Settings.value("IconSize", IconSizeSlider->value()).toInt());

    if (Settings.value("IconMode", ListAssets->viewMode() == QListView::ViewMode::IconMode).toBool())
      on_ButtonIconMode_clicked();
    else
      on_ButtonListMode_clicked();
  }
  Settings.endGroup();
}

void plQtAssetBrowserWidget::ProjectEventHandler(const plToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case plToolsProjectEvent::Type::ProjectOpened:
    {
      // this is necessary to detect new asset types when a plugin has been loaded (on project load)
      UpdateAssetTypes();
    }
    break;
    case plToolsProjectEvent::Type::ProjectClosed:
    {
      m_pFilter->Reset();
    }
    break;
    default:
      break;
  }
}

void plQtAssetBrowserWidget::AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset)
{
  if (m_Mode != Mode::Browser)
    return;

  const plHybridArray<plDocumentManager*, 16>& managers = plDocumentManager::GetAllDocumentManagers();

  plDynamicArray<const plDocumentTypeDescriptor*> documentTypes;

  QMenu* pSubMenu = pMenu->addMenu("New");

  plStringBuilder sTypeFilter = m_pFilter->GetTypeFilter();

  for (plDocumentManager* pMan : managers)
  {
    if (!pMan->GetDynamicRTTI()->IsDerivedFrom<plAssetDocumentManager>())
      continue;

    pMan->GetSupportedDocumentTypes(documentTypes);
  }

  documentTypes.Sort([](const plDocumentTypeDescriptor* a, const plDocumentTypeDescriptor* b) -> bool
    { return plTranslate(a->m_sDocumentTypeName).Compare(plTranslate(b->m_sDocumentTypeName)) < 0; });

  QAction* pAction = pSubMenu->addAction(plMakeQString(plTranslate("Folder")));
  pAction->setIcon(plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Folder.svg"));
  connect(pAction, &QAction::triggered, static_cast<eqQtAssetBrowserFolderView*>(TreeFolderFilter), &eqQtAssetBrowserFolderView::NewFolder);

  pSubMenu->addSeparator();

  for (const plDocumentTypeDescriptor* desc : documentTypes)
  {
    if (!desc->m_bCanCreate || desc->m_sFileExtension.IsEmpty())
      continue;

    QAction* pAction = pSubMenu->addAction(plMakeQString(plTranslate(desc->m_sDocumentTypeName)));
    pAction->setIcon(plQtUiServices::GetSingleton()->GetCachedIconResource(desc->m_sIcon, plColorScheme::GetCategoryColor(desc->m_sAssetCategory, plColorScheme::CategoryColorUsage::MenuEntryIcon)));
    pAction->setProperty("AssetType", desc->m_sDocumentTypeName.GetData());
    pAction->setProperty("AssetManager", QVariant::fromValue<void*>(desc->m_pManager));
    pAction->setProperty("Extension", desc->m_sFileExtension.GetData());
    pAction->setProperty("UseSelection", useSelectedAsset);

    connect(pAction, &QAction::triggered, this, &plQtAssetBrowserWidget::NewAsset);
  }
}


void plQtAssetBrowserWidget::AddImportedViaMenu(QMenu* pMenu)
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  // Find all uses
  plSet<plUuid> importedVia;
  for (const QModelIndex& id : selection)
  {
    const bool bImportable = id.data(plQtAssetBrowserModel::UserRoles::Importable).toBool();
    if (!bImportable)
      continue;

    plString sAbsPath = qtToPlString(m_pModel->data(id, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
    plAssetCurator::GetSingleton()->FindAllUses(sAbsPath, importedVia);
  }

  // Sort by path
  plHybridArray<plUuid, 8> importedViaSorted;
  {
    importedViaSorted.Reserve(importedVia.GetCount());
    plAssetCurator::plLockedSubAssetTable allAssets = plAssetCurator::GetSingleton()->GetKnownSubAssets();
    for (const plUuid& guid : importedVia)
    {
      if (allAssets->Contains(guid))
        importedViaSorted.PushBack(guid);
    }

    importedViaSorted.Sort([&](const plUuid& a, const plUuid& b) -> bool
      { return allAssets->Find(a).Value().m_pAssetInfo->m_Path.GetDataDirParentRelativePath().Compare(allAssets->Find(b).Value().m_pAssetInfo->m_Path.GetDataDirParentRelativePath()) < 0; });
  }

  if (importedViaSorted.IsEmpty())
    return;

  // Create actions to open
  QMenu* pSubMenu = pMenu->addMenu("Imported via");
  pSubMenu->setIcon(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")));

  for (const plUuid& guid : importedViaSorted)
  {
    const plAssetCurator::plLockedSubAsset pSubAsset = plAssetCurator::GetSingleton()->GetSubAsset(guid);
    QIcon icon = plQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, plColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, plColorScheme::CategoryColorUsage::OverlayIcon));
    QString sRelPath = plMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());

    QAction* pAction = pSubMenu->addAction(sRelPath);
    pAction->setIcon(icon);
    pAction->setProperty("AbsPath", plMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath()));
    connect(pAction, &QAction::triggered, this, &plQtAssetBrowserWidget::OnOpenImportReferenceAsset);
  }
}

void plQtAssetBrowserWidget::GetSelectedImportableFiles(plDynamicArray<plString>& out_Files) const
{
  out_Files.Clear();

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    const bool bImportable = id.data(plQtAssetBrowserModel::UserRoles::Importable).toBool();
    if (bImportable)
    {
      out_Files.PushBack(qtToPlString(id.data(plQtAssetBrowserModel::UserRoles::AbsolutePath).toString()));
    }
  }
}

void plQtAssetBrowserWidget::on_ListAssets_clicked(const QModelIndex& index)
{
  const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)index.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  Q_EMIT ItemSelected(m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
}

void plQtAssetBrowserWidget::on_ListAssets_activated(const QModelIndex& index)
{
  const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)index.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  Q_EMIT ItemSelected(m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
}

void plQtAssetBrowserWidget::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)index.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  const plUuid guid = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>();

  if (itemType.IsAnySet(plAssetBrowserItemFlags::Asset | plAssetBrowserItemFlags::SubAsset))
  {
    if (guid.IsValid())
    {
      plAssetCurator::GetSingleton()->UpdateAssetLastAccessTime(guid);
    }
    Q_EMIT ItemChosen(guid, m_pModel->data(index, plQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
  else if (itemType.IsSet(plAssetBrowserItemFlags::File))
  {
    Q_EMIT ItemChosen(plUuid::MakeInvalid(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());

    // plQtUiServices::OpenFileInDefaultProgram(qtToPlString(sAbsPath));
  }
  else if (itemType.IsAnySet(plAssetBrowserItemFlags::Folder | plAssetBrowserItemFlags::DataDirectory))
  {
    m_pFilter->SetPathFilter(m_pModel->data(index, plQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data());
  }
}

void plQtAssetBrowserWidget::on_ButtonListMode_clicked()
{
  m_pModel->SetIconMode(false);
  ListAssets->SetIconMode(false);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(true);
  ButtonIconMode->setChecked(false);
}

void plQtAssetBrowserWidget::on_ButtonIconMode_clicked()
{
  m_pModel->SetIconMode(true);
  ListAssets->SetIconMode(true);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(false);
  ButtonIconMode->setChecked(true);
}

void plQtAssetBrowserWidget::on_IconSizeSlider_valueChanged(int iValue)
{
  ListAssets->SetIconScale(iValue);
}

void plQtAssetBrowserWidget::on_ListAssets_ViewZoomed(plInt32 iIconSizePercentage)
{
  plQtScopedBlockSignals block(IconSizeSlider);
  IconSizeSlider->setValue(iIconSizePercentage);
}

void plQtAssetBrowserWidget::OnTextFilterChanged()
{
  QString sText = plMakeQString(m_pFilter->GetTextFilter());
  if (SearchWidget->text() != sText)
  {
    SearchWidget->setText(sText);
    QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
  }
}

void plQtAssetBrowserWidget::OnTypeFilterChanged()
{
  plStringBuilder sTemp;
  const plStringBuilder sFilter(";", m_pFilter->GetTypeFilter(), ";");

  {
    plQtScopedBlockSignals _(TypeFilter);

    plInt32 iCheckedFilter = 0;
    plInt32 iNumChecked = 0;

    for (plInt32 i = 1; i < TypeFilter->count(); ++i)
    {
      sTemp.Set(";", TypeFilter->itemData(i, Qt::UserRole).toString().toUtf8().data(), ";");

      if (sFilter.FindSubString(sTemp) != nullptr)
      {
        ++iNumChecked;
        iCheckedFilter = i;
      }
    }

    if (iNumChecked == 3)
      TypeFilter->setCurrentIndex(iCheckedFilter);
    else
      TypeFilter->setCurrentIndex((m_Mode != Mode::Browser) ? 0 : 2); // "<All Assets>"

    int index = TypeFilter->currentIndex();

    switch (m_Mode)
    {
      case Mode::Browser:
        m_pFilter->SetShowNonImportableFiles(index == 0);
        m_pFilter->SetShowFiles(index == 0 || index == 1);
        break;
      case Mode::AssetPicker:
        m_pFilter->SetShowNonImportableFiles(false);
        m_pFilter->SetShowFiles(false);
        break;
      case Mode::FilePicker:
        m_pFilter->SetShowNonImportableFiles(true);
        m_pFilter->SetShowFiles(true);
        break;
    }
  }

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void plQtAssetBrowserWidget::OnPathFilterChanged()
{
  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void plQtAssetBrowserWidget::OnSearchWidgetTextChanged(const QString& text)
{
  m_pFilter->SetTextFilter(text.toUtf8().data());
}

void plQtAssetBrowserWidget::keyPressEvent(QKeyEvent* e)
{
  QWidget::keyPressEvent(e);

  if (e->key() == Qt::Key_Delete && m_Mode == Mode::Browser)
  {
    e->accept();
    DeleteSelection();
    return;
  }

  if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
  {
    e->accept();
    OnListOpenAssetDocument();
    return;
  }
}


void plQtAssetBrowserWidget::DeleteSelection()
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)id.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    if (itemType.IsAnySet(plAssetBrowserItemFlags::SubAsset | plAssetBrowserItemFlags::DataDirectory))
    {
      plQtUiServices::MessageBoxWarning(plFmt("Sub-assets and data directories can't be deleted."));
      return;
    }
  }

  QMessageBox::StandardButton choice = plQtUiServices::MessageBoxQuestion(plFmt("Do you want to delete the selected items?"), QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Yes);
  if (choice == QMessageBox::StandardButton::Cancel)
    return;

  for (const QModelIndex& id : selection)
  {
    const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)id.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    QString sQtAbsPath = id.data(plQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    plString sAbsPath = qtToPlString(sQtAbsPath);

    if (itemType.IsSet(plAssetBrowserItemFlags::File))
    {
      if (!QFile::moveToTrash(sQtAbsPath))
      {
        plLog::Error("Failed to delete file '{}'", sAbsPath);
      }
    }
    else
    {
      if (!QFile::moveToTrash(sQtAbsPath))
      {
        plLog::Error("Failed to delete folder '{}'", sAbsPath);
      }
    }
    plFileSystemModel::GetSingleton()->NotifyOfChange(sAbsPath);
  }
}

void plQtAssetBrowserWidget::OnImportAsAboutToShow()
{
  QMenu* pMenu = qobject_cast<QMenu*>(sender());

  if (!pMenu->actions().isEmpty())
    return;

  plHybridArray<plString, 8> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  if (filesToImport.IsEmpty())
    return;

  plSet<plString> extensions;
  plStringBuilder sExt;
  for (const auto& file : filesToImport)
  {
    sExt = file.GetFileExtension();
    sExt.ToLower();
    extensions.Insert(sExt);
  }

  plHybridArray<plAssetDocumentGenerator*, 16> generators;
  plAssetDocumentGenerator::CreateGenerators(generators);
  plHybridArray<plAssetDocumentGenerator::ImportMode, 16> importModes;

  for (plAssetDocumentGenerator* pGen : generators)
  {
    for (plStringView ext : extensions)
    {
      if (pGen->SupportsFileType(ext))
      {
        pGen->GetImportModes({}, importModes);
        break;
      }
    }
  }

  for (const auto& mode : importModes)
  {
    QAction* act = pMenu->addAction(QIcon(plMakeQString(mode.m_sIcon)), plMakeQString(plTranslate(mode.m_sName)));
    act->setData(plMakeQString(mode.m_sName));
    connect(act, &QAction::triggered, this, &plQtAssetBrowserWidget::OnImportAsClicked);
  }

  plAssetDocumentGenerator::DestroyGenerators(generators);
}

void plQtAssetBrowserWidget::OnImportAsClicked()
{
  plHybridArray<plString, 8> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  QAction* act = qobject_cast<QAction*>(sender());
  plString sMode = qtToPlString(act->data().toString());

  plHybridArray<plAssetDocumentGenerator*, 16> generators;
  plAssetDocumentGenerator::CreateGenerators(generators);

  plHybridArray<plAssetDocumentGenerator::ImportMode, 16> importModes;
  for (plAssetDocumentGenerator* pGen : generators)
  {
    importModes.Clear();
    pGen->GetImportModes({}, importModes);

    for (const auto& mode : importModes)
    {
      if (mode.m_sName == sMode)
      {
        for (const plString& file : filesToImport)
        {
          if (pGen->SupportsFileType(file))
          {
            pGen->Import(file, sMode, true).LogFailure();
          }
        }

        goto done;
      }
    }
  }

done:
  plAssetDocumentGenerator::DestroyGenerators(generators);
}

void plQtAssetBrowserWidget::AssetCuratorEventHandler(const plAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case plAssetCuratorEvent::Type::AssetListReset:
      UpdateAssetTypes();
      break;
    default:
      break;
  }
}

void plQtAssetBrowserWidget::on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  const bool bClickedValid = TreeFolderFilter->indexAt(pt).isValid();

  if (bClickedValid)
  {
    const bool bIsRoot = TreeFolderFilter->currentItem() && TreeFolderFilter->currentItem() == TreeFolderFilter->topLevelItem(0);
    if (TreeFolderFilter->currentItem() && !bIsRoot)
    {
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), TreeFolderFilter, SLOT(TreeOpenExplorer()));
    }

    if (TreeFolderFilter->currentItem() && !bIsRoot)
    {
      // Delete
      const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)TreeFolderFilter->currentItem()->data(0, plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      QAction* pDelete = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), TreeFolderFilter, &eqQtAssetBrowserFolderView::DeleteFolder);
      if (itemType.IsSet(plAssetBrowserItemFlags::DataDirectory))
      {
        pDelete->setEnabled(false);
        pDelete->setToolTip("Data directories can't be deleted.");
      }

      // Create
      AddAssetCreatorMenu(&m, false);
    }

    m.addSeparator();
  }

  {
    QAction* pAction = m.addAction(QLatin1String("Show Items in sub-folders"), this, SLOT(OnShowSubFolderItemsToggled()));
    pAction->setCheckable(true);
    pAction->setChecked(m_pFilter->GetShowItemsInSubFolders());
  }

  {
    QAction* pAction = m.addAction(QLatin1String("Show Items in hidden folders"), this, SLOT(OnShowHiddenFolderItemsToggled()));
    pAction->setCheckable(true);
    pAction->setChecked(m_pFilter->GetShowItemsInHiddenFolders());
    pAction->setEnabled(m_pFilter->GetShowItemsInSubFolders());
    pAction->setToolTip("Whether to ignore '_data' folders when showing items in sub-folders is enabled.");
  }

  m.exec(TreeFolderFilter->viewport()->mapToGlobal(pt));
}

void plQtAssetBrowserWidget::on_TypeFilter_currentIndexChanged(int index)
{
  plQtScopedBlockSignals block(TypeFilter);

  plStringBuilder sFilter;

  switch (m_Mode)
  {
    case Mode::Browser:
      m_pFilter->SetShowNonImportableFiles(index == 0);
      m_pFilter->SetShowFiles(index == 0 || index == 1);
      break;
    case Mode::AssetPicker:
      m_pFilter->SetShowNonImportableFiles(false);
      m_pFilter->SetShowFiles(false);
      break;
    case Mode::FilePicker:
      m_pFilter->SetShowNonImportableFiles(true);
      m_pFilter->SetShowFiles(true);
      break;
  }


  switch (index)
  {
    case 0:
    case 1:
    case 2:
      // all filters enabled
      // might be different for dialogs
      sFilter = m_sAllTypesFilter;
      break;

    default:
      sFilter.Set(";", TypeFilter->itemData(index, Qt::UserRole).toString().toUtf8().data(), ";");
      break;
  }

  m_pFilter->SetTypeFilter(sFilter);
}

void plQtAssetBrowserWidget::OnShowSubFolderItemsToggled()
{
  m_pFilter->SetShowItemsInSubFolders(!m_pFilter->GetShowItemsInSubFolders());
}

void plQtAssetBrowserWidget::OnShowHiddenFolderItemsToggled()
{
  m_pFilter->SetShowItemsInHiddenFolders(!m_pFilter->GetShowItemsInHiddenFolders());
}

void plQtAssetBrowserWidget::on_ListAssets_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  if (ListAssets->selectionModel()->hasSelection())
  {
    if (m_Mode == Mode::Browser)
    {
      QString sTitle = "Open Selection";
      QIcon icon = QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg"));
      if (ListAssets->selectionModel()->selectedIndexes().count() == 1)
      {
        const QModelIndex firstItem = ListAssets->selectionModel()->selectedIndexes()[0];
        const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)firstItem.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
        if (itemType.IsAnySet(plAssetBrowserItemFlags::Asset | plAssetBrowserItemFlags::SubAsset))
        {
          sTitle = "Open Document";
        }
        else if (itemType.IsSet(plAssetBrowserItemFlags::File))
        {
          sTitle = "Open File";
        }
        else if (itemType.IsAnySet(plAssetBrowserItemFlags::DataDirectory | plAssetBrowserItemFlags::Folder))
        {
          sTitle = "Enter Folder";
          icon = QIcon(QLatin1String(":/EditorFramework/Icons/Folder.svg"));
        }
      }
      m.setDefaultAction(m.addAction(icon, sTitle, this, SLOT(OnListOpenAssetDocument())));
    }
    else
      m.setDefaultAction(m.addAction(QLatin1String("Select"), this, SLOT(OnListOpenAssetDocument())));

    m.addAction(QIcon(QLatin1String(":/EditorFramework/Icons/AssetNeedsTransform.svg")), QLatin1String("Transform"), this, SLOT(OnTransform()));

    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnListOpenExplorer()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Guid.svg")), QLatin1String("Copy Asset Guid"), this, SLOT(OnListCopyAssetGuid()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct references to this asset"), this, [&]()
      { OnListFindAllReferences(false); });
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct and indirect references to this asset"), this, [&]()
      { OnListFindAllReferences(true); });
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/ZoomOut.svg")), QLatin1String("Filter to this Path"), this, SLOT(OnFilterToThisPath()));
  }

  auto pSortAction = m.addAction(QLatin1String("Sort by Recently Used"), this, SLOT(OnListToggleSortByRecentlyUsed()));
  pSortAction->setCheckable(true);
  pSortAction->setChecked(m_pFilter->GetSortByRecentUse());


  if (m_Mode == Mode::Browser && ListAssets->selectionModel()->hasSelection())
  {
    QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
    bool bImportable = false;
    bool bAllFiles = true;
    for (const QModelIndex& id : selection)
    {
      bImportable |= id.data(plQtAssetBrowserModel::UserRoles::Importable).toBool();

      const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)id.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      if (itemType.IsAnySet(plAssetBrowserItemFlags::SubAsset | plAssetBrowserItemFlags::DataDirectory))
      {
        bAllFiles = false;
      }
    }

    // Delete
    {
      QAction* pDelete = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), this, SLOT(DeleteSelection()));
      if (!bAllFiles)
      {
        pDelete->setEnabled(false);
        pDelete->setToolTip("Sub-assets and data directories can't be deleted.");
      }
    }

    // Import assets
    if (bImportable)
    {
      m.addSeparator();
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")), QLatin1String("Import..."), this, SLOT(ImportSelection()));
      QMenu* imp = m.addMenu(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")), "Import As");
      connect(imp, &QMenu::aboutToShow, this, &plQtAssetBrowserWidget::OnImportAsAboutToShow);
      AddImportedViaMenu(&m);
    }
  }

  m.addSeparator();
  AddAssetCreatorMenu(&m, true);

  m.exec(ListAssets->viewport()->mapToGlobal(pt));
}

void plQtAssetBrowserWidget::OnListOpenAssetDocument()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  QModelIndexList selection = ListAssets->selectionModel()->selectedRows();

  for (auto& index : selection)
  {
    // Only enter folders on a single selection. Otherwise the results are undefined.
    const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)index.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    if (selection.count() > 1 && itemType.IsAnySet(plAssetBrowserItemFlags::DataDirectory | plAssetBrowserItemFlags::Folder))
      continue;
    on_ListAssets_doubleClicked(index);
  }
}


void plQtAssetBrowserWidget::OnTransform()
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedRows();

  plProgressRange range("Transforming Assets", 1 + selection.length(), true);

  for (auto& index : selection)
  {
    if (range.WasCanceled())
      break;

    plUuid guid = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AssetGuid).value<plUuid>();
    QString sPath = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::RelativePath).toString();
    range.BeginNextStep(sPath.toUtf8());
    plTransformStatus res = plAssetCurator::GetSingleton()->TransformAsset(guid, plTransformFlags::TriggeredManually);
    if (res.Failed())
    {
      plLog::Error("{0} ({1})", res.m_sMessage, sPath.toUtf8().data());
    }
  }

  range.BeginNextStep("Writing Lookup Tables");

  plAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
}

void plQtAssetBrowserWidget::OnListOpenExplorer()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  plString sPath = m_pModel->data(ListAssets->currentIndex(), plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  plQtUiServices::OpenInExplorer(sPath, true);
}

void plQtAssetBrowserWidget::OnListCopyAssetGuid()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  plStringBuilder tmp;
  plUuid guid = m_pModel->data(ListAssets->currentIndex(), plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>();

  QClipboard* clipboard = QApplication::clipboard();
  QMimeData* mimeData = new QMimeData();
  mimeData->setText(plConversionUtils::ToString(guid, tmp).GetData());
  clipboard->setMimeData(mimeData);

  plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(plFmt("Copied asset GUID: {}", tmp), plTime::MakeFromSeconds(5));
}

void plQtAssetBrowserWidget::OnFilterToThisPath()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  plStringBuilder sPath = m_pModel->data(ListAssets->currentIndex(), plQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data();
  sPath.PathParentDirectory();
  sPath.Trim("/");

  m_pFilter->SetPathFilter(sPath);
}

void plQtAssetBrowserWidget::OnListFindAllReferences(bool transitive)
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  plUuid guid = m_pModel->data(ListAssets->currentIndex(), plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>();
  plStringBuilder sAssetGuid;
  plConversionUtils::ToString(guid, sAssetGuid);

  plStringBuilder sFilter;
  sFilter.SetFormat("{}:{}", transitive ? "ref-all" : "ref", sAssetGuid);
  m_pFilter->SetTextFilter(sFilter);
  m_pFilter->SetPathFilter("");
}

void plQtAssetBrowserWidget::OnSelectionTimer()
{
  if (m_pModel->rowCount() == 1)
  {
    auto index = m_pModel->index(0, 0);

    ListAssets->selectionModel()->select(index, QItemSelectionModel::SelectionFlag::ClearAndSelect);
  }
}


void plQtAssetBrowserWidget::OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    Q_EMIT ItemCleared();
  }
  else if (ListAssets->selectionModel()->selectedIndexes().size() == 1)
  {
    QModelIndex index = ListAssets->selectionModel()->selectedIndexes()[0];

    const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)index.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

    plUuid guid = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>();
    Q_EMIT ItemSelected(guid, m_pModel->data(index, plQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
}

void plQtAssetBrowserWidget::OnAssetSelectionCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    Q_EMIT ItemCleared();
  }
  else if (ListAssets->selectionModel()->selectedIndexes().size() == 1)
  {
    QModelIndex index = ListAssets->selectionModel()->selectedIndexes()[0];

    const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)index.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

    plUuid guid = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>();
    Q_EMIT ItemSelected(guid, m_pModel->data(index, plQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
}


void plQtAssetBrowserWidget::OnModelReset()
{
  Q_EMIT ItemCleared();
}


void plQtAssetBrowserWidget::NewAsset()
{
  QAction* pSender = qobject_cast<QAction*>(sender());

  plAssetDocumentManager* pManager = (plAssetDocumentManager*)pSender->property("AssetManager").value<void*>();
  plString sAssetType = pSender->property("AssetType").toString().toUtf8().data();
  plString sTranslateAssetType = plTranslate(sAssetType);
  plString sExtension = pSender->property("Extension").toString().toUtf8().data();
  bool useSelection = pSender->property("UseSelection").toBool();

  QString sStartDir = plToolsProject::GetSingleton()->GetProjectDirectory().GetData();

  // find path
  {
    if (TreeFolderFilter->currentItem())
    {
      sStartDir = TreeFolderFilter->currentItem()->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
    }

    // this will take precedence
    if (useSelection && ListAssets->selectionModel()->hasSelection())
    {
      plString sPath = m_pModel->data(ListAssets->currentIndex(), plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

      if (!sPath.IsEmpty())
      {
        plStringBuilder temp = sPath;
        sPath = temp.GetFileDirectory();

        sStartDir = sPath.GetData();
      }
    }
  }

  //
  plStringBuilder sNewAsset = qtToPlString(sStartDir);
  plStringBuilder sBaseFileName;
  plPathUtils::MakeValidFilename(sTranslateAssetType, ' ', sBaseFileName);
  sNewAsset.AppendFormat("/{}.{}", sBaseFileName, sExtension);

  for (plUInt32 i = 0; plOSFile::ExistsFile(sNewAsset); i++)
  {
    sNewAsset = qtToPlString(sStartDir);
    sNewAsset.AppendFormat("/{}{}.{}", sBaseFileName, i, sExtension);
  }

  sNewAsset.MakeCleanPath();

  plDocument* pDoc;
  if (pManager->CreateDocument(sAssetType, sNewAsset, pDoc, plDocumentFlags::Default).m_Result.Failed())
  {
    plLog::Error("Failed to create document: {}", sNewAsset);
    return;
  }

  {
    plStringBuilder sRelativePath = sNewAsset;
    if (plQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sRelativePath))
    {
      m_pFilter->SetTemporaryPinnedItem(sRelativePath);
    }
    plFileSystemModel::GetSingleton()->NotifyOfChange(sNewAsset);
    m_pModel->OnFileSystemUpdate();
  }

  plInt32 iNewIndex = m_pModel->FindIndex(sNewAsset);
  if (iNewIndex != -1)
  {
    m_bOpenAfterRename = true;
    QModelIndex idx = m_pModel->index(iNewIndex, 0);
    ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
    ListAssets->edit(idx);
  }
}


void plQtAssetBrowserWidget::OnFileEditingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset)
{
  plStringBuilder sOldPath = qtToPlString(sAbsPath);
  plStringBuilder sNewPath = sOldPath;
  if (bIsAsset)
    sNewPath.ChangeFileName(qtToPlString(sNewName));
  else
    sNewPath.ChangeFileNameAndExtension(qtToPlString(sNewName));

  if (sOldPath != sNewPath)
  {
    if (plOSFile::MoveFileOrDirectory(sOldPath, sNewPath).Failed())
    {
      plLog::Error("Failed to rename '{}' to '{}'", sOldPath, sNewPath);
      return;
    }

    plFileSystemModel::GetSingleton()->NotifyOfChange(sNewPath);
    plFileSystemModel::GetSingleton()->NotifyOfChange(sOldPath);

    // If we have a temporary item, make sure that any renames ensure that the item is still set as the new temporary
    // A common case is: a type filter is active that excludes a newly created asset. Thus, on creation the new asset is set as the pinned item. Editing of the item is started and the user gives it a new name and we end up here. We want the item to remain pinned.
    if (!m_pFilter->GetTemporaryPinnedItem().IsEmpty())
    {
      plStringBuilder sOldRelativePath = sOldPath;
      plStringBuilder sNewRelativePath = sNewPath;
      if (plQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sOldRelativePath) && plQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewRelativePath))
      {
        if (sOldRelativePath == m_pFilter->GetTemporaryPinnedItem())
        {
          m_pFilter->SetTemporaryPinnedItem(sNewRelativePath);
        }
      }
    }
    m_pModel->OnFileSystemUpdate();

    // it is necessary to flush the events queued on the main thread, otherwise opening the asset may not work as intended
    plAssetCurator::GetSingleton()->MainThreadTick(true);

    plInt32 iNewIndex = m_pModel->FindIndex(sNewPath);
    if (iNewIndex != -1)
    {
      QModelIndex idx = m_pModel->index(iNewIndex, 0);
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
    }
  }

  if (m_bOpenAfterRename)
  {
    m_bOpenAfterRename = false;

    plInt32 iNewIndex = m_pModel->FindIndex(sNewPath);
    if (iNewIndex != -1)
    {
      QModelIndex idx = m_pModel->index(iNewIndex, 0);
      on_ListAssets_doubleClicked(idx);
    }
  }
}

void plQtAssetBrowserWidget::ImportSelection()
{
  plHybridArray<plString, 4> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  if (filesToImport.IsEmpty())
    return;

  plAssetDocumentGenerator::ImportAssets(filesToImport);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    Q_EMIT m_pModel->dataChanged(id, id);
  }
}


void plQtAssetBrowserWidget::OnOpenImportReferenceAsset()
{
  QAction* pSender = qobject_cast<QAction*>(sender());
  plString sAbsPath = qtToPlString(pSender->property("AbsPath").toString());

  plQtEditorApp::GetSingleton()->OpenDocument(sAbsPath, plDocumentFlags::RequestWindow | plDocumentFlags::AddToRecentFilesList);
}

void plQtAssetBrowserWidget::OnListToggleSortByRecentlyUsed()
{
  m_pFilter->SetSortByRecentUse(!m_pFilter->GetSortByRecentUse());
}

void plQtAssetBrowserWidget::SetSelectedAsset(plUuid preselectedAsset)
{
  if (!preselectedAsset.IsValid())
    return;

  // cannot do this immediately, since the UI is probably still building up
  // ListAssets->scrollTo either hangs, or has no effect
  // so we put this into the message queue, and do it later
  QMetaObject::invokeMethod(this, "OnScrollToItem", Qt::ConnectionType::QueuedConnection, Q_ARG(plUuid, preselectedAsset));
}

void plQtAssetBrowserWidget::SetSelectedFile(plStringView sAbsPath)
{
  if (sAbsPath.IsEmpty())
    return;

  // cannot do this immediately, since the UI is probably still building up
  // ListAssets->scrollTo either hangs, or has no effect
  // so we put this into the message queue, and do it later
  QMetaObject::invokeMethod(this, "OnScrollToFile", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, plMakeQString(sAbsPath)));
}

void plQtAssetBrowserWidget::OnScrollToItem(plUuid preselectedAsset)
{
  for (plInt32 i = 0; i < m_pModel->rowCount(); ++i)
  {
    QModelIndex idx = m_pModel->index(i, 0);
    if (m_pModel->data(idx, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>() == preselectedAsset)
    {
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
      return;
    }
  }

  raise();
}

void plQtAssetBrowserWidget::OnScrollToFile(QString sPreselectedFile)
{
  for (plInt32 i = 0; i < m_pModel->rowCount(); ++i)
  {
    QModelIndex idx = m_pModel->index(i, 0);

    if (m_pModel->data(idx, plQtAssetBrowserModel::UserRoles::AbsolutePath).value<QString>() == sPreselectedFile)
    {
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
      return;
    }
  }

  raise();
}

void plQtAssetBrowserWidget::ShowOnlyTheseTypeFilters(plStringView sFilters)
{
  m_sAllTypesFilter.Clear();

  if (!sFilters.IsEmpty())
  {
    plStringBuilder sFilter;
    const plStringBuilder sAllFilters(";", sFilters, ";");

    m_sAllTypesFilter = sAllFilters;

    {
      plQtScopedBlockSignals block(TypeFilter);

      for (plInt32 i = TypeFilter->count(); i > 1; --i)
      {
        const plInt32 idx = i - 1;

        sFilter.Set(";", TypeFilter->itemData(idx, Qt::UserRole).toString().toUtf8().data(), ";");

        if (sAllFilters.FindSubString(sFilter) == nullptr)
        {
          TypeFilter->removeItem(idx);
        }
      }
    }
  }

  m_pFilter->SetTypeFilter(m_sAllTypesFilter);
}

void plQtAssetBrowserWidget::UseFileExtensionFilters(plStringView sFileExtensions)
{
  m_pFilter->SetFileExtensionFilters(sFileExtensions);
}
