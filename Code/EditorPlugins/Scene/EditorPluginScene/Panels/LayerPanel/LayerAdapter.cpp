#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QToolTip>

plQtLayerAdapter::plQtLayerAdapter(plScene2Document* pDocument)
  : plQtDocumentTreeModelAdapter(pDocument->GetSceneObjectManager(), plGetStaticRTTI<plSceneLayer>(), nullptr)
{
  m_pSceneDocument = pDocument;
  m_pSceneDocument->m_LayerEvents.AddEventHandler(
    plMakeDelegate(&plQtLayerAdapter::LayerEventHandler, this), m_LayerEventUnsubscriber);

  plDocument::s_EventsAny.AddEventHandler(plMakeDelegate(&plQtLayerAdapter::DocumentEventHander, this), m_DocumentEventUnsubscriber);
}

plQtLayerAdapter::~plQtLayerAdapter()
{
  m_LayerEventUnsubscriber.Unsubscribe();
  m_DocumentEventUnsubscriber.Unsubscribe();
}

QVariant plQtLayerAdapter::data(const plDocumentObject* pObject, int row, int column, int role) const
{
  switch (role)
  {
    case UserRoles::LayerGuid:
    {
      plObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      plUuid layerGuid = pAccessor->Get<plUuid>(pObject, "Layer");
      return QVariant::fromValue(layerGuid);
    }
    break;
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
      plObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      plUuid layerGuid = pAccessor->Get<plUuid>(pObject, "Layer");
      // Use curator to get name in case the layer is unloaded and there is no document to query.
      const plAssetCurator::plLockedSubAsset subAsset = plAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
      if (subAsset.isValid())
      {
        if (role == Qt::ToolTipRole)
        {
          return subAsset->m_pAssetInfo->m_sAbsolutePath.GetData();
        }
        plStringBuilder sName = subAsset->GetName();
        QString sQtName = QString::fromUtf8(sName.GetData());
        if (plSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
        {
          if (pLayer->IsModified())
          {
            sQtName += "*";
          }
        }
        return sQtName;
      }
      else
      {
        return QStringLiteral("Layer guid not found");
      }
    }
    break;

    case Qt::DecorationRole:
    {
      return plQtUiServices::GetCachedIconResource(":/EditorPluginScene/Icons/Layer.svg");
    }
    break;
    case Qt::ForegroundRole:
    {
      plObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      plUuid layerGuid = pAccessor->Get<plUuid>(pObject, "Layer");
      if (!m_pSceneDocument->IsLayerLoaded(layerGuid))
      {
        return QColor(128, 128, 128);
      }
    }
    break;
    case Qt::FontRole:
    {
      QFont font;
      plObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      plUuid layerGuid = pAccessor->Get<plUuid>(pObject, "Layer");
      if (m_pSceneDocument->GetActiveLayer() == layerGuid)
        font.setBold(true);
      return font;
    }
    break;
  }

  return QVariant();
}

bool plQtLayerAdapter::setData(const plDocumentObject* pObject, int row, int column, const QVariant& value, int role) const
{
  return false;
}

void plQtLayerAdapter::LayerEventHandler(const plScene2LayerEvent& e)
{
  switch (e.m_Type)
  {
    case plScene2LayerEvent::Type::LayerUnloaded:
    case plScene2LayerEvent::Type::LayerLoaded:
    {
      QVector<int> v;
      v.push_back(Qt::DisplayRole);
      v.push_back(Qt::ForegroundRole);
      Q_EMIT dataChanged(m_pSceneDocument->GetLayerObject(e.m_layerGuid), v);
    }
    break;
    case plScene2LayerEvent::Type::ActiveLayerChanged:
    {
      QVector<int> v;
      v.push_back(Qt::FontRole);
      if (auto pObject = m_pSceneDocument->GetLayerObject(m_CurrentActiveLayer))
      {
        Q_EMIT dataChanged(pObject, v);
      }
      Q_EMIT dataChanged(m_pSceneDocument->GetLayerObject(e.m_layerGuid), v);
      m_CurrentActiveLayer = e.m_layerGuid;
    }
    default:
      break;
  }
}

