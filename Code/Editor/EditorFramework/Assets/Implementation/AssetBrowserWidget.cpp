#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserWidget.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include "AssetBrowserWidget.h"

plQtAssetBrowserWidget::plQtAssetBrowserWidget(QWidget* parent)
  : QWidget(parent)
{
  m_uiKnownAssetFolderCount = 0;
  m_bDialogMode = false;

  setupUi(this);

  ButtonListMode->setVisible(false);
  ButtonIconMode->setVisible(false);

  PlasmaEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<PlasmaEditorPreferencesUser>();

  ListTypeFilter->setVisible(true);
  TypeFilter->setVisible(false);

  m_pFilter = new plQtAssetBrowserFilter(this);
  m_pModel = new plQtAssetBrowserModel(this, m_pFilter);

  SearchWidget->setPlaceholderText("Search Assets");

  IconSizeSlider->setValue(50);

  ListAssets->setModel(m_pModel);
  ListAssets->SetIconScale(IconSizeSlider->value());
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  ListAssets->setDragEnabled(true);
  ListAssets->setAcceptDrops(true);
  ListAssets->setDropIndicatorShown(true);
  on_ButtonIconMode_clicked();

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  m_pStatusBar = new QStatusBar(this);
  m_pStatusBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
  m_pStatusBar->setSizeGripEnabled(false);

  m_pCuratorControl = new plQtCuratorControl(this, curatorPanel, splitter);
  m_pCuratorControl->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

  m_pStatusBar->addPermanentWidget(m_pCuratorControl, 1);

  verticalLayout_2->addWidget(m_pStatusBar);



  // Tool Bar
  {
    m_pToolbar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "AssetBrowserToolBar";
    context.m_pDocument = nullptr;
    m_pToolbar->SetActionContext(context);
    m_pToolbar->setObjectName("AssetBrowserToolBar");

    m_pToolbar->addSeparator();

    //Adding other actions related to the filtering here and not in the actionMap because they need access to the filter
    //and making the filters a singleton or part of curator wouldn't make much sense
    m_pToolbar->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Undo.svg")), QLatin1String("Back"), this, SLOT(onPreviousFolder()));
    m_pToolbar->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Redo.svg")), QLatin1String("Forward"), this, SLOT(onNextFolder()));

    ToolBarLayout->insertWidget(0, m_pToolbar);
  }

  TreeFolderFilter->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  PLASMA_VERIFY(connect(m_pFilter, SIGNAL(TextFilterChanged()), this, SLOT(OnTextFilterChanged())) != nullptr, "signal/slot connection failed");
  PLASMA_VERIFY(connect(m_pFilter, SIGNAL(TypeFilterChanged()), this, SLOT(OnTypeFilterChanged())) != nullptr, "signal/slot connection failed");
  PLASMA_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
  PLASMA_VERIFY(connect(m_pModel, SIGNAL(modelReset()), this, SLOT(OnModelReset())) != nullptr, "signal/slot connection failed");
  PLASMA_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(OnAssetSelectionChanged(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  PLASMA_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(OnAssetSelectionCurrentChanged(const QModelIndex&, const QModelIndex&))) != nullptr, "signal/slot connection failed");
  PLASMA_VERIFY(connect(ListAssets, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(on_ListAssets_doubleClicked(const QModelIndex&))) != nullptr, "signal/slot connection failed");
  PLASMA_VERIFY(connect(ListAssets, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(on_ListAssets_customContextMenuRequested(const QPoint&))) != nullptr, "signal/slot connection failed");


  connect(SearchWidget, &plQtSearchWidget::textChanged, this, &plQtAssetBrowserWidget::OnSearchWidgetTextChanged);

  UpdateAssetTypes();

  plAssetCurator::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plQtAssetBrowserWidget::AssetCuratorEventHandler, this));
  plToolsProject::s_Events.AddEventHandler(plMakeDelegate(&plQtAssetBrowserWidget::ProjectEventHandler, this));

  setAcceptDrops(true);
  filterView->SetFilter(m_pFilter);
  filterView->SetTree(TreeFolderFilter);
  filterView->Reset();
  ListTypeFilter->setIconSize(QSize(16, 16));
}

plQtAssetBrowserWidget::~plQtAssetBrowserWidget()
{
  plToolsProject::s_Events.RemoveEventHandler(plMakeDelegate(&plQtAssetBrowserWidget::ProjectEventHandler, this));
  plAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtAssetBrowserWidget::AssetCuratorEventHandler, this));

  ListAssets->setModel(nullptr);
}

void plQtAssetBrowserWidget::dragEnterEvent(QDragEnterEvent* event)
{
  if (!event->source())
    event->acceptProposedAction();
}

