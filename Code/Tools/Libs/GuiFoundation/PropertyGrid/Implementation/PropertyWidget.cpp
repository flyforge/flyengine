#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Dialogs/CurveEditDlg.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <QComboBox>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QWidgetAction>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <qcheckbox.h>
#include <qlayout.h>


/// *** CHECKBOX ***

plQtPropertyEditorCheckboxWidget::plQtPropertyEditorCheckboxWidget()
  : plQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QCheckBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  PLASMA_VERIFY(connect(m_pWidget, SIGNAL(stateChanged(int)), this, SLOT(on_StateChanged_triggered(int))) != nullptr, "signal/slot connection failed");
}

void plQtPropertyEditorCheckboxWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    m_pWidget->setTristate(false);
    m_pWidget->setChecked(value.ConvertTo<bool>() ? Qt::Checked : Qt::Unchecked);
  }
  else
  {
    m_pWidget->setTristate(true);
    m_pWidget->setCheckState(Qt::CheckState::PartiallyChecked);
  }
}

void plQtPropertyEditorCheckboxWidget::mousePressEvent(QMouseEvent* pEv)
{
  QWidget::mousePressEvent(pEv);

  m_pWidget->toggle();
}

void plQtPropertyEditorCheckboxWidget::on_StateChanged_triggered(int state)
{
  if (state == Qt::PartiallyChecked)
  {
    plQtScopedBlockSignals b(m_pWidget);

    m_pWidget->setCheckState(Qt::Checked);
    m_pWidget->setTristate(false);
  }

  BroadcastValueChanged((state != Qt::Unchecked) ? true : false);
}


/// *** DOUBLE SPINBOX ***

plQtPropertyEditorDoubleSpinboxWidget::plQtPropertyEditorDoubleSpinboxWidget(plInt8 iNumComponents)
  : plQtStandardPropertyWidget()
{
  PLASMA_ASSERT_DEBUG(iNumComponents <= 4, "Only up to 4 components are supported");

  m_iNumComponents = iNumComponents;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (plInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new plQtDoubleSpinBox(this);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(-plMath::Infinity<double>());
    m_pWidget[c]->setMaximum(plMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(0.1f);
    m_pWidget[c]->setAccelerated(true);

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void plQtPropertyEditorDoubleSpinboxWidget::OnInit()
{
  auto pNoTemporaryTransactions = m_pProp->GetAttributeByType<plNoTemporaryTransactionsAttribute>();
  m_bUseTemporaryTransaction = (pNoTemporaryTransactions == nullptr);

  if (const plClampValueAttribute* pClamp = m_pProp->GetAttributeByType<plClampValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        plQtScopedBlockSignals bs(m_pWidget[0]);
        m_pWidget[0]->setMinimum(pClamp->GetMinValue());
        m_pWidget[0]->setMaximum(pClamp->GetMaxValue());
        break;
      }
      case 2:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pClamp->GetMinValue().CanConvertTo<plVec2>())
        {
          plVec2 value = pClamp->GetMinValue().ConvertTo<plVec2>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
        }
        if (pClamp->GetMaxValue().CanConvertTo<plVec2>())
        {
          plVec2 value = pClamp->GetMaxValue().ConvertTo<plVec2>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
        }
        break;
      }
      case 3:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pClamp->GetMinValue().CanConvertTo<plVec3>())
        {
          plVec3 value = pClamp->GetMinValue().ConvertTo<plVec3>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
        }
        if (pClamp->GetMaxValue().CanConvertTo<plVec3>())
        {
          plVec3 value = pClamp->GetMaxValue().ConvertTo<plVec3>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
        }
        break;
      }
      case 4:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pClamp->GetMinValue().CanConvertTo<plVec4>())
        {
          plVec4 value = pClamp->GetMinValue().ConvertTo<plVec4>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
          m_pWidget[3]->setMinimum(value.w);
        }
        if (pClamp->GetMaxValue().CanConvertTo<plVec4>())
        {
          plVec4 value = pClamp->GetMaxValue().ConvertTo<plVec4>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
          m_pWidget[3]->setMaximum(value.w);
        }
        break;
      }
    }
  }

  if (const plDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<plDefaultValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        plQtScopedBlockSignals bs(m_pWidget[0]);

        if (pDefault->GetValue().CanConvertTo<double>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<double>());
        }
        break;
      }
      case 2:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pDefault->GetValue().CanConvertTo<plVec2>())
        {
          plVec2 value = pDefault->GetValue().ConvertTo<plVec2>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
        }
        break;
      }
      case 3:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<plVec3>())
        {
          plVec3 value = pDefault->GetValue().ConvertTo<plVec3>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
        }
        break;
      }
      case 4:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<plVec4>())
        {
          plVec4 value = pDefault->GetValue().ConvertTo<plVec4>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
          m_pWidget[3]->setDefaultValue(value.w);
        }
        break;
      }
    }
  }

  if (const plSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<plSuffixAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  if (const plMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<plMinValueTextAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(pMinValueText->GetText());
    }
  }
}

void plQtPropertyEditorDoubleSpinboxWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

  m_OriginalType = value.GetType();

  if (value.IsValid())
  {
    switch (m_iNumComponents)
    {
      case 1:
        m_pWidget[0]->setValue(value.ConvertTo<double>());
        break;
      case 2:
        m_pWidget[0]->setValue(value.ConvertTo<plVec2>().x);
        m_pWidget[1]->setValue(value.ConvertTo<plVec2>().y);
        break;
      case 3:
        m_pWidget[0]->setValue(value.ConvertTo<plVec3>().x);
        m_pWidget[1]->setValue(value.ConvertTo<plVec3>().y);
        m_pWidget[2]->setValue(value.ConvertTo<plVec3>().z);
        break;
      case 4:
        m_pWidget[0]->setValue(value.ConvertTo<plVec4>().x);
        m_pWidget[1]->setValue(value.ConvertTo<plVec4>().y);
        m_pWidget[2]->setValue(value.ConvertTo<plVec4>().z);
        m_pWidget[3]->setValue(value.ConvertTo<plVec4>().w);
        break;
    }
  }
  else
  {
    switch (m_iNumComponents)
    {
      case 1:
        m_pWidget[0]->setValueInvalid();
        break;
      case 2:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        break;
      case 3:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        m_pWidget[2]->setValueInvalid();
        break;
      case 4:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        m_pWidget[2]->setValueInvalid();
        m_pWidget[3]->setValueInvalid();
        break;
    }
  }
}

void plQtPropertyEditorDoubleSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bUseTemporaryTransaction && m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void plQtPropertyEditorDoubleSpinboxWidget::SlotValueChanged()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  switch (m_iNumComponents)
  {
    case 1:
      BroadcastValueChanged(plVariant(m_pWidget[0]->value()).ConvertTo(m_OriginalType));
      break;
    case 2:
      BroadcastValueChanged(plVec2(m_pWidget[0]->value(), m_pWidget[1]->value()));
      break;
    case 3:
      BroadcastValueChanged(plVec3(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value()));
      break;
    case 4:
      BroadcastValueChanged(plVec4(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value()));
      break;
  }
}


/// *** TIME SPINBOX ***

plQtPropertyEditorTimeWidget::plQtPropertyEditorTimeWidget()
  : plQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  {
    m_pWidget = new plQtDoubleSpinBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setDisplaySuffix(" sec");
    m_pWidget->setMinimum(-plMath::Infinity<double>());
    m_pWidget->setMaximum(plMath::Infinity<double>());
    m_pWidget->setSingleStep(0.1f);
    m_pWidget->setAccelerated(true);

    policy.setHorizontalStretch(2);
    m_pWidget->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void plQtPropertyEditorTimeWidget::OnInit()
{
  const plClampValueAttribute* pClamp = m_pProp->GetAttributeByType<plClampValueAttribute>();
  if (pClamp)
  {
    plQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setMinimum(pClamp->GetMinValue());
    m_pWidget->setMaximum(pClamp->GetMaxValue());
  }

  const plDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<plDefaultValueAttribute>();
  if (pDefault)
  {
    plQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setDefaultValue(pDefault->GetValue());
  }
}

void plQtPropertyEditorTimeWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b0(m_pWidget);
  m_pWidget->setValue(value);
}

void plQtPropertyEditorTimeWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void plQtPropertyEditorTimeWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged(plTime::MakeFromSeconds(m_pWidget->value()));
}


/// *** ANGLE SPINBOX ***

plQtPropertyEditorAngleWidget::plQtPropertyEditorAngleWidget()
  : plQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  {
    m_pWidget = new plQtDoubleSpinBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setDisplaySuffix(plStringUtf8(L"\u00B0").GetData());
    m_pWidget->setMinimum(-plMath::Infinity<double>());
    m_pWidget->setMaximum(plMath::Infinity<double>());
    m_pWidget->setSingleStep(0.1f);
    m_pWidget->setAccelerated(true);
    m_pWidget->setDecimals(1);

    policy.setHorizontalStretch(2);
    m_pWidget->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void plQtPropertyEditorAngleWidget::OnInit()
{
  const plClampValueAttribute* pClamp = m_pProp->GetAttributeByType<plClampValueAttribute>();
  if (pClamp)
  {
    plQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setMinimum(pClamp->GetMinValue());
    m_pWidget->setMaximum(pClamp->GetMaxValue());
  }

  const plDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<plDefaultValueAttribute>();
  if (pDefault)
  {
    plQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setDefaultValue(pDefault->GetValue());
  }

  const plSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<plSuffixAttribute>();
  if (pSuffix)
  {
    m_pWidget->setDisplaySuffix(pSuffix->GetSuffix());
  }

  const plMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<plMinValueTextAttribute>();
  if (pMinValueText)
  {
    m_pWidget->setSpecialValueText(pMinValueText->GetText());
  }
}

void plQtPropertyEditorAngleWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b0(m_pWidget);
  m_pWidget->setValue(value);
}

void plQtPropertyEditorAngleWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void plQtPropertyEditorAngleWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged(plAngle::MakeFromDegree(m_pWidget->value()));
}

/// *** INT SPINBOX ***


plQtPropertyEditorIntSpinboxWidget::plQtPropertyEditorIntSpinboxWidget(plInt8 iNumComponents, plInt32 iMinValue, plInt32 iMaxValue)
  : plQtStandardPropertyWidget()
{
  m_iNumComponents = iNumComponents;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();
  policy.setHorizontalStretch(2);

  for (plInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new plQtDoubleSpinBox(this, true);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(iMinValue);
    m_pWidget[c]->setMaximum(iMaxValue);
    m_pWidget[c]->setSingleStep(1);
    m_pWidget[c]->setAccelerated(true);

    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}


plQtPropertyEditorIntSpinboxWidget::~plQtPropertyEditorIntSpinboxWidget() = default;

void plQtPropertyEditorIntSpinboxWidget::OnInit()
{
  auto pNoTemporaryTransactions = m_pProp->GetAttributeByType<plNoTemporaryTransactionsAttribute>();
  m_bUseTemporaryTransaction = (pNoTemporaryTransactions == nullptr);

  if (const plClampValueAttribute* pClamp = m_pProp->GetAttributeByType<plClampValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        const plInt32 iMinValue = pClamp->GetMinValue().ConvertTo<plInt32>();
        const plInt32 iMaxValue = pClamp->GetMaxValue().ConvertTo<plInt32>();

        plQtScopedBlockSignals bs(m_pWidget[0]);
        m_pWidget[0]->setMinimum(pClamp->GetMinValue());
        m_pWidget[0]->setMaximum(pClamp->GetMaxValue());

        if (pClamp->GetMinValue().IsValid() && pClamp->GetMaxValue().IsValid() && (iMaxValue - iMinValue) < 256 && m_bUseTemporaryTransaction)
        {
          plQtScopedBlockSignals bs2(m_pSlider);

          // we have to create the slider here, because in the constructor we don't know the real
          // min and max values from the plClampValueAttribute (only the rough type ranges)
          m_pSlider = new QSlider(this);
          m_pSlider->installEventFilter(this);
          m_pSlider->setOrientation(Qt::Orientation::Horizontal);
          m_pSlider->setMinimum(iMinValue);
          m_pSlider->setMaximum(iMaxValue);

          m_pLayout->insertWidget(0, m_pSlider, 5); // make it take up most of the space
          connect(m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(SlotSliderValueChanged(int)));
          connect(m_pSlider, SIGNAL(sliderReleased()), this, SLOT(on_EditingFinished_triggered()));
        }

        break;
      }
      case 2:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pClamp->GetMinValue().CanConvertTo<plVec2I32>())
        {
          plVec2I32 value = pClamp->GetMinValue().ConvertTo<plVec2I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
        }
        if (pClamp->GetMaxValue().CanConvertTo<plVec2I32>())
        {
          plVec2I32 value = pClamp->GetMaxValue().ConvertTo<plVec2I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
        }
        break;
      }
      case 3:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pClamp->GetMinValue().CanConvertTo<plVec3I32>())
        {
          plVec3I32 value = pClamp->GetMinValue().ConvertTo<plVec3I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
        }
        if (pClamp->GetMaxValue().CanConvertTo<plVec3I32>())
        {
          plVec3I32 value = pClamp->GetMaxValue().ConvertTo<plVec3I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
        }
        break;
      }
      case 4:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pClamp->GetMinValue().CanConvertTo<plVec4I32>())
        {
          plVec4I32 value = pClamp->GetMinValue().ConvertTo<plVec4I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
          m_pWidget[3]->setMinimum(value.w);
        }
        if (pClamp->GetMaxValue().CanConvertTo<plVec4I32>())
        {
          plVec4I32 value = pClamp->GetMaxValue().ConvertTo<plVec4I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
          m_pWidget[3]->setMaximum(value.w);
        }
        break;
      }
    }
  }

  if (const plDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<plDefaultValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pSlider);

        if (pDefault->GetValue().CanConvertTo<plInt32>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<plInt32>());

          if (m_pSlider)
          {
            m_pSlider->setValue(pDefault->GetValue().ConvertTo<plInt32>());
          }
        }
        break;
      }
      case 2:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pDefault->GetValue().CanConvertTo<plVec2I32>())
        {
          plVec2I32 value = pDefault->GetValue().ConvertTo<plVec2I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
        }
        break;
      }
      case 3:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<plVec3I32>())
        {
          plVec3I32 value = pDefault->GetValue().ConvertTo<plVec3I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
        }
        break;
      }
      case 4:
      {
        plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<plVec4I32>())
        {
          plVec4I32 value = pDefault->GetValue().ConvertTo<plVec4I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
          m_pWidget[3]->setDefaultValue(value.w);
        }
        break;
      }
    }
  }

  if (const plSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<plSuffixAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  if (const plMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<plMinValueTextAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(pMinValueText->GetText());
    }
  }
}

void plQtPropertyEditorIntSpinboxWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3], m_pSlider);

  m_OriginalType = value.GetType();

  switch (m_iNumComponents)
  {
    case 1:
      m_pWidget[0]->setValue(value.ConvertTo<plInt32>());

      if (m_pSlider)
      {
        m_pSlider->setValue(value.ConvertTo<plInt32>());
      }

      break;
    case 2:
      m_pWidget[0]->setValue(value.ConvertTo<plVec2I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<plVec2I32>().y);
      break;
    case 3:
      m_pWidget[0]->setValue(value.ConvertTo<plVec3I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<plVec3I32>().y);
      m_pWidget[2]->setValue(value.ConvertTo<plVec3I32>().z);
      break;
    case 4:
      m_pWidget[0]->setValue(value.ConvertTo<plVec4I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<plVec4I32>().y);
      m_pWidget[2]->setValue(value.ConvertTo<plVec4I32>().z);
      m_pWidget[3]->setValue(value.ConvertTo<plVec4I32>().w);
      break;
  }
}

void plQtPropertyEditorIntSpinboxWidget::SlotValueChanged()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  plVariant newValue;
  switch (m_iNumComponents)
  {
    case 1:
      newValue = m_pWidget[0]->value();

      if (m_pSlider)
      {
        plQtScopedBlockSignals b0(m_pSlider);
        m_pSlider->setValue((plInt32)m_pWidget[0]->value());
      }

      break;
    case 2:
      newValue = plVec2I32(m_pWidget[0]->value(), m_pWidget[1]->value());
      break;
    case 3:
      newValue = plVec3I32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value());
      break;
    case 4:
      newValue = plVec4I32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value());
      break;
  }

  BroadcastValueChanged(newValue.ConvertTo(m_OriginalType));
}

void plQtPropertyEditorIntSpinboxWidget::SlotSliderValueChanged(int value)
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  {
    plQtScopedBlockSignals b0(m_pWidget[0]);
    m_pWidget[0]->setValue(value);
  }

  BroadcastValueChanged(plVariant(m_pSlider->value()).ConvertTo(m_OriginalType));
}

void plQtPropertyEditorIntSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bUseTemporaryTransaction && m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}


/// *** QUATERNION ***

plQtPropertyEditorQuaternionWidget::plQtPropertyEditorQuaternionWidget()
  : plQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (plInt32 c = 0; c < 3; ++c)
  {
    m_pWidget[c] = new plQtDoubleSpinBox(this);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(-plMath::Infinity<double>());
    m_pWidget[c]->setMaximum(plMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(1.0);
    m_pWidget[c]->setAccelerated(true);
    m_pWidget[c]->setDisplaySuffix("\xC2\xB0");

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void plQtPropertyEditorQuaternionWidget::OnInit() {}

void plQtPropertyEditorQuaternionWidget::InternalSetValue(const plVariant& value)
{
  if (m_bTemporaryCommand)
    return;

  plQtScopedBlockSignals b0(m_pWidget[0]);
  plQtScopedBlockSignals b1(m_pWidget[1]);
  plQtScopedBlockSignals b2(m_pWidget[2]);

  if (value.IsValid())
  {
    const plQuat qRot = value.ConvertTo<plQuat>();
    plAngle x, y, z;
    qRot.GetAsEulerAngles(x, y, z);

    m_pWidget[0]->setValue(x.GetDegree());
    m_pWidget[1]->setValue(y.GetDegree());
    m_pWidget[2]->setValue(z.GetDegree());
  }
  else
  {
    m_pWidget[0]->setValueInvalid();
    m_pWidget[1]->setValueInvalid();
    m_pWidget[2]->setValueInvalid();
  }
}

void plQtPropertyEditorQuaternionWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void plQtPropertyEditorQuaternionWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(plPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  plAngle x = plAngle::MakeFromDegree(m_pWidget[0]->value());
  plAngle y = plAngle::MakeFromDegree(m_pWidget[1]->value());
  plAngle z = plAngle::MakeFromDegree(m_pWidget[2]->value());

  plQuat qRot = plQuat::MakeFromEulerAngles(x, y, z);

  BroadcastValueChanged(qRot);
}

/// *** LINEEDIT ***

plQtPropertyEditorLineEditWidget::plQtPropertyEditorLineEditWidget()
  : plQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QLineEdit(this);
  m_pWidget->installEventFilter(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered()));
}

