#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>
#include <QItemDelegate>
#include <QListView>

class plQtIconViewDelegate;

class plQtAssetBrowserView : public plQtItemView<QListView>
{
  Q_OBJECT

public:
  plQtAssetBrowserView(QWidget* pParent);
  void SetDialogMode(bool bDialogMode);

  void SetIconMode(bool bIconMode);
  void SetIconScale(plInt32 iIconSizePercentage);
  plInt32 GetIconScale() const;

  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* event) override;
  void dragLeaveEvent(QDragLeaveEvent* event) override;
  void dropEvent(QDropEvent* event) override;

Q_SIGNALS:
  void ViewZoomed(plInt32 iIconSizePercentage);

protected:
  virtual void wheelEvent(QWheelEvent* pEvent) override;

private:
  bool m_bDialogMode;
  plQtIconViewDelegate* m_pDelegate;
  plInt32 m_iIconSizePercentage;
};


class plQtIconViewDelegate : public plQtItemDelegate
{
  Q_OBJECT

public:
  plQtIconViewDelegate(plQtAssetBrowserView* pParent = nullptr);

  void SetDrawTransformState(bool b) { m_bDrawTransformState = b; }

  void SetIconScale(plInt32 iIconSizePercentage);

  virtual bool mousePressEvent(QMouseEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseReleaseEvent(QMouseEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index) override;

public:
  virtual void paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override;

private:
  QSize ItemSize() const;
  QFont GetFont() const;
  plUInt32 ThumbnailSize() const;
  bool IsInIconMode() const;

private:
  enum
  {
    MaxSize = plThumbnailSize,
    HighlightBorderWidth = 3,
    ItemSideMargin = 5,
    TextSpacing = 5
  };

  bool m_bDrawTransformState;
  plInt32 m_iIconSizePercentage;
  plQtAssetBrowserView* m_pView;
};