void plQtAssetBrowserWidget::dragMoveEvent(QDragMoveEvent* event)
{
  event->acceptProposedAction();
}

void plQtAssetBrowserWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
  event->accept();
}

void plQtAssetBrowserWidget::dropEvent(QDropEvent* event)
{
  const QMimeData* mime = event->mimeData();
  if (mime->hasUrls())
  {
    QList<QUrl> urlList = mime->urls();
    plHybridArray<plString, 16> assetsToImport;

    plString pDir = plToolsProject::GetSingleton()->GetProjectDirectory();

    bool overWriteAll = false;
    for (qsizetype i = 0, count = qMin(urlList.size(), qsizetype(32)); i < count; ++i)
    {
      QUrl url = urlList.at(i);
      QFileInfo fileinfo = QFileInfo(url.toLocalFile());

      if (fileinfo.exists())
      {
        //build source and destination paths info ===================================================================
        plStringBuilder srcPath = urlList.at(i).path().toUtf8().constData();
        srcPath.Shrink(1, 0); // remove a "/" at the beginning of the sourcepath that keeps it from opening

        plStringBuilder dstPath = plStringBuilder();
        dstPath.Append(pDir);
        dstPath.ReplaceFirst(plToolsProject::GetSingleton()->GetProjectName(false), "");
        dstPath.Append(m_pFilter->GetPathFilter());
        if (!dstPath.EndsWith("/"))
          dstPath.Append("/");
        dstPath.Append(fileinfo.fileName().toUtf8().constData());

        //Move the file/folder ==============================================
        if (fileinfo.isDir())
        {

          if (!overWriteAll && plOSFile::ExistsDirectory(dstPath))
          {
            QMessageBox msgBox;
            msgBox.setText("Overwrite folder?");
            msgBox.setInformativeText("The folder you are copying already exists, overwrite the existing one?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            int res = msgBox.exec();
            switch (res)
            {
              case QMessageBox::Yes:
                break;
              case QMessageBox::YesToAll:
                overWriteAll = true;
                break;
              case QMessageBox::Cancel:
              default:
                for (auto file : assetsToImport)
                {
                  if (plOSFile::DeleteFile(file) == PLASMA_FAILURE)
                  {
                    plLog::Error("failed to delete temp file {}", file);
                  }
                }
                return;
            }
          }

          if (plOSFile::CopyFolder(srcPath, dstPath) != PLASMA_SUCCESS)
          {
            plLog::Error("Failed to copy the folder in the project directory");
            return;
          }

          plDynamicArray<plFileStats> movedFiles = plDynamicArray<plFileStats>();
          plOSFile::GatherAllItemsInFolder(movedFiles, dstPath);
          for (int i = 0; i < movedFiles.GetCount(); i++)
          {
            movedFiles[i].GetFullPath(dstPath);
            assetsToImport.PushBack(dstPath);
          }
  }
        else if (fileinfo.isFile())
        {
          if (!overWriteAll && plOSFile::ExistsFile(dstPath))
          {
            QMessageBox msgBox;
            msgBox.setText("Overwrite file?");
            msgBox.setInformativeText("The file you are copying already exists, overwrite the existing one?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            int res = msgBox.exec();
            switch (res)
            {
              case QMessageBox::Yes:
                break;
              case QMessageBox::YesToAll:
                overWriteAll = true;
                break;
              case QMessageBox::Cancel:
              default:
                for (auto file : assetsToImport)
                {
                  if (plOSFile::DeleteFile(file) == PLASMA_FAILURE)
                  {
                    plLog::Error("failed to delete temp file {}", file);
                  }
                }
                return;
            }
          }
          if (plOSFile::CopyFile(srcPath, dstPath) != PLASMA_SUCCESS)
          {
            plLog::Error("Failed to copy the file in the project directory");
            return;
          }
          assetsToImport.PushBack(dstPath);
        }
      }
  else
      {
        plLog::Error("Couldn't find file at {} for copying", urlList.at(i).path().toUtf8().constData());
        return;
      }
    }
    if (!plAssetDocumentGenerator::ImportAssets(assetsToImport)) {
      for (auto file : assetsToImport)
      {
        if (plOSFile::DeleteFile(file) == PLASMA_FAILURE)
        {
          plLog::Error("failed to delete temp file {}", file);
        }
      }
    }
  }
  else
    plLog::Dev("Ignoring unhandled MIME data received");


  event->acceptProposedAction();
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
    plQtScopedBlockSignals block(ListTypeFilter);

    ListTypeFilter->clear();

    // 'All' Filter
    {
      QListWidgetItem* pItem = new QListWidgetItem(QIcon(QLatin1String(":/AssetIcons/All.svg")), QLatin1String("<All>"));
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
      pItem->setCheckState(Qt::CheckState::Checked);
      pItem->setData(Qt::UserRole, QLatin1String("<All>"));

      ListTypeFilter->addItem(pItem);
    }

    for (const auto& it : assetTypes)
    {
      QListWidgetItem* pItem = new QListWidgetItem(plQtUiServices::GetCachedIconResource(it.Value()->m_sIcon, plColorScheme::GetGroupColor(it.Value()->m_IconColorGroup, 2)), QString::fromUtf8(it.Key(), it.Key().GetElementCount()));
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
      pItem->setCheckState(Qt::CheckState::Unchecked);
      pItem->setData(Qt::UserRole, QLatin1String(it.Value()->m_sDocumentTypeName));

      ListTypeFilter->addItem(pItem);
    }
  }

  {
    plQtScopedBlockSignals block(TypeFilter);

    TypeFilter->clear();

    // 'All' Filter
    TypeFilter->addItem(QIcon(QLatin1String(":/AssetIcons/All.svg")), QLatin1String("<All>"));

    for (const auto& it : assetTypes)
    {
      TypeFilter->addItem(plQtUiServices::GetCachedIconResource(it.Value()->m_sIcon, plColorScheme::GetGroupColor(it.Value()->m_IconColorGroup, 2)), QString::fromUtf8(it.Key(), it.Key().GetElementCount()));
      TypeFilter->setItemData(TypeFilter->count() - 1, QString::fromUtf8(it.Value()->m_sDocumentTypeName, it.Value()->m_sDocumentTypeName.GetElementCount()), Qt::UserRole);
    }
  }

  UpdateDirectoryTree();

  // make sure to apply the previously active type filter settings to the UI
  OnTypeFilterChanged();
}

void plQtAssetBrowserWidget::SetDialogMode()
{
  m_pToolbar->hide();
  m_bDialogMode = true;

  ListAssets->SetDialogMode(true);
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
      // remove project structure from asset browser
      ClearDirectoryTree();

      m_pFilter->Reset();
    }
    break;
    default:
      break;
  }
}


