#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/EditDynamicEnumsDlg.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

plQtDynamicStringEnumPropertyWidget::plQtDynamicStringEnumPropertyWidget()
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

void plQtDynamicStringEnumPropertyWidget::OnInit()
{
  PL_ASSERT_DEV(m_pProp->GetAttributeByType<plDynamicStringEnumAttribute>() != nullptr,
    "plQtDynamicStringEnumPropertyWidget was created without a plDynamicStringEnumAttribute!");

  const plDynamicStringEnumAttribute* pAttr = m_pProp->GetAttributeByType<plDynamicStringEnumAttribute>();

  m_pEnum = &plDynamicStringEnum::GetDynamicEnum(pAttr->GetDynamicEnumName());

  if (auto pDefaultValueAttr = m_pProp->GetAttributeByType<plDefaultValueAttribute>())
  {
    m_pEnum->AddValidValue(pDefaultValueAttr->GetValue().ConvertTo<plString>(), true);
  }

  const auto& AllValues = m_pEnum->GetAllValidValues();

  plQtScopedBlockSignals bs(m_pWidget);
  m_pWidget->clear();

  for (const auto& val : AllValues)
  {
    m_pWidget->addItem(QString::fromUtf8(val.GetData()));
  }

  if (!m_pEnum->GetStorageFile().IsEmpty())
  {
    m_pWidget->addItem("< Edit Values... >", QString("<edit>"));
  }
}

void plQtDynamicStringEnumPropertyWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    m_iLastIndex = m_pWidget->findText(value.ConvertTo<plString>().GetData());
  }
  else
  {
    m_iLastIndex = -1;
  }

  m_pWidget->setCurrentIndex(m_iLastIndex);
}

void plQtDynamicStringEnumPropertyWidget::on_CurrentEnum_changed(int iEnum)
{
  if (m_pWidget->currentData() == QString("<edit>"))
  {
    plQtEditDynamicEnumsDlg dlg(m_pEnum, this);
    if (dlg.exec() == QDialog::Accepted)
    {
      iEnum = dlg.GetSelectedItem();
      OnInit();
    }
    else
    {
      iEnum = m_iLastIndex;
    }

    m_pWidget->setCurrentIndex(iEnum);
    return;
  }

  m_iLastIndex = iEnum;
  QString sValue = m_pWidget->itemText(iEnum);
  BroadcastValueChanged(sValue.toUtf8().data());
}
