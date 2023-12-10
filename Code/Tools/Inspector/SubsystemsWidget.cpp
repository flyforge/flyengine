#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/MainWindow.moc.h>

plQtSubsystemsWidget* plQtSubsystemsWidget::s_pWidget = nullptr;

plQtSubsystemsWidget::plQtSubsystemsWidget(QWidget* pParent)
  : ads::CDockWidget("Subsystem Widget", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(TableSubsystems);

  setIcon(QIcon(":/Icons/Icons/Subsystem.svg"));

  ResetStats();
}

void plQtSubsystemsWidget::ResetStats()
{
  m_bUpdateSubsystems = true;
  m_Subsystems.Clear();
}


void plQtSubsystemsWidget::UpdateStats()
{
  UpdateSubSystems();
}

void plQtSubsystemsWidget::UpdateSubSystems()
{
  if (!m_bUpdateSubsystems)
    return;

  m_bUpdateSubsystems = false;

  plQtScopedUpdatesDisabled _1(TableSubsystems);

  TableSubsystems->clear();

  TableSubsystems->setRowCount(m_Subsystems.GetCount());

  QStringList Headers;
  Headers.append("");
  Headers.append(" SubSystem ");
  Headers.append(" Plugin ");
  Headers.append(" Startup Done ");
  Headers.append(" Dependencies ");

  TableSubsystems->setColumnCount(Headers.size());

  TableSubsystems->setHorizontalHeaderLabels(Headers);

  {
    plStringBuilder sTemp;
    plInt32 iRow = 0;

    for (plMap<plString, SubsystemData>::Iterator it = m_Subsystems.GetIterator(); it.IsValid(); ++it)
    {
      const SubsystemData& ssd = it.Value();

      QLabel* pIcon = new QLabel();
      pIcon->setPixmap(plQtUiServices::GetCachedPixmapResource(":/Icons/Icons/Subsystem.svg"));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableSubsystems->setCellWidget(iRow, 0, pIcon);

      sTemp.Format("  {0}  ", it.Key());
      TableSubsystems->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));

      sTemp.Format("  {0}  ", ssd.m_sPlugin);
      TableSubsystems->setCellWidget(iRow, 2, new QLabel(sTemp.GetData()));

      if (ssd.m_bStartupDone[plStartupStage::HighLevelSystems])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#00aa00;\">  Engine  </span></p>"));
      else if (ssd.m_bStartupDone[plStartupStage::CoreSystems])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#5555ff;\">  Core  </span></p>"));
      else if (ssd.m_bStartupDone[plStartupStage::BaseSystems])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#cece00;\">  Base  </span></p>"));
      else
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#ff0000;\">Not Initialized</span></p>"));

      ((QLabel*)TableSubsystems->cellWidget(iRow, 3))->setAlignment(Qt::AlignHCenter);

      sTemp.Format("  {0}  ", ssd.m_sDependencies);
      TableSubsystems->setCellWidget(iRow, 4, new QLabel(sTemp.GetData()));

      ++iRow;
    }
  }

  TableSubsystems->resizeColumnsToContents();
}

void plQtSubsystemsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  plTelemetryMessage Msg;

  while (plTelemetry::RetrieveMessage('STRT', Msg) == PLASMA_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
      case ' CLR':
      {
        s_pWidget->m_Subsystems.Clear();
        s_pWidget->m_bUpdateSubsystems = true;
      }
      break;

      case 'SYST':
      {
        plString sGroup, sSystem;

        Msg.GetReader() >> sGroup;
        Msg.GetReader() >> sSystem;

        plStringBuilder sFinalName = sGroup.GetData();
        sFinalName.Append("::");
        sFinalName.Append(sSystem.GetData());

        SubsystemData& ssd = s_pWidget->m_Subsystems[sFinalName.GetData()];

        Msg.GetReader() >> ssd.m_sPlugin;

        for (plUInt32 i = 0; i < plStartupStage::ENUM_COUNT; ++i)
          Msg.GetReader() >> ssd.m_bStartupDone[i];

        plUInt8 uiDependencies = 0;
        Msg.GetReader() >> uiDependencies;

        plStringBuilder sAllDeps;

        plString sDep;
        for (plUInt8 i = 0; i < uiDependencies; ++i)
        {
          Msg.GetReader() >> sDep;

          if (sAllDeps.IsEmpty())
            sAllDeps = sDep.GetData();
          else
          {
            sAllDeps.Append(" | ");
            sAllDeps.Append(sDep.GetData());
          }
        }

        ssd.m_sDependencies = sAllDeps.GetData();

        s_pWidget->m_bUpdateSubsystems = true;
      }
      break;
    }
  }
}
