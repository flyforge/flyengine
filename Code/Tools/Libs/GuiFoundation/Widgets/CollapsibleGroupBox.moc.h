#pragma once


#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>
#include <GuiFoundation/ui_CollapsibleGroupBox.h>

class PLASMA_GUIFOUNDATION_DLL plQtCollapsibleGroupBox : public plQtGroupBoxBase, protected Ui_CollapsibleGroupBox
{
  Q_OBJECT
public:
  explicit plQtCollapsibleGroupBox(QWidget* pParent);

  virtual void SetTitle(plStringView sTitle) override;
  virtual void SetIcon(const QIcon& icon) override;
  virtual void SetFillColor(const QColor& color) override;

  virtual void SetCollapseState(bool bCollapsed) override;
  virtual bool GetCollapseState() const override;

  virtual QWidget* GetContent() override;
  virtual QWidget* GetHeader() override;

protected:
  virtual bool eventFilter(QObject* object, QEvent* event) override;
  virtual void paintEvent(QPaintEvent* event) override;

protected:
  bool m_bCollapsed = false;
};