void plQtPropertyEditorLineEditWidget::OnInit()
{
  if (m_pProp->GetAttributeByType<plReadOnlyAttribute>() != nullptr || m_pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
  {
    setEnabled(true);

    plQtScopedBlockSignals bs(m_pWidget);

    m_pWidget->setReadOnly(true);
    QPalette palette = m_pWidget->palette();
    palette.setColor(QPalette::Base, QColor(0, 0, 0, 0));
    m_pWidget->setPalette(palette);
  }
}

void plQtPropertyEditorLineEditWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b(m_pWidget);

  m_OriginalType = value.GetType();

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(value.ConvertTo<plString>().GetData()));
  }
}

void plQtPropertyEditorLineEditWidget::on_TextChanged_triggered(const QString& value)
{
  BroadcastValueChanged(plVariant(value.toUtf8().data()).ConvertTo(m_OriginalType));
}

void plQtPropertyEditorLineEditWidget::on_TextFinished_triggered()
{
  BroadcastValueChanged(plVariant(m_pWidget->text().toUtf8().data()).ConvertTo(m_OriginalType));
}


/// *** COLOR ***

plQtColorButtonWidget::plQtColorButtonWidget(QWidget* pParent)
  : QFrame(pParent)
{
  setAutoFillBackground(true);
  setCursor(Qt::PointingHandCursor);
}

void plQtColorButtonWidget::SetColor(const plVariant& color)
{
  if (color.IsValid())
  {
    plColor col0 = color.ConvertTo<plColor>();
    col0.NormalizeToLdrRange();

    const plColorGammaUB col = col0;

    QColor qol;
    qol.setRgb(col.r, col.g, col.b, col.a);

    m_Pal.setBrush(QPalette::Window, QBrush(qol, Qt::SolidPattern));
    setPalette(m_Pal);
  }
  else
  {
    setPalette(m_Pal);
  }
}

void plQtColorButtonWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  setPalette(m_Pal);
  QFrame::showEvent(event);
}

void plQtColorButtonWidget::mouseReleaseEvent(QMouseEvent* event)
{
  Q_EMIT clicked();
}

QSize plQtColorButtonWidget::sizeHint() const
{
  return minimumSizeHint();
}

QSize plQtColorButtonWidget::minimumSizeHint() const
{
  QFontMetrics fm(font());

  QStyleOptionFrame opt;
  initStyleOption(&opt);
  return style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(20, fm.height()), this);
}

plQtPropertyEditorColorWidget::plQtPropertyEditorColorWidget()
  : plQtStandardPropertyWidget()
{
  m_bExposeAlpha = false;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new plQtColorButtonWidget(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pWidget);

  PLASMA_VERIFY(connect(m_pWidget, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void plQtPropertyEditorColorWidget::OnInit()
{
  m_bExposeAlpha = (m_pProp->GetAttributeByType<plExposeColorAlphaAttribute>() != nullptr);
}

void plQtPropertyEditorColorWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b(m_pWidget);

  m_OriginalValue = GetOldValue();
  m_pWidget->SetColor(value);
}

void plQtPropertyEditorColorWidget::on_Button_triggered()
{
  Broadcast(plPropertyEvent::Type::BeginTemporary);

  bool bShowHDR = false;

  plColor temp = plColor::White;
  if (m_OriginalValue.IsValid())
  {
    bShowHDR = m_OriginalValue.IsA<plColor>();

    temp = m_OriginalValue.ConvertTo<plColor>();
  }

  plQtUiServices::GetSingleton()->ShowColorDialog(
    temp, m_bExposeAlpha, bShowHDR, this, SLOT(on_CurrentColor_changed(const plColor&)), SLOT(on_Color_accepted()), SLOT(on_Color_reset()));
}

void plQtPropertyEditorColorWidget::on_CurrentColor_changed(const plColor& color)
{
  plVariant col;

  if (m_OriginalValue.IsA<plColorGammaUB>())
  {
    // plVariant does not down-cast to plColorGammaUB automatically
    col = plColorGammaUB(color);
  }
  else
  {
    col = color;
  }

  m_pWidget->SetColor(col);
  BroadcastValueChanged(col);
}

void plQtPropertyEditorColorWidget::on_Color_reset()
{
  m_pWidget->SetColor(m_OriginalValue);
  Broadcast(plPropertyEvent::Type::CancelTemporary);
}

void plQtPropertyEditorColorWidget::on_Color_accepted()
{
  m_OriginalValue = GetOldValue();
  Broadcast(plPropertyEvent::Type::EndTemporary);
}


/// *** ENUM COMBOBOX ***

plQtPropertyEditorEnumWidget::plQtPropertyEditorEnumWidget()
  : plQtStandardPropertyWidget()
{

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QComboBox(this);
  m_pWidget->installEventFilter(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int)));
}

