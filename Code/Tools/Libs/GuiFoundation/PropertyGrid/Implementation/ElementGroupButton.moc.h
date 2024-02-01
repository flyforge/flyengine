#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QToolButton>

class PL_GUIFOUNDATION_DLL plQtElementGroupButton : public QToolButton
{
  Q_OBJECT
public:
  enum class ElementAction
  {
    MoveElementUp,
    MoveElementDown,
    DeleteElement,
    Help,
  };

  explicit plQtElementGroupButton(QWidget* pParent, ElementAction action, plQtPropertyWidget* pGroupWidget);
  ElementAction GetAction() const { return m_Action; }
  plQtPropertyWidget* GetGroupWidget() const { return m_pGroupWidget; }

private:
  ElementAction m_Action;
  plQtPropertyWidget* m_pGroupWidget;
};

