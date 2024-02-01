#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class QPushButton;
class QMenu;
class QCheckBox;

class PL_GUIFOUNDATION_DLL plQtPropertyEditorTagSetWidget : public plQtPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorTagSetWidget();
  virtual ~plQtPropertyEditorTagSetWidget();

  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items) override;
  virtual bool HasLabel() const override { return true; }

protected:
  virtual void DoPrepareToDie() override {}

private Q_SLOTS:
  void on_Menu_aboutToShow();
  void onCheckBoxClicked(bool bChecked);

private:
  virtual void OnInit() override;
  void InternalUpdateValue();

private:
  plDynamicArray<QCheckBox*> m_Tags;
  QHBoxLayout* m_pLayout;
  QPushButton* m_pWidget;
  QMenu* m_pMenu;
};