void plQtAssetBrowserWidget::AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset)
{
  if (m_bDialogMode)
    return;

  const plHybridArray<plDocumentManager*, 16>& managers = plDocumentManager::GetAllDocumentManagers();

  plDynamicArray<const plDocumentTypeDescriptor*> documentTypes;

  QMenu* pSubMenu = pMenu->addMenu("New asset");

  plStringBuilder sTypeFilter = m_pFilter->GetTypeFilter();

  for (plDocumentManager* pMan : managers)
  {
    if (!pMan->GetDynamicRTTI()->IsDerivedFrom<plAssetDocumentManager>())
      continue;

    pMan->GetSupportedDocumentTypes(documentTypes);
  }

  documentTypes.Sort([](const plDocumentTypeDescriptor* a, const plDocumentTypeDescriptor* b) -> bool { return plStringUtils::Compare(plTranslate(a->m_sDocumentTypeName), plTranslate(b->m_sDocumentTypeName)) < 0; });

  for (const plDocumentTypeDescriptor* desc : documentTypes)
  {
    if (!desc->m_bCanCreate || desc->m_sFileExtension.IsEmpty())
      continue;

    QAction* pAction = pSubMenu->addAction(plTranslate(desc->m_sDocumentTypeName));
    pAction->setIcon(plQtUiServices::GetSingleton()->GetCachedIconResource(desc->m_sIcon, plColorScheme::GetGroupColor(desc->m_IconColorGroup, 2)));
    pAction->setProperty("AssetType", desc->m_sDocumentTypeName.GetData());
    pAction->setProperty("AssetManager", QVariant::fromValue<void*>(desc->m_pManager));
    pAction->setProperty("Extension", desc->m_sFileExtension.GetData());
    pAction->setProperty("UseSelection", useSelectedAsset);

    connect(pAction, &QAction::triggered, this, &plQtAssetBrowserWidget::OnNewAsset);
  }
}