void plQtPropertyEditorEnumWidget::OnInit()
{
  const plRTTI* pType = m_pProp->GetSpecificType();

  plQtScopedBlockSignals bs(m_pWidget);

  plUInt32 uiCount = pType->GetProperties().GetCount();
  // Start at 1 to skip default value.
  for (plUInt32 i = 1; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != plPropertyCategory::Constant)
      continue;

    const plAbstractConstantProperty* pConstant = static_cast<const plAbstractConstantProperty*>(pProp);

    m_pWidget->addItem(plMakeQString(plTranslate(pConstant->GetPropertyName())), pConstant->GetConstant().ConvertTo<plInt64>());
  }
}

void plQtPropertyEditorEnumWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    plInt32 iIndex = m_pWidget->findData(value.ConvertTo<plInt64>());
    PLASMA_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!");
    m_pWidget->setCurrentIndex(iIndex);
  }
  else
  {
    m_pWidget->setCurrentIndex(-1);
  }
}

void plQtPropertyEditorEnumWidget::on_CurrentEnum_changed(int iEnum)
{
  plInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}


/// *** BITFLAGS COMBOBOX ***

plQtPropertyEditorBitflagsWidget::plQtPropertyEditorBitflagsWidget()
  : plQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QPushButton(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pMenu = new QMenu(m_pWidget);
  m_pWidget->setMenu(m_pMenu);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(on_Menu_aboutToShow()));
  connect(m_pMenu, SIGNAL(aboutToHide()), this, SLOT(on_Menu_aboutToHide()));
}

plQtPropertyEditorBitflagsWidget::~plQtPropertyEditorBitflagsWidget()
{
  m_pWidget->setMenu(nullptr);
  delete m_pMenu;
}

void plQtPropertyEditorBitflagsWidget::OnInit()
{
  const plRTTI* enumType = m_pProp->GetSpecificType();

  const plRTTI* pType = enumType;
  plUInt32 uiCount = pType->GetProperties().GetCount();

  // Start at 1 to skip default value.
  for (plUInt32 i = 1; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != plPropertyCategory::Constant)
      continue;

    const plAbstractConstantProperty* pConstant = static_cast<const plAbstractConstantProperty*>(pProp);

    QWidgetAction* pAction = new QWidgetAction(m_pMenu);
    QCheckBox* pCheckBox = new QCheckBox(plMakeQString(plTranslate(pConstant->GetPropertyName())), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pAction->setDefaultWidget(pCheckBox);

    m_Constants[pConstant->GetConstant().ConvertTo<plInt64>()] = pCheckBox;
    m_pMenu->addAction(pAction);
  }

  // sets all bits to clear or set
  {
    QWidgetAction* pAllAction = new QWidgetAction(m_pMenu);
    m_pAllButton = new QPushButton(QString::fromUtf8("All"), m_pMenu);
    connect(m_pAllButton, &QPushButton::clicked, this, [this](bool bChecked)
      { SetAllChecked(true); });
    pAllAction->setDefaultWidget(m_pAllButton);
    m_pMenu->addAction(pAllAction);

    QWidgetAction* pClearAction = new QWidgetAction(m_pMenu);
    m_pClearButton = new QPushButton(QString::fromUtf8("Clear"), m_pMenu);
    connect(m_pClearButton, &QPushButton::clicked, this, [this](bool bChecked)
      { SetAllChecked(false); });
    pClearAction->setDefaultWidget(m_pClearButton);
    m_pMenu->addAction(pClearAction);
  }
}

void plQtPropertyEditorBitflagsWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b(m_pWidget);
  m_iCurrentBitflags = value.ConvertTo<plInt64>();

  QString sText;
  for (auto it = m_Constants.GetIterator(); it.IsValid(); ++it)
  {
    bool bChecked = (it.Key() & m_iCurrentBitflags) != 0;
    QString sName = it.Value()->text();
    if (bChecked)
    {
      sText += sName + "|";
    }
    it.Value()->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
  }
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);

  m_pWidget->setText(sText);
}

void plQtPropertyEditorBitflagsWidget::SetAllChecked(bool bChecked)
{
  for (auto& pCheckBox : m_Constants)
  {
    pCheckBox.Value()->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
  }
}

