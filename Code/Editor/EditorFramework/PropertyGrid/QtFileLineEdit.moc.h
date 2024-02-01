#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <QLineEdit>

class plQtFilePropertyWidget;

  /// \brief A QLineEdit that is used by plQtFilePropertyWidget
class PL_EDITORFRAMEWORK_DLL plQtFileLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  explicit plQtFileLineEdit(plQtFilePropertyWidget* pParent = nullptr);
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;

  plQtFilePropertyWidget* m_pOwner = nullptr;
};

