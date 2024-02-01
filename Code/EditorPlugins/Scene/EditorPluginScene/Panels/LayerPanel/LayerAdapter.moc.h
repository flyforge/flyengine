#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>

class plScene2Document;
struct plScene2LayerEvent;
struct plDocumentEvent;

/// \brief Custom adapter for layers, used in plQtLayerPanel.
class PL_EDITORPLUGINSCENE_DLL plQtLayerAdapter : public plQtDocumentTreeModelAdapter
{
  Q_OBJECT;

public:
  plQtLayerAdapter(plScene2Document* pDocument);
  ~plQtLayerAdapter();
  virtual QVariant data(const plDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;
  virtual bool setData(const plDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const override;

  enum UserRoles
  {
    LayerGuid = Qt::UserRole + 0,
  };

private:
  void LayerEventHandler(const plScene2LayerEvent& e);
  void DocumentEventHander(const plDocumentEvent& e);

private:
  plScene2Document* m_pSceneDocument;
  plEvent<const plScene2LayerEvent&>::Unsubscriber m_LayerEventUnsubscriber;
  plEvent<const plDocumentEvent&>::Unsubscriber m_DocumentEventUnsubscriber;
  plUuid m_CurrentActiveLayer;
};

/// \brief Custom delegate for layers, used in plQtLayerPanel.
/// Provides buttons to toggle the layer visible / loaded states.
/// Relies on plQtLayerAdapter to trigger updates and provide the LayerGuid.
class plQtLayerDelegate : public plQtItemDelegate
{
  Q_OBJECT
public:
  plQtLayerDelegate(QObject* pParent, plScene2Document* pDocument);

  virtual bool mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseMoveEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual bool helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  static QRect GetVisibleIconRect(const QStyleOptionViewItem& opt);
  static QRect GetLoadedIconRect(const QStyleOptionViewItem& opt);

  bool m_bPressed = false;
  plScene2Document* m_pDocument = nullptr;
};

/// \brief Custom model for layers, used in plQtLayerPanel.
class plQtLayerModel : public plQtDocumentTreeModel
{
  Q_OBJECT

public:
  plQtLayerModel(plScene2Document* pDocument);
  ~plQtLayerModel() = default;

private:
  plScene2Document* m_pDocument = nullptr;
};