void plQtPropertyEditorBitflagsWidget::on_Menu_aboutToShow()
{
  m_pMenu->setMinimumWidth(m_pWidget->geometry().width());
}

void plQtPropertyEditorBitflagsWidget::on_Menu_aboutToHide()
{
  plInt64 iValue = 0;
  QString sText;
  for (auto it = m_Constants.GetIterator(); it.IsValid(); ++it)
  {
    bool bChecked = it.Value()->checkState() == Qt::Checked;
    QString sName = it.Value()->text();
    if (bChecked)
    {
      sText += sName + "|";
      iValue |= it.Key();
    }
  }
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);

  m_pWidget->setText(sText);

  if (m_iCurrentBitflags != iValue)
  {
    m_iCurrentBitflags = iValue;
    BroadcastValueChanged(m_iCurrentBitflags);
  }
}


/// *** CURVE1D ***

plQtCurve1DButtonWidget::plQtCurve1DButtonWidget(QWidget* pParent)
  : QLabel(pParent)
{
  setAutoFillBackground(true);
  setCursor(Qt::PointingHandCursor);
  setScaledContents(true);
}

void plQtCurve1DButtonWidget::UpdatePreview(plObjectAccessorBase* pObjectAccessor, const plDocumentObject* pCurveObject, QColor color, double fLowerExtents, bool bLowerFixed, double fUpperExtents, bool bUpperFixed, double fDefaultValue, double fLowerRange, double fUpperRange)
{
  plInt32 iNumPoints = 0;
  pObjectAccessor->GetCount(pCurveObject, "ControlPoints", iNumPoints).AssertSuccess();

  plVariant v;
  plHybridArray<plVec2d, 32> points;
  points.Reserve(iNumPoints);

  double minX = fLowerExtents * 4800.0;
  double maxX = fUpperExtents * 4800.0;

  double minY = fLowerRange;
  double maxY = fUpperRange;

  for (plInt32 i = 0; i < iNumPoints; ++i)
  {
    const plDocumentObject* pPoint = pObjectAccessor->GetChildObject(pCurveObject, "ControlPoints", i);

    plVec2d p;

    pObjectAccessor->GetValue(pPoint, "Tick", v).AssertSuccess();
    p.x = v.ConvertTo<double>();

    pObjectAccessor->GetValue(pPoint, "Value", v).AssertSuccess();
    p.y = v.ConvertTo<double>();

    points.PushBack(p);

    if (!bLowerFixed)
      minX = plMath::Min(minX, p.x);

    if (!bUpperFixed)
      maxX = plMath::Max(maxX, p.x);

    minY = plMath::Min(minY, p.y);
    maxY = plMath::Max(maxY, p.y);
  }

  const double pW = plMath::Max(10, size().width());
  const double pH = plMath::Clamp(size().height(), 5, 24);

  QPixmap pixmap((int)pW, (int)pH);
  pixmap.fill(palette().base().color());

  QPainter pt(&pixmap);
  pt.setPen(color);
  pt.setRenderHint(QPainter::RenderHint::Antialiasing);

  if (!points.IsEmpty())
  {
    points.Sort([](const plVec2d& lhs, const plVec2d& rhs) -> bool
      { return lhs.x < rhs.x; });

    const double normX = 1.0 / (maxX - minX);
    const double normY = 1.0 / (maxY - minY);

    QPainterPath path;

    {
      double startX = plMath::Min(minX, points[0].x);
      double startY = points[0].y;

      startX = (startX - minX) * normX;
      startY = 1.0 - ((startY - minY) * normY);

      path.moveTo((int)(startX * pW), (int)(startY * pH));
    }

    for (plUInt32 i = 0; i < points.GetCount(); ++i)
    {
      auto pt0 = points[i];
      pt0.x = (pt0.x - minX) * normX;
      pt0.y = 1.0 - ((pt0.y - minY) * normY);

      path.lineTo((int)(pt0.x * pW), (int)(pt0.y * pH));
    }

    {
      double endX = plMath::Max(maxX, points.PeekBack().x);
      double endY = points.PeekBack().y;

      endX = (endX - minX) * normX;
      endY = 1.0 - ((endY - minY) * normY);

      path.lineTo((int)(endX * pW), (int)(endY * pH));
    }

    pt.drawPath(path);
  }
  else
  {
    const double normY = 1.0 / (maxY - minY);
    double valY = 1.0 - ((fDefaultValue - minY) * normY);

    pt.drawLine(0, (int)(valY * pH), (int)pW, (int)(valY * pH));
  }

  setPixmap(pixmap);
}

void plQtCurve1DButtonWidget::mouseReleaseEvent(QMouseEvent* event)
{
  Q_EMIT clicked();
}