QString plQtAssetBrowserWidget::getSelectedPath()
{
  QString sStartDir = plToolsProject::GetSingleton()->GetProjectDirectory().GetData();

  // find path
  {
    plString pathfilter = m_pFilter->GetPathFilter();
    if (!pathfilter.IsEmpty())
    {
      plStringBuilder path = m_pFilter->GetPathFilter();

      if (path.IsEmpty())
      {
        plLog::Error("Can't create asset on root");
        return QString();
      }

      plMap<plString, plFileStatus, plCompareString_NoCase> folders = plAssetCurator::GetSingleton()->GetAllAssetFolders();
      plStringBuilder result = plStringBuilder();
      auto it = folders.GetIterator();
      if (!it.IsValid())
      {
        return QString();
      }

      for (; it != folders.GetLastIterator(); it.Next())
      {
        if (it.Key().EndsWith_NoCase(path))
        {
          if (result.IsEmpty())
          {
            result = it.Key();
          }
          else if (result.GetCharacterCount() > it.Key().GetCharacterCount())
          {
            result = it.Key();
          }
        }
      }
      if (result)
        sStartDir = QString(result.GetData());
    }

    // this will take precedence
    if (ListAssets->selectionModel()->hasSelection())
    {
      plString sPath = m_pModel->data(ListAssets->currentIndex(), plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

      if (!sPath.IsEmpty() && plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      {
        plStringBuilder temp = sPath;
        if (plOSFile::ExistsFile(temp)) // if we clicked create asset on a folder, we want to create it INSIDE this folder
        {
          sPath = temp.GetFileDirectory();
        }

        sStartDir = sPath.GetData();
      }
    }
  }

  return sStartDir;
}


void plQtAssetBrowserWidget::on_ListAssets_clicked(const QModelIndex& index)
{
  Q_EMIT ItemSelected(m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::ParentRelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
}

void plQtAssetBrowserWidget::onPreviousFolder()
{
  m_pFilter->OnPrevious();
}

void plQtAssetBrowserWidget::onNextFolder()
{
  m_pFilter->OnNext();
}

void plQtAssetBrowserWidget::on_ListAssets_activated(const QModelIndex& index)
{
  Q_EMIT ItemSelected(m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::ParentRelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
}

void plQtAssetBrowserWidget::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  plUuid guid = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>();
  bool isDir = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::Type).value<bool>();

  if (guid.IsValid())
  {
    plAssetCurator::GetSingleton()->UpdateAssetLastAccessTime(guid);
  }
  if (isDir)
  {
    if (!ListAssets->selectionModel()->hasSelection())
      return;

    plStringBuilder sPath = m_pModel->data(ListAssets->currentIndex(), plQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data();

    m_pFilter->SetPathFilter(sPath);
  }
  else
  {
    Q_EMIT ItemChosen(guid, m_pModel->data(index, plQtAssetBrowserModel::UserRoles::ParentRelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
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
  SearchWidget->setText(QString::fromUtf8(m_pFilter->GetTextFilter()));

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void plQtAssetBrowserWidget::OnTypeFilterChanged()
{
  plStringBuilder sTemp;
  const plStringBuilder sFilter(";", m_pFilter->GetTypeFilter(), ";");


  {
    plQtScopedBlockSignals _(ListTypeFilter);

    bool bNoneChecked = true;

    for (plInt32 i = 1; i < ListTypeFilter->count(); ++i)
    {
      sTemp.Set(";", ListTypeFilter->item(i)->data(Qt::UserRole).toString().toUtf8().data(), ";");

      const bool bChecked = sFilter.FindSubString(sTemp) != nullptr;

      ListTypeFilter->item(i)->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);

      if (bChecked)
        bNoneChecked = false;
    }

    ListTypeFilter->item(0)->setCheckState(bNoneChecked ? Qt::Checked : Qt::Unchecked);
  }

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

    if (iNumChecked == 1)
      TypeFilter->setCurrentIndex(iCheckedFilter);
    else
      TypeFilter->setCurrentIndex(0);
  }

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}


void plQtAssetBrowserWidget::OnSearchWidgetTextChanged(const QString& text)
{
  m_pFilter->SetTextFilter(text.toUtf8().data());
}

void plQtAssetBrowserWidget::on_ListTypeFilter_itemChanged(QListWidgetItem* item)
{
  plQtScopedBlockSignals block(ListTypeFilter);

  if (item->data(Qt::UserRole).toString() == "<All>")
  {
    if (item->checkState() == Qt::Checked)
    {
      // deactivate all others
      for (plInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        ListTypeFilter->item(i)->setCheckState(Qt::Unchecked);
      }
    }
    else
    {
      plStringBuilder sFilter;

      // activate all others
      for (plInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        if (!m_sAllTypesFilter.IsEmpty())
        {
          sFilter.Set(";", ListTypeFilter->item(i)->data(Qt::UserRole).toString().toUtf8().data(), ";");

          if (m_sAllTypesFilter.FindSubString(sFilter) != nullptr)
            ListTypeFilter->item(i)->setCheckState(Qt::Checked);
          else
            ListTypeFilter->item(i)->setCheckState(Qt::Unchecked);
        }
        else
          ListTypeFilter->item(i)->setCheckState(Qt::Checked);
      }
    }
  }
  else
  {
    if (item->checkState() == Qt::Checked)
    {
      // deactivate the 'all' button
      ListTypeFilter->item(0)->setCheckState(Qt::Unchecked);
    }
    else
    {
      bool bAnyChecked = false;

      for (plInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        if (ListTypeFilter->item(i)->checkState() == Qt::Checked)
          bAnyChecked = true;
      }

      // activate the 'All' item
      if (!bAnyChecked)
        ListTypeFilter->item(0)->setCheckState(Qt::Checked);
    }
  }

  plStringBuilder sFilter;

  for (plInt32 i = 1; i < ListTypeFilter->count(); ++i)
  {
    if (ListTypeFilter->item(i)->checkState() == Qt::Checked)
      sFilter.Append(";", ListTypeFilter->item(i)->data(Qt::UserRole).toString().toUtf8().data(), ";");
  }

  if (sFilter.IsEmpty())         // all filters enabled
    sFilter = m_sAllTypesFilter; // might be different for dialogs

  m_pFilter->SetTypeFilter(sFilter);
}

void plQtAssetBrowserWidget::AssetCuratorEventHandler(const plAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case plAssetCuratorEvent::Type::AssetListReset:
      UpdateAssetTypes();
      filterView->Reset();
      UpdateDirectoryTree();
      break;
    case plAssetCuratorEvent::Type::AssetAdded:
    case plAssetCuratorEvent::Type::AssetRemoved:
      UpdateDirectoryTree();
      break;
    default:
      break;
  }
}

void plQtAssetBrowserWidget::UpdateDirectoryTree()
{
  plQtScopedBlockSignals block(TreeFolderFilter);

  if (TreeFolderFilter->topLevelItemCount() == 0)
  {
    QTreeWidgetItem* pNewParent = new QTreeWidgetItem();
    pNewParent->setText(0, QLatin1String("<root>"));

    TreeFolderFilter->addTopLevelItem(pNewParent);

    pNewParent->setExpanded(true);
  }

  plMap<plString, plFileStatus, plCompareString_NoCase> Folders = plAssetCurator::GetSingleton()->GetAllAssetFolders();

  plDynamicArray<plString>& removed = plAssetCurator::GetSingleton()->GetRemovedFolders();
  for (plString path : removed) {
    plDynamicArray<plStringView> compTypes;
    path.Split(false, compTypes, "/");

    QTreeWidgetItem* parNode = TreeFolderFilter->topLevelItem(0);

    for (int i = 0; i < compTypes.GetCount(); i++)
    {
      plStringBuilder str;
      compTypes[i].GetData(str);

      if (parNode->childCount() > 0)
      {
        for (int j = 0; j < parNode->childCount(); j++)
        {
          QTreeWidgetItem* node = parNode->child(j);
          if (node->text(0) == str)
          {
            if (i == compTypes.GetCount() - 1)
            {
              // found the node to remove
              TreeFolderFilter->removeItemWidget(node, 0);
              delete node;
              break;
            }
            parNode = node;
          }
        }
      }
    }
  }

  removed.Clear();

  plStringBuilder tmp;

  for (const auto& sDir : Folders)
  {
    tmp = sDir.Key();
    plString abspath = tmp;

    if (!plQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(tmp))
    {
      continue;
    }


    BuildDirectoryTree(tmp, TreeFolderFilter->topLevelItem(0), "", false, abspath.GetData());
  }

  m_uiKnownAssetFolderCount = Folders.GetCount();
  TreeFolderFilter->setSortingEnabled(true);
  TreeFolderFilter->sortItems(0, Qt::SortOrder::AscendingOrder);
}


void plQtAssetBrowserWidget::ClearDirectoryTree()
{
  TreeFolderFilter->clear();
  m_uiKnownAssetFolderCount = 0;
}

void plQtAssetBrowserWidget::BuildDirectoryTree(const char* szCurPath, QTreeWidgetItem* pParent, const char* szCurPathToItem, bool bIsHidden, const char* szAbsPath)
{
  if (plStringUtils::IsNullOrEmpty(szCurPath))
    return;

  const char* szNextSep = plStringUtils::FindSubString(szCurPath, "/");

  QTreeWidgetItem* pNewParent = nullptr;

  plString sFolderName;

  if (szNextSep == nullptr)
    sFolderName = szCurPath;
  else
    sFolderName = plStringView(szCurPath, szNextSep);

  if (sFolderName.EndsWith_NoCase("_data"))
  {
    bIsHidden = true;
  }

  plStringBuilder sCurPath = szCurPathToItem;
  sCurPath.AppendPath(sFolderName);

  const QString sQtFolderName = QString::fromUtf8(sFolderName.GetData());

  for (plInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (pParent->child(i)->text(0) == sQtFolderName)
    {
      // item already exists
      pNewParent = pParent->child(i);
      goto godown;
    }
  }

  pNewParent = new QTreeWidgetItem();
  pNewParent->setText(0, sQtFolderName);
  pNewParent->setData(0, plQtAssetBrowserModel::UserRoles::RelativePath, QString::fromUtf8(sCurPath.GetData()));
  pNewParent->setData(0, plQtAssetBrowserModel::UserRoles::AbsolutePath, QString::fromUtf8(szAbsPath));

  if (bIsHidden)
  {
    pNewParent->setForeground(0, QColor::fromRgba(qRgb(110, 110, 120)));
  }

  pParent->addChild(pNewParent);

godown:

  if (szNextSep == nullptr)
    return;

  BuildDirectoryTree(szNextSep + 1, pNewParent, sCurPath, bIsHidden, szAbsPath);
}

void plQtAssetBrowserWidget::on_TreeFolderFilter_itemSelectionChanged()
{
  if (m_bTreeSelectionChangeInProgress)
    return;

  plStringBuilder sCurPath;

  if (!TreeFolderFilter->selectedItems().isEmpty())
  {
    sCurPath = TreeFolderFilter->selectedItems()[0]->data(0, plQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data();
  }

  m_pFilter->SetPathFilter(sCurPath);
}

void plQtAssetBrowserWidget::on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  if (TreeFolderFilter->currentItem())
  {
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnTreeOpenExplorer()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), this, SLOT(OnDelete()));
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
  }

  m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/DataDirAdd.svg")), QLatin1String("Add folder"), this, SLOT(OnAddFolder()));

  AddAssetCreatorMenu(&m, false);

  m.exec(TreeFolderFilter->viewport()->mapToGlobal(pt));
}

void plQtAssetBrowserWidget::on_TypeFilter_currentIndexChanged(int index)
{
  plQtScopedBlockSignals block(TypeFilter);

  plStringBuilder sFilter;

  if (index > 0)
  {
    sFilter.Set(";", TypeFilter->itemData(index, Qt::UserRole).toString().toUtf8().data(), ";");
  }
  else
  {
    // all filters enabled
    // might be different for dialogs
    sFilter = m_sAllTypesFilter;
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

void plQtAssetBrowserWidget::OnTreeOpenExplorer()
{
  if (!TreeFolderFilter->currentItem())
    return;

  plStringBuilder sPath = TreeFolderFilter->currentItem()->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  if (!plQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(sPath, true))
    return;

  plQtUiServices::OpenInExplorer(sPath, false);
}

void plQtAssetBrowserWidget::on_ListAssets_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  if (ListAssets->selectionModel()->hasSelection())
  {
    if (!m_bDialogMode)
      m.setDefaultAction(m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open Document"), this, SLOT(OnListOpenAssetDocument())));
    else
      m.setDefaultAction(m.addAction(QLatin1String("Select"), this, SLOT(OnListOpenAssetDocument())));

    m.addAction(QIcon(QLatin1String(":/EditorFramework/Icons/AssetNeedsTransform.svg")), QLatin1String("Transform"), this, SLOT(OnTransform()));

    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnListOpenExplorer()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Guid.svg")), QLatin1String("Copy Asset Guid"), this, SLOT(OnListCopyAssetGuid()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct references to this asset"), this, [&]() { OnListFindAllReferences(false); });
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct and indirect references to this asset"), this, [&]() { OnListFindAllReferences(true); });
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/ZoomOut.svg")), QLatin1String("Filter to this Path"), this, SLOT(OnFilterToThisPath()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), this, SLOT(OnDelete()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Copy.svg")), QLatin1String("Duplicate"), this, SLOT(OnDuplicate()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Rename.svg")), QLatin1String("Rename"), this, SLOT(OnRename()));

  }

  auto pSortAction = m.addAction(QLatin1String("Sort by Recently Used"), this, SLOT(OnListToggleSortByRecentlyUsed()));
  pSortAction->setCheckable(true);
  pSortAction->setChecked(m_pFilter->GetSortByRecentUse());

  m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/DataDirAdd.svg")), QLatin1String("Add folder"), this, SLOT(OnAddFolder()));




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
    plUuid guid = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>();

    if (guid.IsValid())
    {
      plAssetCurator::GetSingleton()->UpdateAssetLastAccessTime(guid);
    }

    Q_EMIT ItemChosen(guid, m_pModel->data(index, plQtAssetBrowserModel::UserRoles::ParentRelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
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
    QString sPath = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::ParentRelativePath).toString();
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

  plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(plFmt("Copied asset GUID: {}", tmp), plTime::Seconds(5));
}

void plQtAssetBrowserWidget::OnFilterToThisPath()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  plStringBuilder sPath = m_pModel->data(ListAssets->currentIndex(), plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  sPath.PathParentDirectory();
  sPath.Trim("/");

  if (!plQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath))
    return;

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
  sFilter.Format("{}:{}", transitive ? "ref-all" : "ref", sAssetGuid);
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

    plUuid guid = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>();
    Q_EMIT ItemSelected(guid, m_pModel->data(index, plQtAssetBrowserModel::UserRoles::ParentRelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
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

    plUuid guid = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>();
    Q_EMIT ItemSelected(guid, m_pModel->data(index, plQtAssetBrowserModel::UserRoles::ParentRelativePath).toString(), m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  }
}


void plQtAssetBrowserWidget::OnModelReset()
{
  Q_EMIT ItemCleared();
}


void plQtAssetBrowserWidget::OnNewAsset()
{
  QAction* pSender = qobject_cast<QAction*>(sender());

  plAssetDocumentManager* pManager = (plAssetDocumentManager*)pSender->property("AssetManager").value<void*>();
  plString sAssetType = pSender->property("AssetType").toString().toUtf8().data();
  plString sTranslateAssetType = plTranslate(sAssetType);
  plString sExtension = pSender->property("Extension").toString().toUtf8().data();
  bool useSelection = pSender->property("UseSelection").toBool();

  QString sStartDir = getSelectedPath();

  plStringBuilder title("Create ", sTranslateAssetType), sFilter;

  sFilter.Format("{0} (*.{1})", sTranslateAssetType, sExtension);

  QString sSelectedFilter = sFilter.GetData();
  plStringBuilder sOutput = QFileDialog::getSaveFileName(QApplication::activeWindow(), title.GetData(), sStartDir, sFilter.GetData(), &sSelectedFilter, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();

  if (sOutput.IsEmpty())
    return;

  plDocument* pDoc;
  if (pManager->CreateDocument(sAssetType, sOutput, pDoc, plDocumentFlags::RequestWindow | plDocumentFlags::AddToRecentFilesList).m_Result.Succeeded())
  {
    pDoc->EnsureVisible();
  }
}

void plQtAssetBrowserWidget::OnAddFolder()
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("Add folder"),
    tr("Folder's name:"), QLineEdit::Normal,
    QString(), &ok);

  if (!ok || text.isEmpty())
  {
    return;
  }

  plStringBuilder result = getSelectedPath().toUtf8().data();
  result.AppendFormat("/{}/", text.toUtf8());

  if (plOSFile::CreateDirectoryStructure(result).Failed())
  {
    plLog::Error("Couldn't create folder {}", result);
    return;
  }

  plAssetCurator::GetSingleton()->CheckFileSystem();
  UpdateDirectoryTree();
}

void plQtAssetBrowserWidget::OnDelete()
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    return;
  }
  for (QModelIndex index : ListAssets->selectionModel()->selectedIndexes())
  {
    plString path = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).value<QString>().toUtf8().data();
    if (plOSFile::ExistsFile(path))
    {
      // TODO : remove linked files?  Might need the assets rewrite before doing that
      if (plOSFile::DeleteFile(path).Failed())
      {
        plLog::Error("failed to delete file {}", path);
      }
    }
    else if (plOSFile::ExistsDirectory(path))
    {
      if (plOSFile::DeleteFolder(path).Failed())
      {
        plLog::Error("failed to delete folder {}", path);
      }
    }
    else
    {
      plLog::Error("File/folder {} doesn't exists", path);
    }
  }
}

