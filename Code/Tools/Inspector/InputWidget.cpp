#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qlistwidget.h>

plQtInputWidget* plQtInputWidget::s_pWidget = nullptr;

plQtInputWidget::plQtInputWidget(QWidget* pParent)
  : ads::CDockWidget("Input Widget", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(InputWidgetFrame);

  setIcon(QIcon(":/Icons/Icons/InputActions.svg"));

  ResetStats();
}

void plQtInputWidget::ResetStats()
{
  ClearSlots();
  ClearActions();
}

void plQtInputWidget::ClearSlots()
{
  m_InputSlots.Clear();
  TableInputSlots->clear();

  {
    QStringList Headers;
    Headers.append("");
    Headers.append(" Slot ");
    Headers.append(" State ");
    Headers.append(" Value ");
    Headers.append(" Dead Zone ");
    Headers.append(" Flags (Binary) ");

    TableInputSlots->setColumnCount(static_cast<int>(Headers.size()));

    TableInputSlots->setHorizontalHeaderLabels(Headers);
    TableInputSlots->horizontalHeader()->show();
  }
}

void plQtInputWidget::ClearActions()
{
  m_InputActions.Clear();
  TableInputActions->clear();

  {
    QStringList Headers;
    Headers.append("");
    Headers.append(" Action ");
    Headers.append(" State ");
    Headers.append(" Value ");

    for (plInt32 slot = 0; slot < plInputActionConfig::MaxInputSlotAlternatives; ++slot)
      Headers.append(QString(" Slot %1 ").arg(slot + 1));

    TableInputActions->setColumnCount(static_cast<int>(Headers.size()));

    TableInputActions->setHorizontalHeaderLabels(Headers);
    TableInputActions->horizontalHeader()->show();
  }
}

void plQtInputWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  plTelemetryMessage msg;

  bool bUpdateSlotTable = false;
  bool bFillSlotTable = false;
  bool bUpdateActionTable = false;
  bool bFillActionTable = false;

  while (plTelemetry::RetrieveMessage('INPT', msg) == PLASMA_SUCCESS)
  {
    if (msg.GetMessageID() == 'SLOT')
    {
      plString sSlotName;
      msg.GetReader() >> sSlotName;

      SlotData& sd = s_pWidget->m_InputSlots[sSlotName];

      msg.GetReader() >> sd.m_uiSlotFlags;

      plUInt8 uiKeyState = 0;
      msg.GetReader() >> uiKeyState;
      sd.m_KeyState = (plKeyState::Enum)uiKeyState;

      msg.GetReader() >> sd.m_fValue;
      msg.GetReader() >> sd.m_fDeadZone;

      if (sd.m_iTableRow == -1)
        bUpdateSlotTable = true;

      bFillSlotTable = true;
    }

    if (msg.GetMessageID() == 'ACTN')
    {
      plString sInputSetName;
      msg.GetReader() >> sInputSetName;

      plString sActionName;
      msg.GetReader() >> sActionName;

      plStringBuilder sFinalName = sInputSetName.GetData();
      sFinalName.Append("::");
      sFinalName.Append(sActionName.GetData());

      ActionData& sd = s_pWidget->m_InputActions[sFinalName.GetData()];

      plUInt8 uiKeyState = 0;
      msg.GetReader() >> uiKeyState;
      sd.m_KeyState = (plKeyState::Enum)uiKeyState;

      msg.GetReader() >> sd.m_fValue;
      msg.GetReader() >> sd.m_bUseTimeScaling;

      for (plUInt32 i = 0; i < plInputActionConfig::MaxInputSlotAlternatives; ++i)
      {
        msg.GetReader() >> sd.m_sTrigger[i];
        msg.GetReader() >> sd.m_fTriggerScaling[i];
      }

      if (sd.m_iTableRow == -1)
        bUpdateActionTable = true;

      bFillActionTable = true;
    }
  }

  if (bUpdateSlotTable)
    s_pWidget->UpdateSlotTable(true);
  else if (bFillSlotTable)
    s_pWidget->UpdateSlotTable(false);

  if (bUpdateActionTable)
    s_pWidget->UpdateActionTable(true);
  else if (bFillActionTable)
    s_pWidget->UpdateActionTable(false);
}

