#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>


plFileNameValidator::plFileNameValidator(QObject* pParent, plStringView sParentFolder, plStringView sCurrentName)
  : QValidator(pParent)
  , m_sParentFolder(sParentFolder)
  , m_sCurrentName(sCurrentName)
{
}

QValidator::State plFileNameValidator::validate(QString& ref_sInput, int& ref_iPos) const
{
  plStringBuilder sTemp = ref_sInput.toUtf8().constData();
  if (sTemp.IsEmpty())
    return QValidator::State::Intermediate;
  if (plPathUtils::ContainsInvalidFilenameChars(sTemp))
    return QValidator::State::Invalid;
  if (sTemp.StartsWith_NoCase(" ") || sTemp.EndsWith(" "))
    return QValidator::State::Intermediate;

  if (!m_sCurrentName.IsEmpty() && sTemp == m_sCurrentName)
    return QValidator::State::Acceptable;

  plStringBuilder sAbsPath = m_sParentFolder;
  sAbsPath.AppendPath(sTemp);
  if (plOSFile::ExistsDirectory(sAbsPath) || plOSFile::ExistsFile(sAbsPath))
    return QValidator::State::Intermediate;

  return QValidator::State::Acceptable;
}


plFolderNameDelegate::plFolderNameDelegate(QObject* pParent /*= nullptr*/)
  : QItemDelegate(pParent)
{
}

QWidget* plFolderNameDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  plStringBuilder sAbsPath = index.data(plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().constData();

  QLineEdit* editor = new QLineEdit(pParent);
  editor->setValidator(new plFileNameValidator(editor, sAbsPath.GetFileDirectory(), sAbsPath.GetFileNameAndExtension()));
  return editor;
}

void plFolderNameDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const
{
  QString sOldName = index.data(plQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
  QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(pEditor);
  emit editingFinished(sOldName, pLineEdit->text());
}



eqQtAssetBrowserFolderView::eqQtAssetBrowserFolderView(QWidget* pParent)
  : QTreeWidget(pParent)
{
  viewport()->setAcceptDrops(true);
  setAcceptDrops(true);

  setDropIndicatorShown(true);
  setDefaultDropAction(Qt::MoveAction);

  SetDialogMode(false);

  setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  auto pDelegate = new plFolderNameDelegate(this);
  PL_VERIFY(connect(pDelegate, &plFolderNameDelegate::editingFinished, this, &eqQtAssetBrowserFolderView::OnFolderEditingFinished, Qt::QueuedConnection), "signal/slot connection failed");
  setItemDelegate(pDelegate);

  plFileSystemModel::GetSingleton()->m_FolderChangedEvents.AddEventHandler(plMakeDelegate(&eqQtAssetBrowserFolderView::FileSystemModelFolderEventHandler, this));
  plToolsProject::s_Events.AddEventHandler(plMakeDelegate(&eqQtAssetBrowserFolderView::ProjectEventHandler, this));

  PL_VERIFY(connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(OnItemSelectionChanged())) != nullptr, "signal/slot connection failed");

  UpdateDirectoryTree();
}

eqQtAssetBrowserFolderView::~eqQtAssetBrowserFolderView()
{
  plToolsProject::s_Events.RemoveEventHandler(plMakeDelegate(&eqQtAssetBrowserFolderView::ProjectEventHandler, this));
  plFileSystemModel::GetSingleton()->m_FolderChangedEvents.RemoveEventHandler(plMakeDelegate(&eqQtAssetBrowserFolderView::FileSystemModelFolderEventHandler, this));
}


void eqQtAssetBrowserFolderView::SetFilter(plQtAssetBrowserFilter* pFilter)
{
  m_pFilter = pFilter;
  PL_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
}


void eqQtAssetBrowserFolderView::SetDialogMode(bool bDialogMode)
{
  m_bDialogMode = bDialogMode;

  if (m_bDialogMode)
  {
    setDragEnabled(false);
    setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
  }
  else
  {
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
  }
}

