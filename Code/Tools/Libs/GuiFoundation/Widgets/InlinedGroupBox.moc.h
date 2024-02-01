#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>

class PL_GUIFOUNDATION_DLL plQtInlinedGroupBox : public plQtGroupBoxBase
{
  Q_OBJECT
public:
  explicit plQtInlinedGroupBox(QWidget* pParent);

  virtual void SetTitle(plStringView sTitle) override;
  virtual void SetIcon(const QIcon& icon) override;
  virtual void SetFillColor(const QColor& color) override;

  virtual void SetCollapseState(bool bCollapsed) override;
  virtual bool GetCollapseState() const override;

  virtual QWidget* GetContent() override;
  virtual QWidget* GetHeader() override;

protected:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual bool eventFilter(QObject* object, QEvent* event) override;

protected:
  QWidget* m_pContent;
  QWidget* m_pHeader;
};