plQtPropertyEditorCurve1DWidget::plQtPropertyEditorCurve1DWidget()
  : plQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new plQtCurve1DButtonWidget(this);
  m_pButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pButton);

  PLASMA_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void plQtPropertyEditorCurve1DWidget::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  plQtPropertyWidget::SetSelection(items);

  UpdatePreview();
}

void plQtPropertyEditorCurve1DWidget::OnInit() {}
void plQtPropertyEditorCurve1DWidget::DoPrepareToDie() {}

void plQtPropertyEditorCurve1DWidget::UpdatePreview()
{
  if (m_Items.IsEmpty())
    return;

  const plDocumentObject* pParent = m_Items[0].m_pObject;
  const plDocumentObject* pCurve = m_pObjectAccessor->GetChildObject(pParent, m_pProp->GetPropertyName(), {});
  const plColorAttribute* pColorAttr = m_pProp->GetAttributeByType<plColorAttribute>();
  const plCurveExtentsAttribute* pExtentsAttr = m_pProp->GetAttributeByType<plCurveExtentsAttribute>();
  const plDefaultValueAttribute* pDefAttr = m_pProp->GetAttributeByType<plDefaultValueAttribute>();
  const plClampValueAttribute* pClampAttr = m_pProp->GetAttributeByType<plClampValueAttribute>();

  const bool bLowerFixed = pExtentsAttr ? pExtentsAttr->m_bLowerExtentFixed : false;
  const bool bUpperFixed = pExtentsAttr ? pExtentsAttr->m_bUpperExtentFixed : false;
  const double fLowerExt = pExtentsAttr ? pExtentsAttr->m_fLowerExtent : 0.0;
  const double fUpperExt = pExtentsAttr ? pExtentsAttr->m_fUpperExtent : 1.0;
  const plColorGammaUB color = pColorAttr ? pColorAttr->GetColor() : plColor::GreenYellow;
  const double fLowerRange = (pClampAttr && pClampAttr->GetMinValue().IsNumber()) ? pClampAttr->GetMinValue().ConvertTo<double>() : 0.0;
  const double fUpperRange = (pClampAttr && pClampAttr->GetMaxValue().IsNumber()) ? pClampAttr->GetMaxValue().ConvertTo<double>() : 1.0;
  const double fDefVal = (pDefAttr && pDefAttr->GetValue().IsNumber()) ? pDefAttr->GetValue().ConvertTo<double>() : 0.0;

  m_pButton->UpdatePreview(m_pObjectAccessor, pCurve, QColor(color.r, color.g, color.b), fLowerExt, bLowerFixed, fUpperExt, bUpperFixed, fDefVal, fLowerRange, fUpperRange);
}

void plQtPropertyEditorCurve1DWidget::on_Button_triggered()
{
  const plDocumentObject* pParent = m_Items[0].m_pObject;
  const plDocumentObject* pCurve = m_pObjectAccessor->GetChildObject(pParent, m_pProp->GetPropertyName(), {});
  const plColorAttribute* pColorAttr = m_pProp->GetAttributeByType<plColorAttribute>();
  const plCurveExtentsAttribute* pExtentsAttr = m_pProp->GetAttributeByType<plCurveExtentsAttribute>();
  const plClampValueAttribute* pClampAttr = m_pProp->GetAttributeByType<plClampValueAttribute>();

  // TODO: would like to have one transaction open to finish/cancel at the end
  // but also be able to undo individual steps while editing
  // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->StartTransaction("Edit Curve");

  plQtCurveEditDlg* pDlg = new plQtCurveEditDlg(m_pObjectAccessor, pCurve, this);
  pDlg->restoreGeometry(plQtCurveEditDlg::GetLastDialogGeometry());

  if (pColorAttr)
  {
    pDlg->SetCurveColor(pColorAttr->GetColor());
  }

  if (pExtentsAttr)
  {
    pDlg->SetCurveExtents(pExtentsAttr->m_fLowerExtent, pExtentsAttr->m_bLowerExtentFixed, pExtentsAttr->m_fUpperExtent, pExtentsAttr->m_bUpperExtentFixed);
  }

  if (pClampAttr)
  {
    const double fLower = pClampAttr->GetMinValue().IsNumber() ? pClampAttr->GetMinValue().ConvertTo<double>() : -plMath::HighValue<double>();
    const double fUpper = pClampAttr->GetMaxValue().IsNumber() ? pClampAttr->GetMaxValue().ConvertTo<double>() : plMath::HighValue<double>();

    pDlg->SetCurveRanges(fLower, fUpper);
  }

  if (pDlg->exec() == QDialog::Accepted)
  {
    // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->FinishTransaction();

    UpdatePreview();
  }
  else
  {
    // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->CancelTransaction();
  }

  delete pDlg;
}