void eqQtAssetBrowserFolderView::NewFolder()
{
  QAction* pSender = qobject_cast<QAction*>(sender());

  if (!currentItem())
    return;

  plStringBuilder sPath = currentItem()->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  plStringBuilder sNewFolder = sPath;
  sNewFolder.AppendFormat("/NewFolder");

  for (plUInt32 i = 2; plOSFile::ExistsDirectory(sNewFolder); i++)
  {
    sNewFolder = sPath;
    sNewFolder.AppendFormat("/NewFolder{}", i);
  }

  if (plFileSystem::CreateDirectoryStructure(sNewFolder).Succeeded())
  {
    plFileSystemModel::GetSingleton()->NotifyOfChange(sNewFolder);
    OnFlushFileSystemEvents();

    if (plQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewFolder))
    {
      QTreeWidgetItem* pItem = FindDirectoryTreeItem(sNewFolder, topLevelItem(0), {});
      if (pItem)
      {
        m_bTreeSelectionChangeInProgress = true;
        scrollToItem(pItem);
        clearSelection();
        pItem->setSelected(true);
        setCurrentItem(pItem);
        m_bTreeSelectionChangeInProgress = false;
        OnItemSelectionChanged(); // make sure the path filter is set to the new folder
        editItem(pItem);
      }
    }
  }
}

void eqQtAssetBrowserFolderView::OnFolderEditingFinished(const QString& sAbsPath, const QString& sNewName)
{
  plStringBuilder sPath = sAbsPath.toUtf8().data();
  plStringBuilder sNewPath = sPath;
  sNewPath.ChangeFileNameAndExtension(sNewName.toUtf8().data());

  if (sPath != sNewPath)
  {
    if (plOSFile::MoveFileOrDirectory(sPath, sNewPath).Failed())
    {
      plLog::Error("Failed to rename '{}' to '{}'", sPath, sNewPath);
      return;
    }

    plFileSystemModel::GetSingleton()->NotifyOfChange(sNewPath);
    plFileSystemModel::GetSingleton()->NotifyOfChange(sPath);
    OnFlushFileSystemEvents();

    if (plQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewPath))
    {
      QTreeWidgetItem* pItem = FindDirectoryTreeItem(sNewPath, topLevelItem(0), {});
      if (pItem)
      {
        scrollToItem(pItem);
        clearSelection();
        pItem->setSelected(true);
        topLevelItem(0)->setSelected(false);
        setCurrentItem(pItem);
      }
    }
  }
}

void eqQtAssetBrowserFolderView::FileSystemModelFolderEventHandler(const plFolderChangedEvent& e)
{
  PL_LOCK(m_FolderStructureMutex);
  m_QueuedFolderEvents.PushBack(e);

  QTimer::singleShot(0, this, SLOT(OnFlushFileSystemEvents()));
}

void eqQtAssetBrowserFolderView::ProjectEventHandler(const plToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case plToolsProjectEvent::Type::ProjectClosed:
    {
      // remove project structure from asset browser
      ClearDirectoryTree();
    }
    break;
    default:
      break;
  }
}

void eqQtAssetBrowserFolderView::dragMoveEvent(QDragMoveEvent* e)
{
  QTreeWidget::dragMoveEvent(e);

  plHybridArray<plString, 1> files;
  plString sTarget;
  plStatus res = canDrop(e, files, sTarget);
  if (res.Failed())
  {
    plQtUiServices::ShowGlobalStatusBarMessage(res.m_sMessage.GetView());
    e->ignore();
  }
  else
  {
    plQtUiServices::ShowGlobalStatusBarMessage({});
  }
}

