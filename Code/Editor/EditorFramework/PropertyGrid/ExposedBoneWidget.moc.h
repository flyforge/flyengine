#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

class QSlider;

class plQtExposedBoneWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtExposedBoneWidget();

  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items) override;

private Q_SLOTS:
  void onBeginTemporary();
  void onEndTemporary();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

  bool m_bTemporaryCommand = false;
  QHBoxLayout* m_pLayout = nullptr;
  plQtDoubleSpinBox* m_pRotWidget[3];
};