void plQtAssetBrowserWidget::OnDuplicate()
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    return;
  }
  for (QModelIndex index : ListAssets->selectionModel()->selectedIndexes())
  {
    plString path = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).value<QString>().toUtf8().data();
    plStringBuilder fileName = path.GetFileName();
    fileName.Append("_copy");
    plStringBuilder destPath = path;
    destPath.ChangeFileName(fileName);
    while (plOSFile::ExistsFile(destPath))
    {
      fileName.Append("_copy");
      destPath.ChangeFileName(fileName);
    }
    // find document manager for what we want to clone and use clonedocument on it
    if (plOSFile::ExistsFile(path))
    {
      const plDocumentTypeDescriptor* pTypeDesc = nullptr;
      if (plDocumentManager::FindDocumentTypeFromPath(path, false, pTypeDesc).Failed())
      {
        PLASMA_REPORT_FAILURE("Invalid asset setup");
      }
      plUuid newUid = plUuid();
      newUid.CreateNewUuid();
      if (pTypeDesc->m_pManager->CloneDocument(path, destPath, newUid).Failed())//TODO: after duplicating an asset, renaming the original will make it appear twice in the browser
      {
        plLog::Error("failed to clone asset {}", path);
      }
    }
    if (plOSFile::ExistsDirectory(path))
    {
      plLog::Error("Cannot duplicate folders");
    }
    else
    {
      plLog::Error("File/folder {} doesn't exists", path);
    }
  }
}