void plQtInputWidget::UpdateSlotTable(bool bRecreate)
{
  plQtScopedUpdatesDisabled _1(TableInputSlots);

  if (bRecreate)
  {
    TableInputSlots->clear();
    TableInputSlots->setRowCount(m_InputSlots.GetCount());

    QStringList Headers;
    Headers.append("");
    Headers.append(" Slot ");
    Headers.append(" State ");
    Headers.append(" Value ");
    Headers.append(" Dead Zone ");
    Headers.append(" Flags (Binary) ");

    TableInputSlots->setColumnCount(static_cast<int>(Headers.size()));

    TableInputSlots->setHorizontalHeaderLabels(Headers);
    TableInputSlots->horizontalHeader()->show();

    plStringBuilder sTemp;

    plInt32 iRow = 0;
    for (plMap<plString, SlotData>::Iterator it = m_InputSlots.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      sTemp.Format("  {0}  ", it.Key());

      QLabel* pIcon = new QLabel();
      QIcon icon = plQtUiServices::GetCachedIconResource(":/Icons/Icons/InputSlots.svg");
      pIcon->setPixmap(icon.pixmap(QSize(24, 24)));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableInputSlots->setCellWidget(iRow, 0, pIcon);

      TableInputSlots->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));
      TableInputSlots->setCellWidget(iRow, 2, new QLabel("????????????"));
      TableInputSlots->setCellWidget(iRow, 3, new QLabel("????????"));
      TableInputSlots->setCellWidget(iRow, 4, new QLabel("????????"));
      TableInputSlots->setCellWidget(iRow, 5, new QLabel("????????????????"));

      const plUInt32 uiFlags = it.Value().m_uiSlotFlags;

      // Flags
      {
        plStringBuilder sFlags;
        sFlags.Printf("  %16b  ", uiFlags);

        QLabel* pFlags = (QLabel*)TableInputSlots->cellWidget(iRow, 5);
        pFlags->setAlignment(Qt::AlignRight);
        pFlags->setText(sFlags.GetData());
      }

      // Flags Tooltip
      {
        // in VS 2012 at least the snprintf fails when "yes" and "no" are passed directly, instead of as const char* variables
        const char* szYes = "<b>yes</b>";
        const char* szNo = "no";

        plStringBuilder tt("<p>");
        tt.AppendFormat("ReportsRelativeValues: {0}<br>", (uiFlags & plInputSlotFlags::ReportsRelativeValues) ? szYes : szNo);
        tt.AppendFormat("ValueBinaryZeroOrOne: {0}<br>", (uiFlags & plInputSlotFlags::ValueBinaryZeroOrOne) ? szYes : szNo);
        tt.AppendFormat("ValueRangeZeroToOne: {0}<br>", (uiFlags & plInputSlotFlags::ValueRangeZeroToOne) ? szYes : szNo);
        tt.AppendFormat("ValueRangeZeroToInf: {0}<br>", (uiFlags & plInputSlotFlags::ValueRangeZeroToInf) ? szYes : szNo);
        tt.AppendFormat("Pressable: {0}<br>", (uiFlags & plInputSlotFlags::Pressable) ? szYes : szNo);
        tt.AppendFormat("Holdable: {0}<br>", (uiFlags & plInputSlotFlags::Holdable) ? szYes : szNo);
        tt.AppendFormat("HalfAxis: {0}<br>", (uiFlags & plInputSlotFlags::HalfAxis) ? szYes : szNo);
        tt.AppendFormat("FullAxis: {0}<br>", (uiFlags & plInputSlotFlags::FullAxis) ? szYes : szNo);
        tt.AppendFormat("RequiresDeadZone: {0}<br>", (uiFlags & plInputSlotFlags::RequiresDeadZone) ? szYes : szNo);
        tt.AppendFormat("ValuesAreNonContinuous: {0}<br>", (uiFlags & plInputSlotFlags::ValuesAreNonContinuous) ? szYes : szNo);
        tt.AppendFormat("ActivationDependsOnOthers: {0}<br>", (uiFlags & plInputSlotFlags::ActivationDependsOnOthers) ? szYes : szNo);
        tt.AppendFormat("NeverTimeScale: {0}<br>", (uiFlags & plInputSlotFlags::NeverTimeScale) ? szYes : szNo);
        tt.Append("</p>");

        TableInputSlots->cellWidget(iRow, 5)->setToolTip(tt.GetData());
      }

      ++iRow;
    }

    TableInputSlots->resizeColumnsToContents();
  }

  {
    plStringBuilder sTemp;

    plInt32 iRow = 0;
    for (plMap<plString, SlotData>::Iterator it = m_InputSlots.GetIterator(); it.IsValid(); ++it)
    {
      QLabel* pState = (QLabel*)TableInputSlots->cellWidget(iRow, 2);
      pState->setAlignment(Qt::AlignHCenter);

      switch (it.Value().m_KeyState)
      {
        case plKeyState::Down:
          pState->setText("Down");
          break;
        case plKeyState::Pressed:
          pState->setText("Pressed");
          break;
        case plKeyState::Released:
          pState->setText("Released");
          break;
        case plKeyState::Up:
          pState->setText("");
          break;
      }

      // Value
      {
        QLabel* pValue = (QLabel*)TableInputSlots->cellWidget(iRow, 3);
        pValue->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fValue == 0.0f)
          pValue->setText("");
        else
        {
          sTemp.Format(" {0} ", plArgF(it.Value().m_fValue, 4));
          pValue->setText(sTemp.GetData());
        }
      }

      // Dead-zone
      {
        QLabel* pDeadZone = (QLabel*)TableInputSlots->cellWidget(iRow, 4);
        pDeadZone->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fDeadZone == 0.0f)
          pDeadZone->setText("");
        else
          pDeadZone->setText(QString::number(it.Value().m_fDeadZone, 'f', 2));
      }

      ++iRow;
    }
  }
}

