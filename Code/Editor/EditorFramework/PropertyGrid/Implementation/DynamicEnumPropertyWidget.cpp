#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>

plQtDynamicEnumPropertyWidget::plQtDynamicEnumPropertyWidget()
  : plQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QComboBox(this);
  m_pWidget->installEventFilter(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  PL_VERIFY(connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int))) != nullptr, "connection failed");
}

void plQtDynamicEnumPropertyWidget::OnInit()
{
  PL_ASSERT_DEV(
    m_pProp->GetAttributeByType<plDynamicEnumAttribute>() != nullptr, "plQtDynamicEnumPropertyWidget was created without a plDynamicEnumAttribute!");

  const plDynamicEnumAttribute* pAttr = m_pProp->GetAttributeByType<plDynamicEnumAttribute>();

  const auto& denum = plDynamicEnum::GetDynamicEnum(pAttr->GetDynamicEnumName());
  const auto& AllValues = denum.GetAllValidValues();

  plQtScopedBlockSignals bs(m_pWidget);

  for (auto it = AllValues.GetIterator(); it.IsValid(); ++it)
  {
    m_pWidget->addItem(QString::fromUtf8(it.Value().GetData()), it.Key());
  }
}

void plQtDynamicEnumPropertyWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    plInt32 iIndex = m_pWidget->findData(value.ConvertTo<plInt64>());
    // PL_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!"); // 'invalid value'
    m_pWidget->setCurrentIndex(iIndex);
  }
  else
  {
    m_pWidget->setCurrentIndex(-1);
  }
}

void plQtDynamicEnumPropertyWidget::on_CurrentEnum_changed(int iEnum)
{
  plInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}