void plQtAssetBrowserWidget::OnRename()
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    return;
  }
  if (ListAssets->selectionModel()->selectedIndexes().size() > 1)
  {
    plLog::Error("Cannot rename multiple items at a time");
    return;
  }

  bool ok;
  QString text = QInputDialog::getText(this, tr("Add folder"),
    tr("Folder's name:"), QLineEdit::Normal,
    QString(), &ok);

  if (!ok || text.isEmpty())
  {
    return;
  }

  QModelIndex index = ListAssets->selectionModel()->selectedIndexes()[0];
  plString path = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).value<QString>().toUtf8().data();
  plStringBuilder destPath = path;
  destPath.ChangeFileName(text.toUtf8().data());
  if (plOSFile::ExistsFile(path))
  {
    if (plOSFile::ExistsFile(destPath))
    {
      plLog::Error("File {} already exists", destPath);
      return;
    }
    if (plOSFile::CopyFile(path, destPath).Failed())
    {
      plLog::Error("Failed to copy file {} during renaming", path);
      return;
    }
    if (plOSFile::DeleteFile(path).Failed())
    {
      plLog::Error("Failed to delete file {} during renaming", path);
      return;
    }
  }
  else if (plOSFile::ExistsDirectory(path))
  {
    plLog::Error("Cannot rename folders yet");
  }
  else
  {
    plLog::Error("Cannot find file/folder {} for renaming", path);
  }

}


