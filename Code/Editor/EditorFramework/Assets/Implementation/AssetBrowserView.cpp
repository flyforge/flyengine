#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetBrowserView.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>


plQtAssetBrowserView::plQtAssetBrowserView(QWidget* parent)
  : plQtItemView<QListView>(parent)
{
  m_iIconSizePercentage = 100;
  m_pDelegate = new plQtIconViewDelegate(this);

  SetDialogMode(false);

  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  setViewMode(QListView::ViewMode::IconMode);
  setUniformItemSizes(true);
  setResizeMode(QListView::ResizeMode::Adjust);

  setItemDelegate(m_pDelegate);
  SetIconScale(m_iIconSizePercentage);
}

void plQtAssetBrowserView::SetDialogMode(bool bDialogMode)
{
  m_bDialogMode = bDialogMode;

  if (m_bDialogMode)
  {
    m_pDelegate->SetDrawTransformState(false);
    setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  }
  else
  {
    m_pDelegate->SetDrawTransformState(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  }
}

void plQtAssetBrowserView::SetIconMode(bool bIconMode)
{
  if (bIconMode)
  {
    setViewMode(QListView::ViewMode::IconMode);
    SetIconScale(m_iIconSizePercentage);
  }
  else
  {
    setViewMode(QListView::ViewMode::ListMode);
    setGridSize(QSize());
  }
}

void plQtAssetBrowserView::SetIconScale(plInt32 iIconSizePercentage)
{
  m_iIconSizePercentage = plMath::Clamp(iIconSizePercentage, 10, 100);
  m_pDelegate->SetIconScale(m_iIconSizePercentage);

  if (viewMode() != QListView::ViewMode::IconMode)
    return;

  setGridSize(m_pDelegate->sizeHint(QStyleOptionViewItem(), QModelIndex()));
}

plInt32 plQtAssetBrowserView::GetIconScale() const
{
  return m_iIconSizePercentage;
}

void plQtAssetBrowserView::dragEnterEvent(QDragEnterEvent* event)
{
  if (event->source())
    event->acceptProposedAction();
}

void plQtAssetBrowserView::dragMoveEvent(QDragMoveEvent* event)
{
  event->acceptProposedAction();
}

void plQtAssetBrowserView::dragLeaveEvent(QDragLeaveEvent* event)
{
  event->accept();
}

void plQtAssetBrowserView::dropEvent(QDropEvent* event)
{
  if (!event->mimeData()->hasUrls())
    return;

  QList<QUrl> paths = event->mimeData()->urls();
  plString targetPar = indexAt(event->pos()).data(plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  if (targetPar.IsEmpty())
  {
    return;
  }

  for (auto it = paths.begin(); it != paths.end(); it++)
  {
    plStringBuilder src = it->path().toUtf8().constData();

    src.Shrink(1, 0); //remove prepending '/' in name that appears for some reason

    plStringBuilder target = targetPar;
    target.AppendFormat("/{}", it->fileName().toUtf8());

    if (targetPar == src)
    {
      plLog::Error("Can't drop a file or folder on itself");
      continue;
    }


    if (!plOSFile::ExistsDirectory(src) && !plOSFile::ExistsFile(src))
    {
      plLog::Error("Cannot find file/folder to move : {}", src);
      return;
    }
    if (plOSFile::ExistsDirectory(src))
    {
      if (plOSFile::ExistsDirectory(target))  //ask to overwrite if target already exists
      {
        plStringBuilder msg = plStringBuilder();
        msg.Format("destination {} already exists", target);
        QMessageBox msgBox;
        msgBox.setText(msg.GetData());
        msgBox.setInformativeText("Overwrite existing folder?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        if (!msgBox.exec())
          return;

        if (plOSFile::DeleteFolder(target).Failed())
        {
          plLog::Error("Failed to delete folder {}", target);
          return;
        }
      }

      if (plOSFile::CreateDirectoryStructure(target).Succeeded())
      {
        if (plOSFile::CopyFolder(src, target).Failed())
        {
          plLog::Error("Failed to copy folder {} content", src);
        }
        if (plOSFile::DeleteFolder(src).Failed())
        {
          plLog::Error("Failed to delete folder {}", src);
        }
      }
      else
      {
        plLog::Error("Failed to copy folder {} to {}", src, target);
      }
    }
    else if (plOSFile::ExistsFile(src))
    {
      if (plOSFile::ExistsFile(target))   //ask to overwrite if target already exists
      {
        plStringBuilder msg = plStringBuilder();
        msg.Format("destination {} already exists", target);
        QMessageBox msgBox;
        msgBox.setText(msg.GetData());
        msgBox.setInformativeText("Overwrite existing file?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        if (!msgBox.exec())
          continue;

        if (plOSFile::DeleteFile(target).Failed())
        {
          plLog::Error("Failed to delete file {}", target);
          return;
        }
      }
      if (plOSFile::MoveFileOrDirectory(src, target).Failed())
      {
        plLog::Error("failed to move file or dir from {} to {}", src, target);
        return;
      }
    }
  }

  plAssetCurator::GetSingleton()->CheckFileSystem();
}


void plQtAssetBrowserView::wheelEvent(QWheelEvent* pEvent)
{
  if (pEvent->modifiers() == Qt::CTRL)
  {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    plInt32 iDelta = pEvent->angleDelta().y() > 0 ? 5 : -5;
#else
    plInt32 iDelta = pEvent->delta() > 0 ? 5 : -5;
#endif
    SetIconScale(m_iIconSizePercentage + iDelta);
    Q_EMIT ViewZoomed(m_iIconSizePercentage);
    return;
  }

  QListView::wheelEvent(pEvent);
}

plQtIconViewDelegate::plQtIconViewDelegate(plQtAssetBrowserView* pParent)
  : plQtItemDelegate(pParent)
{
  m_bDrawTransformState = true;
  m_iIconSizePercentage = 100;
  m_pView = pParent;
}

void plQtIconViewDelegate::SetIconScale(plInt32 iIconSizePercentage)
{
  m_iIconSizePercentage = iIconSizePercentage;
}

bool plQtIconViewDelegate::mousePressEvent(QMouseEvent* event, const QStyleOptionViewItem& opt, const QModelIndex& index)
{
  const plUInt32 uiThumbnailSize = ThumbnailSize();
  QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
  thumbnailRect.setSize(QSize(16, 16));
  if (thumbnailRect.contains(event->localPos().toPoint()))
  {
    event->accept();
    return true;
  }
  return false;
}

bool plQtIconViewDelegate::mouseReleaseEvent(QMouseEvent* event, const QStyleOptionViewItem& opt, const QModelIndex& index)
{
  const plUInt32 uiThumbnailSize = ThumbnailSize();
  QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
  thumbnailRect.setSize(QSize(16, 16));
  if (thumbnailRect.contains(event->localPos().toPoint()))
  {
    plUuid guid = index.data(plQtAssetBrowserModel::UserRoles::AssetGuid).value<plUuid>();

    plTransformStatus ret = plAssetCurator::GetSingleton()->TransformAsset(guid, plTransformFlags::TriggeredManually);

    if (ret.Failed())
    {
      QString path = index.data(plQtAssetBrowserModel::UserRoles::ParentRelativePath).toString();
      plLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, path.toUtf8().data());
    }
    else
    {
      plAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }

    event->accept();
    return true;
  }
  return false;
}

void plQtIconViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  if (!IsInIconMode())
  {
    plQtItemDelegate::paint(painter, opt, index);
    return;
  }

  const plUInt32 uiThumbnailSize = ThumbnailSize();

  // Prepare painter.
  {
    painter->save();
    if (hasClipping())
      painter->setClipRect(opt.rect);

    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
  }

  // Draw highlight background (copy of QItemDelegate::drawBackground)
  {
    QRect highlightRect = opt.rect.adjusted(ItemSideMargin - HighlightBorderWidth, ItemSideMargin - HighlightBorderWidth, 0, 0);
    highlightRect.setHeight(uiThumbnailSize + 2 * HighlightBorderWidth);
    highlightRect.setWidth(uiThumbnailSize + 2 * HighlightBorderWidth);

    if ((opt.state & QStyle::State_Selected))
    {
      QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
      if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
        cg = QPalette::Inactive;

      painter->fillRect(highlightRect, opt.palette.brush(cg, QPalette::Highlight));
    }
    else
    {
      QVariant value = index.data(Qt::BackgroundRole);
      if (value.canConvert<QBrush>())
      {
        QPointF oldBO = painter->brushOrigin();
        painter->setBrushOrigin(highlightRect.topLeft());
        painter->fillRect(highlightRect, qvariant_cast<QBrush>(value));
        painter->setBrushOrigin(oldBO);
      }
    }
  }

  // Draw thumbnail.
  {
    QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
    thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
    QPixmap pixmap = qvariant_cast<QPixmap>(index.data(Qt::DecorationRole));
    painter->drawPixmap(thumbnailRect, pixmap);
  }

  // Draw icon.
  {
    QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin - 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
    thumbnailRect.setSize(QSize(16, 16));
    QIcon icon = qvariant_cast<QIcon>(index.data(plQtAssetBrowserModel::UserRoles::AssetIcon));
    icon.paint(painter, thumbnailRect);
  }

  // Draw Transform State Icon
  if (m_bDrawTransformState)
  {
    QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
    thumbnailRect.setSize(QSize(16, 16));

    plAssetInfo::TransformState state = (plAssetInfo::TransformState)index.data(plQtAssetBrowserModel::UserRoles::TransformState).toInt();

    switch (state)
    {
      case plAssetInfo::TransformState::Unknown:
        plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetUnknown.svg").paint(painter, thumbnailRect);
        break;
      case plAssetInfo::TransformState::NeedsThumbnail:
        plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsThumbnail.svg").paint(painter, thumbnailRect);
        break;
      case plAssetInfo::TransformState::NeedsTransform:
        plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsTransform.svg").paint(painter, thumbnailRect);
        break;
      case plAssetInfo::TransformState::UpToDate:
        plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetOk.svg").paint(painter, thumbnailRect);
        break;
      case plAssetInfo::TransformState::MissingDependency:
        plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingDependency.svg").paint(painter, thumbnailRect);
        break;
      case plAssetInfo::TransformState::MissingReference:
        plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingReference.svg").paint(painter, thumbnailRect);
        break;
      case plAssetInfo::TransformState::TransformError:
        plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetFailedTransform.svg").paint(painter, thumbnailRect);
        break;
      case plAssetInfo::TransformState::NeedsImport:
        plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsImport.svg").paint(painter, thumbnailRect);
        break;
      case plAssetInfo::TransformState::Folder:
        break;
      case plAssetInfo::TransformState::COUNT:
        break;
    }
  }

  // Draw caption.
  {
    //painter->setFont(GetFont());
    QRect textRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin + uiThumbnailSize, -ItemSideMargin, -ItemSideMargin-1); //magic -1, otherwise we can see the top of some letters on the third line

    QString caption = qvariant_cast<QString>(index.data(Qt::DisplayRole));
    painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere, caption);
  }


  painter->restore();
}

