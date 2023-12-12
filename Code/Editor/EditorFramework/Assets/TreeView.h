#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <QTreeWidget>


class plQtAssetTreeWidget : public QTreeWidget
{
public:
  plQtAssetTreeWidget(QWidget* parent);
  virtual ~plQtAssetTreeWidget() = default;

  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* event) override;
  void dragLeaveEvent(QDragLeaveEvent* event) override;
  void dropEvent(QDropEvent* event) override;

  void mousePressEvent(QMouseEvent* event) override;

  QMimeData* mimeData(const QList<QTreeWidgetItem*>& items) const override;

  void resetTree();

private:
  plDynamicArray<plString> m_expandedNodes;

};