plStatus eqQtAssetBrowserFolderView::canDrop(QDropEvent* e, plDynamicArray<plString>& out_files, plString& out_sTargetFolder)
{
  if (!e->mimeData()->hasFormat("application/plEditor.files"))
  {
    return plStatus(PL_FAILURE);
  }

  DropIndicatorPosition dropIndicator = dropIndicatorPosition();
  if (dropIndicator != QAbstractItemView::OnItem)
  {
    return plStatus(PL_FAILURE);
  }

  auto action = e->dropAction();
  if (action != Qt::MoveAction)
  {
    return plStatus(PL_FAILURE);
  }

  out_files.Clear();
  QByteArray encodedData = e->mimeData()->data("application/plEditor.files");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  plHybridArray<QString, 1> files;
  stream >> files;

  QModelIndex dropIndex = indexAt(e->position().toPoint());
  if (dropIndex.isValid())
  {
    QString sAbsTarget = dropIndex.data(plQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    out_sTargetFolder = qtToPlString(sAbsTarget);

    for (const QString& sFile : files)
    {
      plString sFileToMove = qtToPlString(sFile);
      out_files.PushBack(sFileToMove);

      if (plPathUtils::IsSubPath(sFileToMove, out_sTargetFolder))
      {
        return plStatus(plFmt("Can't move '{}' into its own sub-folder '{}'", sFileToMove, out_sTargetFolder));
      }
    }
  }

  return plStatus(PL_SUCCESS);
}

void eqQtAssetBrowserFolderView::dropEvent(QDropEvent* e)
{
  plQtUiServices::ShowGlobalStatusBarMessage({});
  plHybridArray<plString, 1> files;
  plString sTargetFolder;
  // Always accept and call base class to end the drop operation as a no-op in the base class.
  e->accept();
  QTreeWidget::dropEvent(e);
  if (canDrop(e, files, sTargetFolder).Failed())
  {
    return;
  }


  QMessageBox::StandardButton choice = plQtUiServices::MessageBoxQuestion(plFmt("Do you want to move {} files / folders into '{}'?", files.GetCount(), sTargetFolder), QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Yes);
  if (choice == QMessageBox::StandardButton::Cancel)
    return;

  plStringBuilder sNewLocation;
  for (const plString& sFile : files)
  {
    sNewLocation = sTargetFolder;
    sNewLocation.AppendPath(plPathUtils::GetFileNameAndExtension(sFile));
    if (plOSFile::MoveFileOrDirectory(sFile, sNewLocation).Failed())
    {
      plLog::Error("Failed to move '{}' to '{}'", sFile, sNewLocation);
    }
    plFileSystemModel::GetSingleton()->NotifyOfChange(sNewLocation);
    plFileSystemModel::GetSingleton()->NotifyOfChange(sFile);
  }
  OnFlushFileSystemEvents();

  if (e->source() == this && files.GetCount() == 1)
  {
    if (plQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewLocation))
    {
      QTreeWidgetItem* pItem = FindDirectoryTreeItem(sNewLocation, topLevelItem(0), {});
      if (pItem)
      {
        m_bTreeSelectionChangeInProgress = true;
        scrollToItem(pItem);
        clearSelection();
        pItem->setSelected(true);
        setCurrentItem(pItem);
        m_bTreeSelectionChangeInProgress = false;
      }
    }
  }
}


QStringList eqQtAssetBrowserFolderView::mimeTypes() const
{
  QStringList types;
  types << "application/plEditor.files";
  return types;
}


Qt::DropActions eqQtAssetBrowserFolderView::supportedDropActions() const
{
  return Qt::DropAction::MoveAction | Qt::DropAction::CopyAction;
}

