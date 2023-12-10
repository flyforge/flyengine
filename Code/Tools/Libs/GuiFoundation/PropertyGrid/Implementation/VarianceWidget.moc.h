#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/VarianceTypes.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>

class QSlider;

class plQtVarianceTypeWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtVarianceTypeWidget();

  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items) override;

private Q_SLOTS:
  void onBeginTemporary();
  void onEndTemporary();
  void SlotValueChanged();
  void SlotVarianceChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

  bool m_bTemporaryCommand = false;
  QHBoxLayout* m_pLayout = nullptr;
  plQtDoubleSpinBox* m_pValueWidget = nullptr;
  QSlider* m_pVarianceWidget = nullptr;
  const plAbstractMemberProperty* m_pValueProp = nullptr;
  const plAbstractMemberProperty* m_pVarianceProp = nullptr;
};

