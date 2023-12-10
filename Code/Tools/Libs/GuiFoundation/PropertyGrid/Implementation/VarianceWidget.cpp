#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/PropertyGrid/Implementation/VarianceWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QBoxLayout>
#include <QSlider>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plQtVarianceTypeWidget::plQtVarianceTypeWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pValueWidget = new plQtDoubleSpinBox(this);
  m_pValueWidget->installEventFilter(m_pValueWidget);
  m_pValueWidget->setMinimum(-plMath::Infinity<double>());
  m_pValueWidget->setMaximum(plMath::Infinity<double>());
  m_pValueWidget->setSingleStep(0.1f);
  m_pValueWidget->setAccelerated(true);
  m_pValueWidget->setDecimals(2);

  m_pVarianceWidget = new QSlider(this);
  m_pVarianceWidget->setOrientation(Qt::Orientation::Horizontal);
  m_pVarianceWidget->setMinimum(0);
  m_pVarianceWidget->setMaximum(100);
  m_pVarianceWidget->setSingleStep(1);

  m_pLayout->addWidget(m_pValueWidget);
  m_pLayout->addWidget(m_pVarianceWidget);

  connect(m_pValueWidget, SIGNAL(editingFinished()), this, SLOT(onEndTemporary()));
  connect(m_pValueWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  connect(m_pVarianceWidget, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
  connect(m_pVarianceWidget, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
  connect(m_pVarianceWidget, SIGNAL(valueChanged(int)), this, SLOT(SlotVarianceChanged()));
}

void plQtVarianceTypeWidget::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  plQtStandardPropertyWidget::SetSelection(items);
  PLASMA_ASSERT_DEBUG(m_pProp->GetSpecificType()->IsDerivedFrom<plVarianceTypeBase>(), "Selection does not match plVarianceType.");
}

void plQtVarianceTypeWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;
}

void plQtVarianceTypeWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void plQtVarianceTypeWidget::SlotValueChanged()
{
  onBeginTemporary();

  plVariant value;
  plToolsReflectionUtils::GetVariantFromFloat(m_pValueWidget->value(), m_pValueProp->GetSpecificType()->GetVariantType(), value);

  auto obj = m_OldValue.Get<plTypedObject>();
  void* pCopy = plReflectionSerializer::Clone(obj.m_pObject, obj.m_pType);
  plReflectionUtils::SetMemberPropertyValue(m_pValueProp, pCopy, value);
  plVariant newValue;
  newValue.MoveTypedObject(pCopy, obj.m_pType);

  BroadcastValueChanged(newValue);
}


void plQtVarianceTypeWidget::SlotVarianceChanged()
{
  double variance = plMath::Clamp<double>(m_pVarianceWidget->value() / 100.0, 0, 1);

  plVariant newValue = m_OldValue;
  plTypedPointer ptr = newValue.GetWriteAccess();
  plReflectionUtils::SetMemberPropertyValue(m_pVarianceProp, ptr.m_pObject, variance);

  BroadcastValueChanged(newValue);
}

void plQtVarianceTypeWidget::OnInit()
{
  m_pValueProp = static_cast<const plAbstractMemberProperty*>(GetProperty()->GetSpecificType()->FindPropertyByName("Value"));
  m_pVarianceProp = static_cast<const plAbstractMemberProperty*>(GetProperty()->GetSpecificType()->FindPropertyByName("Variance"));

  // Property type adjustments
  plQtScopedBlockSignals bs(m_pValueWidget);
  const plRTTI* pValueType = m_pValueProp->GetSpecificType();
  if (pValueType == plGetStaticRTTI<plTime>())
  {
    m_pValueWidget->setDisplaySuffix(" sec");
  }
  else if (pValueType == plGetStaticRTTI<plAngle>())
  {
    m_pValueWidget->setDisplaySuffix(plStringUtf8(L"\u00B0").GetData());
  }

  // Handle attributes
  if (const plSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<plSuffixAttribute>())
  {
    m_pValueWidget->setDisplaySuffix(pSuffix->GetSuffix());
  }
  if (const plClampValueAttribute* pClamp = m_pProp->GetAttributeByType<plClampValueAttribute>())
  {
    if (pClamp->GetMinValue().CanConvertTo<double>())
    {
      m_pValueWidget->setMinimum(pClamp->GetMinValue());
    }
    else if (const plRTTI* pType = pClamp->GetMinValue().GetReflectedType(); pType && pType->IsDerivedFrom<plVarianceTypeBase>())
    {
      m_pValueWidget->setMinimum(pClamp->GetMinValue()["Value"]);
      m_pVarianceWidget->setMinimum(static_cast<plInt32>(pClamp->GetMinValue()["Variance"].ConvertTo<double>() * 100.0));
    }
    if (pClamp->GetMaxValue().CanConvertTo<double>())
    {
      m_pValueWidget->setMaximum(pClamp->GetMaxValue());
    }
    else if (const plRTTI* pType = pClamp->GetMaxValue().GetReflectedType(); pType && pType->IsDerivedFrom<plVarianceTypeBase>())
    {
      m_pValueWidget->setMaximum(pClamp->GetMaxValue()["Value"]);
      m_pVarianceWidget->setMaximum(static_cast<plInt32>(pClamp->GetMaxValue()["Variance"].ConvertTo<double>() * 100.0));
    }
  }
  if (const plDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<plDefaultValueAttribute>())
  {
    if (pDefault->GetValue().CanConvertTo<double>())
    {
      m_pValueWidget->setDefaultValue(pDefault->GetValue());
    }
    else if (const plRTTI* pType = pDefault->GetValue().GetReflectedType(); pType && pType->IsDerivedFrom<plVarianceTypeBase>())
    {
      m_pValueWidget->setDefaultValue(pDefault->GetValue()["Value"]);
    }
  }
}

void plQtVarianceTypeWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals bs(m_pValueWidget, m_pVarianceWidget);
  if (value.IsValid())
  {
    m_pValueWidget->setValue(value["Value"]);
    m_pVarianceWidget->setValue(value["Variance"].ConvertTo<double>() * 100.0);
  }
  else
  {
    m_pValueWidget->setValueInvalid();
    m_pVarianceWidget->setValue(50);
  }
}