QMimeData* eqQtAssetBrowserFolderView::mimeData(const QList<QTreeWidgetItem*>& items) const
{
  plHybridArray<QString, 1> files;
  for (const QTreeWidgetItem* pItem : items)
  {
    QModelIndex id = indexFromItem(pItem);
    QString text = id.data(plQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    files.PushBack(text);
  }

  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << files;

  QMimeData* mimeData = new QMimeData();
  mimeData->setData("application/plEditor.files", encodedData);
  return mimeData;
}

void eqQtAssetBrowserFolderView::keyPressEvent(QKeyEvent* e)
{
  QTreeWidget::keyPressEvent(e);

  if (e->key() == Qt::Key_Delete && !m_bDialogMode)
  {
    e->accept();
    DeleteFolder();
    return;
  }
}

void eqQtAssetBrowserFolderView::DeleteFolder()
{
  if (QTreeWidgetItem* pCurrentItem = currentItem())
  {
    QModelIndex id = indexFromItem(pCurrentItem);
    QString sQtAbsPath = id.data(plQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    plString sAbsPath = qtToPlString(sQtAbsPath);
    QMessageBox::StandardButton choice = plQtUiServices::MessageBoxQuestion(plFmt("Do you want to delete the folder\n'{}'?", sAbsPath), QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Yes);
    if (choice == QMessageBox::StandardButton::Cancel)
      return;

    if (!QFile::moveToTrash(sQtAbsPath))
    {
      plLog::Error("Failed to delete folder '{}'", sAbsPath);
    }
    plFileSystemModel::GetSingleton()->NotifyOfChange(sAbsPath);
  }
}

void eqQtAssetBrowserFolderView::OnFlushFileSystemEvents()
{
  PL_LOCK(m_FolderStructureMutex);

  for (const auto& e : m_QueuedFolderEvents)
  {
    switch (e.m_Type)
    {
      case plFolderChangedEvent::Type::FolderAdded:
      {
        BuildDirectoryTree(e.m_Path, e.m_Path.GetDataDirParentRelativePath(), topLevelItem(0), "", false);
      }
      break;

      case plFolderChangedEvent::Type::FolderRemoved:
      {
        RemoveDirectoryTreeItem(e.m_Path.GetDataDirParentRelativePath(), topLevelItem(0), "");
      }
      break;

      case plFolderChangedEvent::Type::ModelReset:
        UpdateDirectoryTree();
        break;

      default:
        break;
    }
  }

  m_QueuedFolderEvents.Clear();
}

void eqQtAssetBrowserFolderView::mousePressEvent(QMouseEvent* e)
{
  QModelIndex inx = indexAt(e->pos());
  if (!inx.isValid())
    return;

  QTreeWidget::mousePressEvent(e);
}

void eqQtAssetBrowserFolderView::OnItemSelectionChanged()
{
  if (m_bTreeSelectionChangeInProgress)
    return;

  plStringBuilder sCurPath;

  if (!selectedItems().isEmpty())
  {
    sCurPath = selectedItems()[0]->data(0, plQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data();
  }

  m_pFilter->SetPathFilter(sCurPath);
}

void eqQtAssetBrowserFolderView::OnPathFilterChanged()
{
  const QString sPath = plMakeQString(m_pFilter->GetPathFilter());

  if (topLevelItemCount() == 1)
  {
    if (m_bTreeSelectionChangeInProgress)
      return;

    m_bTreeSelectionChangeInProgress = true;
    clearSelection();
    SelectPathFilter(topLevelItem(0), sPath);
    m_bTreeSelectionChangeInProgress = false;
  }
}


void eqQtAssetBrowserFolderView::TreeOpenExplorer()
{
  if (!currentItem())
    return;

  plStringBuilder sPath = currentItem()->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  plQtUiServices::OpenInExplorer(sPath, false);
}

bool eqQtAssetBrowserFolderView::SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath)
{
  if (pParent->data(0, plQtAssetBrowserModel::UserRoles::RelativePath).toString() == sPath)
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
void eqQtAssetBrowserFolderView::UpdateDirectoryTree()
{
  plQtScopedBlockSignals block(this);

  if (topLevelItemCount() == 0)
  {
    QTreeWidgetItem* pNewParent = new QTreeWidgetItem();
    pNewParent->setText(0, QLatin1String("<root>"));

    addTopLevelItem(pNewParent);

    pNewParent->setExpanded(true);

    selectionModel()->select(indexFromItem(pNewParent), QItemSelectionModel::SelectionFlag::ClearAndSelect);
  }

  auto Folders = plFileSystemModel::GetSingleton()->GetFolders();

  if (m_uiKnownAssetFolderCount == Folders->GetCount())
    return;

  m_uiKnownAssetFolderCount = Folders->GetCount();

  plStringBuilder tmp;

  for (const auto& sDir : *Folders)
  {
    BuildDirectoryTree(sDir.Key(), sDir.Key().GetDataDirParentRelativePath(), topLevelItem(0), "", false);
  }

  setSortingEnabled(true);
  sortItems(0, Qt::SortOrder::AscendingOrder);
}


void eqQtAssetBrowserFolderView::ClearDirectoryTree()
{
  clear();
  m_uiKnownAssetFolderCount = 0;
}

void eqQtAssetBrowserFolderView::BuildDirectoryTree(const plDataDirPath& path, plStringView sCurPath, QTreeWidgetItem* pParent, plStringView sCurPathToItem, bool bIsHidden)
{
  if (sCurPath.IsEmpty())
    return;

  const char* szNextSep = sCurPath.FindSubString("/");

  QTreeWidgetItem* pNewParent = nullptr;

  plString sFolderName;

  if (szNextSep == nullptr)
    sFolderName = sCurPath;
  else
    sFolderName = plStringView(sCurPath.GetStartPointer(), szNextSep);

  if (sFolderName.EndsWith_NoCase("_data"))
  {
    bIsHidden = true;
  }

  plStringBuilder sCurPath2 = sCurPathToItem;
  sCurPath2.AppendPath(sFolderName);

  const QString sQtFolderName = plMakeQString(sFolderName.GetView());

  if (sQtFolderName == "AssetCache")
    return;

  for (plInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (pParent->child(i)->text(0) == sQtFolderName)
    {
      // item already exists
      pNewParent = pParent->child(i);
      goto godown;
    }
  }

  { // #TODO_ASSET data for folder
    const bool bIsDataDir = sCurPathToItem.IsEmpty();
    pNewParent = new QTreeWidgetItem();
    pNewParent->setText(0, sQtFolderName);
    pNewParent->setData(0, plQtAssetBrowserModel::UserRoles::AbsolutePath, plMakeQString(path.GetAbsolutePath().GetView()));
    pNewParent->setData(0, plQtAssetBrowserModel::UserRoles::RelativePath, plMakeQString(path.GetDataDirParentRelativePath()));
    plBitflags<plAssetBrowserItemFlags> flags = bIsDataDir ? plAssetBrowserItemFlags::DataDirectory : plAssetBrowserItemFlags::Folder;
    pNewParent->setData(0, plQtAssetBrowserModel::UserRoles::ItemFlags, (int)flags.GetValue());
    pNewParent->setIcon(0, plQtUiServices::GetCachedIconResource(bIsDataDir ? ":/EditorFramework/Icons/DataDirectory.svg" : ":/EditorFramework/Icons/Folder.svg"));
    if (!bIsDataDir)
      pNewParent->setFlags(pNewParent->flags() | Qt::ItemFlag::ItemIsEditable | Qt::ItemFlag::ItemIsDragEnabled | Qt::ItemFlag::ItemIsDropEnabled);
    else
      pNewParent->setFlags(pNewParent->flags() | Qt::ItemFlag::ItemIsDropEnabled);

    if (bIsHidden)
    {
      pNewParent->setForeground(0, QColor::fromRgba(qRgb(110, 110, 120)));
    }

    pParent->addChild(pNewParent);
  }

godown:

  if (szNextSep == nullptr)
    return;

  BuildDirectoryTree(path, szNextSep + 1, pNewParent, sCurPath2, bIsHidden);
}

void eqQtAssetBrowserFolderView::RemoveDirectoryTreeItem(plStringView sCurPath, QTreeWidgetItem* pParent, plStringView sCurPathToItem)
{
  if (QTreeWidgetItem* pTreeItem = FindDirectoryTreeItem(sCurPath, pParent, sCurPathToItem))
  {
    delete pTreeItem;
  }
}


QTreeWidgetItem* eqQtAssetBrowserFolderView::FindDirectoryTreeItem(plStringView sCurPath, QTreeWidgetItem* pParent, plStringView sCurPathToItem)
{
  if (sCurPath.IsEmpty())
    return nullptr;

  const char* szNextSep = sCurPath.FindSubString("/");

  QTreeWidgetItem* pNewParent = nullptr;

  plString sFolderName;

  if (szNextSep == nullptr)
    sFolderName = sCurPath;
  else
    sFolderName = plStringView(sCurPath.GetStartPointer(), szNextSep);

  plStringBuilder sCurPath2 = sCurPathToItem;
  sCurPath2.AppendPath(sFolderName);

  const QString sQtFolderName = plMakeQString(sFolderName.GetView());

  for (plInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (pParent->child(i)->text(0) == sQtFolderName)
    {
      // item already exists
      pNewParent = pParent->child(i);
      goto godown;
    }
  }

  return nullptr;

godown:

  if (szNextSep == nullptr)
  {
    return pNewParent;
  }

  return FindDirectoryTreeItem(szNextSep + 1, pNewParent, sCurPath2);
}