void plQtLayerAdapter::DocumentEventHander(const plDocumentEvent& e)
{
  if (e.m_Type == plDocumentEvent::Type::DocumentSaved || e.m_Type == plDocumentEvent::Type::ModifiedChanged)
  {
    const plDocumentObject* pLayerObj = m_pSceneDocument->GetLayerObject(e.m_pDocument->GetGuid());
    if (pLayerObj)
    {
      QVector<int> v;
      v.push_back(Qt::DisplayRole);
      v.push_back(Qt::ForegroundRole);
      Q_EMIT dataChanged(pLayerObj, v);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

plQtLayerDelegate::plQtLayerDelegate(QObject* pParent, plScene2Document* pDocument)
  : plQtItemDelegate(pParent)
  , m_pDocument(pDocument)
{
}

bool plQtLayerDelegate::mousePressEvent(QMouseEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  const QRect visibleRect = GetVisibleIconRect(option);
  const QRect loadedRect = GetLoadedIconRect(option);
  if (event->button() == Qt::MouseButton::LeftButton && (visibleRect.contains(event->localPos().toPoint()) || loadedRect.contains(event->localPos().toPoint())))
  {
    m_bPressed = true;
    event->accept();
    return true;
  }
  return plQtItemDelegate::mousePressEvent(event, option, index);
}

bool plQtLayerDelegate::mouseReleaseEvent(QMouseEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (m_bPressed)
  {
    const QRect visibleRect = GetVisibleIconRect(option);
    const QRect loadedRect = GetLoadedIconRect(option);
    if (visibleRect.contains(event->localPos().toPoint()))
    {
      const plUuid layerGuid = index.data(plQtLayerAdapter::UserRoles::LayerGuid).value<plUuid>();
      const bool bVisible = !m_pDocument->IsLayerVisible(layerGuid);
      m_pDocument->SetLayerVisible(layerGuid, bVisible).LogFailure();
    }
    else if (loadedRect.contains(event->localPos().toPoint()))
    {
      const plUuid layerGuid = index.data(plQtLayerAdapter::UserRoles::LayerGuid).value<plUuid>();
      if (layerGuid != m_pDocument->GetGuid())
      {
        plLayerAction::ToggleLayerLoaded(m_pDocument, layerGuid);
      }
    }
    m_bPressed = false;
    event->accept();
    return true;
  }
  return plQtItemDelegate::mouseReleaseEvent(event, option, index);
}

bool plQtLayerDelegate::mouseMoveEvent(QMouseEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (m_bPressed)
  {
    return true;
  }
  return plQtItemDelegate::mouseMoveEvent(event, option, index);
}

void plQtLayerDelegate::paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  plQtItemDelegate::paint(painter, opt, index);

  {
    const plUuid layerGuid = index.data(plQtLayerAdapter::UserRoles::LayerGuid).value<plUuid>();
    if (layerGuid.IsValid())
    {
      {
        const QRect thumbnailRect = GetVisibleIconRect(opt);
        const bool bVisible = m_pDocument->IsLayerVisible(layerGuid);
        const QIcon::Mode mode = bVisible ? QIcon::Mode::Normal : QIcon::Mode::Disabled;
        plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginScene/Icons/LayerVisible.svg").paint(painter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, mode);
      }

      if (layerGuid != m_pDocument->GetGuid())
      {
        const QRect thumbnailRect = GetLoadedIconRect(opt);
        const bool bLoaded = m_pDocument->IsLayerLoaded(layerGuid);
        const QIcon::Mode mode = bLoaded ? QIcon::Mode::Normal : QIcon::Mode::Disabled;
        plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginScene/Icons/LayerLoaded.svg").paint(painter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, mode);
      }
    }
  }
}

QSize plQtLayerDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  return plQtItemDelegate::sizeHint(opt, index);
}

bool plQtLayerDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  const plUuid layerGuid = index.data(plQtLayerAdapter::UserRoles::LayerGuid).value<plUuid>();
  if (layerGuid.IsValid())
  {
    const QRect visibleRect = GetVisibleIconRect(option);
    const QRect loadedRect = GetLoadedIconRect(option);
    if (visibleRect.contains(event->pos()))
    {
      const bool bVisible = m_pDocument->IsLayerVisible(layerGuid);
      QToolTip::showText(event->globalPos(), bVisible ? "Hide Layer" : "Show Layer", view);
      return true;
    }
    else if (loadedRect.contains(event->pos()))
    {
      const bool bLoaded = m_pDocument->IsLayerLoaded(layerGuid);
      QToolTip::showText(event->globalPos(), bLoaded ? "Unload Layer" : "Load Layer", view);
      return true;
    }
  }
  return plQtItemDelegate::helpEvent(event, view, option, index);
}

QRect plQtLayerDelegate::GetVisibleIconRect(const QStyleOptionViewItem& opt)
{
  return opt.rect.adjusted(opt.rect.width() - opt.rect.height(), 0, 0, 0);
}

QRect plQtLayerDelegate::GetLoadedIconRect(const QStyleOptionViewItem& opt)
{
  return opt.rect.adjusted(opt.rect.width() - opt.rect.height() * 2, 0, -opt.rect.height(), 0);
}

//////////////////////////////////////////////////////////////////////////

plQtLayerModel::plQtLayerModel(plScene2Document* pDocument)
  : plQtDocumentTreeModel(pDocument->GetSceneObjectManager(), pDocument->GetSettingsObject()->GetGuid())
  , m_pDocument(pDocument)
{
  m_sTargetContext = "layertree";
}