QSize plQtIconViewDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  if (IsInIconMode())
  {
    return ItemSize();
  }
  else
  {
    return plQtItemDelegate::sizeHint(opt, index);
  }
}

QSize plQtIconViewDelegate::ItemSize() const
{
  QFont font = GetFont();
  QFontMetrics fm(font);

  plUInt32 iThumbnail = ThumbnailSize();
  const plUInt32 iItemWidth = iThumbnail + 2 * ItemSideMargin;
  const plUInt32 iItemHeight = iThumbnail + 2 * (ItemSideMargin + fm.height() + TextSpacing);

  return QSize(iItemWidth, iItemHeight);
}

QFont plQtIconViewDelegate::GetFont() const
{
  QFont font = QApplication::font();

  float fScaleFactor = plMath::Clamp((1.0f + (m_iIconSizePercentage / 100.0f)) * 0.75f, 0.75f, 1.25f);

  font.setPointSizeF(font.pointSizeF() * fScaleFactor);
  return font;
}

plUInt32 plQtIconViewDelegate::ThumbnailSize() const
{
  return MaxSize * (float)m_iIconSizePercentage / 100.0f;
}

bool plQtIconViewDelegate::IsInIconMode() const
{
  return m_pView->viewMode() == QListView::ViewMode::IconMode;
}
