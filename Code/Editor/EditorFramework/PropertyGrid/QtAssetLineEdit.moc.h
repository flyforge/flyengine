#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QLineEdit>
#include <QModelIndex>

class plQtAssetPropertyWidget;

/// \brief A QLineEdit that is used by plQtAssetPropertyWidget
class PLASMA_EDITORFRAMEWORK_DLL plQtAssetLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  explicit plQtAssetLineEdit(QWidget* pParent = nullptr);
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;
  virtual void paintEvent(QPaintEvent* e) override;

  plQtAssetPropertyWidget* m_pOwner = nullptr;
};