void plQtAssetBrowserWidget::OnListToggleSortByRecentlyUsed()
{
  m_pFilter->SetSortByRecentUse(!m_pFilter->GetSortByRecentUse());
}

void plQtAssetBrowserWidget::OnPathFilterChanged()
{
  const QString sPath = QString::fromUtf8(m_pFilter->GetPathFilter());

  if (TreeFolderFilter->topLevelItemCount() == 1)
  {
    if (m_bTreeSelectionChangeInProgress)
      return;

    m_bTreeSelectionChangeInProgress = true;
    TreeFolderFilter->clearSelection();
    SelectPathFilter(TreeFolderFilter->topLevelItem(0), sPath);
    m_bTreeSelectionChangeInProgress = false;
  }

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

bool plQtAssetBrowserWidget::SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath)
{
  if (pParent->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString() == sPath)
  {
    pParent->setSelected(true);
    return true;
  }

  for (plInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (SelectPathFilter(pParent->child(i), sPath))
    {
      pParent->setExpanded(true);
      return true;
    }
  }

  return false;
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

void plQtAssetBrowserWidget::OnScrollToItem(plUuid preselectedAsset)
{
  for (plInt32 i = 0; i < m_pModel->rowCount(); ++i)
  {
    if (m_pModel->data(m_pModel->index(i, 0), plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>() == preselectedAsset)
    {
      ListAssets->selectionModel()->select(m_pModel->index(i, 0), QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(m_pModel->index(i, 0), QAbstractItemView::ScrollHint::EnsureVisible);
      return;
    }
  }

  raise();
}

void plQtAssetBrowserWidget::ShowOnlyTheseTypeFilters(const char* szFilters)
{
  m_sAllTypesFilter.Clear();

  if (!plStringUtils::IsNullOrEmpty(szFilters))
  {
    plStringBuilder sFilter;
    const plStringBuilder sAllFilters(";", szFilters, ";");

    m_sAllTypesFilter = sAllFilters;

    {
      plQtScopedBlockSignals block(ListTypeFilter);

      for (plInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        sFilter.Set(";", ListTypeFilter->item(i)->data(Qt::UserRole).toString().toUtf8().data(), ";");

        if (sAllFilters.FindSubString(sFilter) == nullptr)
        {
          ListTypeFilter->item(i)->setHidden(true);
        }
      }
    }

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
