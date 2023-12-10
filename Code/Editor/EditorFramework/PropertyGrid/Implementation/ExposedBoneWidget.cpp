#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/ExposedBoneWidget.moc.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QBoxLayout>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plQtExposedBoneWidget::plQtExposedBoneWidget()
{
  m_pRotWidget[0] = nullptr;
  m_pRotWidget[1] = nullptr;
  m_pRotWidget[2] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (plInt32 c = 0; c < 3; ++c)
  {
    m_pRotWidget[c] = new plQtDoubleSpinBox(this);
    m_pRotWidget[c]->setMinimum(-plMath::Infinity<double>());
    m_pRotWidget[c]->setMaximum(plMath::Infinity<double>());
    m_pRotWidget[c]->setSingleStep(1.0);
    m_pRotWidget[c]->setAccelerated(true);
    m_pRotWidget[c]->setDisplaySuffix("\xC2\xB0");

    policy.setHorizontalStretch(2);
    m_pRotWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pRotWidget[c]);

    connect(m_pRotWidget[c], SIGNAL(editingFinished()), this, SLOT(onEndTemporary()));
    connect(m_pRotWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void plQtExposedBoneWidget::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  plQtStandardPropertyWidget::SetSelection(items);
  PLASMA_ASSERT_DEBUG(m_pProp->GetSpecificType()->IsDerivedFrom<plExposedBone>(), "Selection does not match plExposedBone.");
}

void plQtExposedBoneWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;
}

void plQtExposedBoneWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void plQtExposedBoneWidget::SlotValueChanged()
{
  onBeginTemporary();

  auto obj = m_OldValue.Get<plTypedObject>();
  plExposedBone* pCopy = reinterpret_cast<plExposedBone*>(plReflectionSerializer::Clone(obj.m_pObject, obj.m_pType));

  {
    plAngle x = plAngle::MakeFromDegree(m_pRotWidget[0]->value());
    plAngle y = plAngle::MakeFromDegree(m_pRotWidget[1]->value());
    plAngle z = plAngle::MakeFromDegree(m_pRotWidget[2]->value());

    pCopy->m_Transform.m_qRotation = plQuat::MakeFromEulerAngles(x, y, z);
  }

  plVariant newValue;
  newValue.MoveTypedObject(pCopy, obj.m_pType);

  BroadcastValueChanged(newValue);
}

void plQtExposedBoneWidget::OnInit()
{
}

void plQtExposedBoneWidget::InternalSetValue(const plVariant& value)
{
  if (value.GetReflectedType() != plGetStaticRTTI<plExposedBone>())
    return;

  const plExposedBone* pBone = reinterpret_cast<const plExposedBone*>(value.GetData());

  plQtScopedBlockSignals b0(m_pRotWidget[0]);
  plQtScopedBlockSignals b1(m_pRotWidget[1]);
  plQtScopedBlockSignals b2(m_pRotWidget[2]);

  if (value.IsValid())
  {
    plAngle x, y, z;
    pBone->m_Transform.m_qRotation.GetAsEulerAngles(x, y, z);

    m_pRotWidget[0]->setValue(x.GetDegree());
    m_pRotWidget[1]->setValue(y.GetDegree());
    m_pRotWidget[2]->setValue(z.GetDegree());
  }
  else
  {
    m_pRotWidget[0]->setValueInvalid();
    m_pRotWidget[1]->setValueInvalid();
    m_pRotWidget[2]->setValueInvalid();
  }
}
