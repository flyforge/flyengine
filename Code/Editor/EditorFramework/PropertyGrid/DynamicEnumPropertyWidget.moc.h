#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class QComboBox;

/// *** Asset Browser ***

class PL_EDITORFRAMEWORK_DLL plQtDynamicEnumPropertyWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtDynamicEnumPropertyWidget();


protected slots:
  void on_CurrentEnum_changed(int iEnum);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

protected:
  QComboBox* m_pWidget;
  QHBoxLayout* m_pLayout;
};

