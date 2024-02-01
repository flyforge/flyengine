#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetBrowserView.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>


plQtAssetBrowserView::plQtAssetBrowserView(QWidget* pParent)
  : plQtItemView<QListView>(pParent)
{
  m_iIconSizePercentage = 100;
  m_pDelegate = new plQtIconViewDelegate(this);

  SetDialogMode(false);
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
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pDelegate->SetDrawTransformState(false);
    setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  }
  else
  {
    setEditTriggers(QAbstractItemView::EditKeyPressed);
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

bool plQtIconViewDelegate::mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& opt, const QModelIndex& index)
{
  const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)index.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  if (!itemType.IsSet(plAssetBrowserItemFlags::Asset))
    return false;

  const plUInt32 uiThumbnailSize = ThumbnailSize();
  QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
  thumbnailRect.setSize(QSize(16, 16));
  if (thumbnailRect.contains(pEvent->position().toPoint()))
  {
    pEvent->accept();
    return true;
  }
  return false;
}

bool plQtIconViewDelegate::mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& opt, const QModelIndex& index)
{
  const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)index.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  if (!itemType.IsSet(plAssetBrowserItemFlags::Asset))
    return false;

  const plUInt32 uiThumbnailSize = ThumbnailSize();
  QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
  thumbnailRect.setSize(QSize(16, 16));
  if (thumbnailRect.contains(pEvent->position().toPoint()))
  {
    plUuid guid = index.data(plQtAssetBrowserModel::UserRoles::AssetGuid).value<plUuid>();

    plTransformStatus ret = plAssetCurator::GetSingleton()->TransformAsset(guid, plTransformFlags::TriggeredManually);

    if (ret.Failed())
    {
      QString path = index.data(plQtAssetBrowserModel::UserRoles::RelativePath).toString();
      plLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, path.toUtf8().data());
    }
    else
    {
      plAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }

    pEvent->accept();
    return true;
  }
  return false;
}

QWidget* plQtIconViewDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  plStringBuilder sAbsPath = index.data(plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().constData();

  QLineEdit* editor = new QLineEdit(pParent);
  editor->setValidator(new plFileNameValidator(editor, sAbsPath.GetFileDirectory(), sAbsPath.GetFileNameAndExtension()));
  return editor;
}

void plQtIconViewDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const
{
  QString sOldName = index.data(plQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
  QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(pEditor);
  pModel->setData(index, pLineEdit->text());
}

void plQtIconViewDelegate::updateEditorGeometry(QWidget* pEditor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  if (!pEditor)
    return;

  const plUInt32 uiThumbnailSize = ThumbnailSize();
  const QRect textRect = option.rect.adjusted(ItemSideMargin, ItemSideMargin + uiThumbnailSize + TextSpacing, -ItemSideMargin, -ItemSideMargin - TextSpacing);
  pEditor->setGeometry(textRect);
}

void plQtIconViewDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  if (!IsInIconMode())
  {
    plQtItemDelegate::paint(pPainter, opt, index);
    return;
  }

  const plUInt32 uiThumbnailSize = ThumbnailSize();
  const plBitflags<plAssetBrowserItemFlags> itemType = (plAssetBrowserItemFlags::Enum)index.data(plQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  // Prepare painter.
  {
    pPainter->save();
    if (hasClipping())
      pPainter->setClipRect(opt.rect);

    pPainter->setRenderHint(QPainter::SmoothPixmapTransform, true);
  }

  // Draw assets with a background to distinguish them easily from normal files / folders.
  if (itemType.IsAnySet(plAssetBrowserItemFlags::Asset | plAssetBrowserItemFlags::SubAsset))
  {
    QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
      cg = QPalette::Inactive;

    plInt32 border = ItemSideMargin - HighlightBorderWidth;
    QRect assetRect = opt.rect.adjusted(border, border, -border, -border);
    pPainter->fillRect(assetRect, opt.palette.brush(cg, QPalette::AlternateBase));
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

      pPainter->fillRect(highlightRect, opt.palette.brush(cg, QPalette::Highlight));
    }
    else
    {
      QVariant value = index.data(Qt::BackgroundRole);
      if (value.canConvert<QBrush>())
      {
        QPointF oldBO = pPainter->brushOrigin();
        pPainter->setBrushOrigin(highlightRect.topLeft());
        pPainter->fillRect(highlightRect, qvariant_cast<QBrush>(value));
        pPainter->setBrushOrigin(oldBO);
      }
    }
  }

  if (itemType.IsAnySet(plAssetBrowserItemFlags::File) && !itemType.IsAnySet(plAssetBrowserItemFlags::Asset))
  {
    // Draw thumbnail.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
      thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
      QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
      icon.paint(pPainter, thumbnailRect);
    }

    // Draw icon.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin - 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
      thumbnailRect.setSize(QSize(16, 16));
      QIcon icon = qvariant_cast<QIcon>(index.data(plQtAssetBrowserModel::UserRoles::AssetIcon));
      icon.paint(pPainter, thumbnailRect);
    }
  }
  else if (itemType.IsAnySet(plAssetBrowserItemFlags::Folder | plAssetBrowserItemFlags::DataDirectory))
  {
    // Draw icon.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
      thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
      QIcon icon = qvariant_cast<QIcon>(index.data(plQtAssetBrowserModel::UserRoles::AssetIcon));
      icon.paint(pPainter, thumbnailRect);
    }
  }
  else // asset
  {
    // Draw thumbnail.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
      thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
      QPixmap pixmap = qvariant_cast<QPixmap>(index.data(Qt::DecorationRole));
      pPainter->drawPixmap(thumbnailRect, pixmap);
    }

    // Draw icon.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin - 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
      thumbnailRect.setSize(QSize(16, 16));
      QIcon icon = qvariant_cast<QIcon>(index.data(plQtAssetBrowserModel::UserRoles::AssetIcon));
      icon.paint(pPainter, thumbnailRect);
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
          plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetUnknown.svg").paint(pPainter, thumbnailRect);
          break;
        case plAssetInfo::TransformState::NeedsThumbnail:
          plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsThumbnail.svg").paint(pPainter, thumbnailRect);
          break;
        case plAssetInfo::TransformState::NeedsTransform:
          plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsTransform.svg").paint(pPainter, thumbnailRect);
          break;
        case plAssetInfo::TransformState::UpToDate:
          plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetOk.svg").paint(pPainter, thumbnailRect);
          break;
        case plAssetInfo::TransformState::MissingTransformDependency:
          plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingDependency.svg").paint(pPainter, thumbnailRect);
          break;
        case plAssetInfo::TransformState::MissingThumbnailDependency:
          plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingReference.svg").paint(pPainter, thumbnailRect);
          break;
        case plAssetInfo::TransformState::CircularDependency:
          plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetFailedTransform.svg").paint(pPainter, thumbnailRect);
          break;
        case plAssetInfo::TransformState::TransformError:
          plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetFailedTransform.svg").paint(pPainter, thumbnailRect);
          break;
        case plAssetInfo::TransformState::NeedsImport:
          plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsImport.svg").paint(pPainter, thumbnailRect);
          break;
        case plAssetInfo::TransformState::COUNT:
          break;
      }
    }
  }

  // Draw caption.
  {
    pPainter->setFont(GetFont());
    QRect textRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin + uiThumbnailSize + TextSpacing, -ItemSideMargin, -ItemSideMargin - TextSpacing);

    QString caption = qvariant_cast<QString>(index.data(Qt::DisplayRole));
    pPainter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere, caption);
  }


  pPainter->restore();
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
  return static_cast<plUInt32>((float)MaxSize * (float)m_iIconSizePercentage / 100.0f);
}

bool plQtIconViewDelegate::IsInIconMode() const
{
  return m_pView->viewMode() == QListView::ViewMode::IconMode;
}
