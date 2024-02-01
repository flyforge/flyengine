#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class QComboBox;
class plDynamicStringEnum;

class PL_EDITORFRAMEWORK_DLL plQtDynamicStringEnumPropertyWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtDynamicStringEnumPropertyWidget();


protected slots:
  void on_CurrentEnum_changed(int iEnum);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

protected:
  QComboBox* m_pWidget;
  QHBoxLayout* m_pLayout;
  plDynamicStringEnum* m_pEnum = nullptr;
  plInt32 m_iLastIndex = -1;
};