void plQtInputWidget::UpdateActionTable(bool bRecreate)
{
  plQtScopedUpdatesDisabled _1(TableInputActions);

  if (bRecreate)
  {
    TableInputActions->clear();
    TableInputActions->setRowCount(m_InputActions.GetCount());

    QStringList Headers;
    Headers.append("");
    Headers.append(" Action ");
    Headers.append(" State ");
    Headers.append(" Value ");

    for (plInt32 slot = 0; slot < plInputActionConfig::MaxInputSlotAlternatives; ++slot)
      Headers.append(QString(" Slot %1 ").arg(slot + 1));

    TableInputActions->setColumnCount(static_cast<int>(Headers.size()));

    TableInputActions->setHorizontalHeaderLabels(Headers);
    TableInputActions->horizontalHeader()->show();

    plStringBuilder sTemp;

    plInt32 iRow = 0;
    for (plMap<plString, ActionData>::Iterator it = m_InputActions.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      sTemp.Format("  {0}  ", it.Key());

      QLabel* pIcon = new QLabel();
      QIcon icon = plQtUiServices::GetCachedIconResource(":/Icons/Icons/InputActions.svg");
      pIcon->setPixmap(icon.pixmap(QSize(24, 24)));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableInputActions->setCellWidget(iRow, 0, pIcon);

      TableInputActions->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));
      TableInputActions->setCellWidget(iRow, 2, new QLabel("????????????"));
      TableInputActions->setCellWidget(iRow, 3, new QLabel("????????????????????????"));
      TableInputActions->setCellWidget(iRow, 4, new QLabel(""));
      TableInputActions->setCellWidget(iRow, 5, new QLabel(""));
      TableInputActions->setCellWidget(iRow, 6, new QLabel(""));

      // Trigger Slots

      for (plInt32 slot = 0; slot < plInputActionConfig::MaxInputSlotAlternatives; ++slot)
      {
        if (it.Value().m_sTrigger[slot].IsEmpty())
          sTemp = "  ";
        else
          sTemp.Format("  [Scale: {0}] {1}  ", plArgF(it.Value().m_fTriggerScaling[slot], 2), it.Value().m_sTrigger[slot]);

        QLabel* pValue = (QLabel*)TableInputActions->cellWidget(iRow, 4 + slot);
        pValue->setText(sTemp.GetData());
      }

      ++iRow;
    }

    TableInputActions->resizeColumnsToContents();
  }

  {
    plStringBuilder sTemp;

    plInt32 iRow = 0;
    for (plMap<plString, ActionData>::Iterator it = m_InputActions.GetIterator(); it.IsValid(); ++it)
    {
      QLabel* pState = (QLabel*)TableInputActions->cellWidget(iRow, 2);
      pState->setAlignment(Qt::AlignHCenter);

      switch (it.Value().m_KeyState)
      {
        case plKeyState::Down:
          pState->setText("Down");
          break;
        case plKeyState::Pressed:
          pState->setText("Pressed");
          break;
        case plKeyState::Released:
          pState->setText("Released");
          break;
        case plKeyState::Up:
          pState->setText("");
          break;
      }

      // Value
      {
        QLabel* pValue = (QLabel*)TableInputActions->cellWidget(iRow, 3);
        pValue->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fValue == 0.0f)
          pValue->setText("");
        else
        {
          if (it.Value().m_bUseTimeScaling)
            sTemp.Format(" {0} (Time-Scaled) ", plArgF(it.Value().m_fValue, 4));
          else
            sTemp.Format(" {0} (Absolute) ", plArgF(it.Value().m_fValue, 4));

          pValue->setText(sTemp.GetData());
        }
      }


      ++iRow;
    }
  }
}

void plQtInputWidget::on_ButtonClearSlots_clicked()
{
  ClearSlots();
}

void plQtInputWidget::on_ButtonClearActions_clicked()
{
  ClearActions();
}
