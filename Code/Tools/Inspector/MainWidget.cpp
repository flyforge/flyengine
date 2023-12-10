#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Inspector/MainWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <QDir>
#include <QMenu>
#include <QSettings>
#include <QStandardPaths>

plQtMainWidget* plQtMainWidget::s_pWidget = nullptr;

plQtMainWidget::plQtMainWidget(QWidget* pParent)
  : ads::CDockWidget("Main", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(MainWidgetFrame);

  this->setFeature(ads::CDockWidget::DockWidgetClosable, false);

  m_uiMaxStatSamples = 20000; // should be enough for 5 minutes of history at 60 Hz

  setContextMenuPolicy(Qt::NoContextMenu);

  TreeStats->setContextMenuPolicy(Qt::CustomContextMenu);

  ResetStats();

  LoadFavorites();

  QSettings Settings;
  Settings.beginGroup("MainWidget");

  splitter->restoreState(Settings.value("SplitterState", splitter->saveState()).toByteArray());
  splitter->restoreGeometry(Settings.value("SplitterSize", splitter->saveGeometry()).toByteArray());

  Settings.endGroup();
}

plQtMainWidget::~plQtMainWidget()
{
  SaveFavorites();
}

void plQtMainWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  plTelemetryMessage Msg;

  while (plTelemetry::RetrieveMessage('STAT', Msg) == PLASMA_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
      case ' DEL':
      {
        plString sStatName;
        Msg.GetReader() >> sStatName;

        plMap<plString, StatData>::Iterator it = s_pWidget->m_Stats.Find(sStatName);

        if (!it.IsValid())
          break;

        if (it.Value().m_pItem)
          delete it.Value().m_pItem;

        if (it.Value().m_pItemFavorite)
          delete it.Value().m_pItemFavorite;

        s_pWidget->m_Stats.Remove(it);
      }
      break;

      case ' SET':
      {
        plString sStatName;
        Msg.GetReader() >> sStatName;

        StatData& sd = s_pWidget->m_Stats[sStatName];

        Msg.GetReader() >> sd.m_Value;

        StatSample ss;
        ss.m_Value = sd.m_Value.ConvertTo<double>();
        Msg.GetReader() >> ss.m_AtGlobalTime;

        sd.m_History.PushBack(ss);

        s_pWidget->m_MaxGlobalTime = plMath::Max(s_pWidget->m_MaxGlobalTime, ss.m_AtGlobalTime);

        // remove excess samples
        if (sd.m_History.GetCount() > s_pWidget->m_uiMaxStatSamples)
          sd.m_History.PopFront(sd.m_History.GetCount() - s_pWidget->m_uiMaxStatSamples);

        if (sd.m_pItem == nullptr)
        {
          sd.m_pItem = s_pWidget->CreateStat(sStatName.GetData(), false);

          if (s_pWidget->m_Favorites.Find(sStatName).IsValid())
            sd.m_pItem->setCheckState(0, Qt::Checked);
        }

        const plString sValue = sd.m_Value.ConvertTo<plString>();
        sd.m_pItem->setData(1, Qt::DisplayRole, sValue.GetData());

        if (sd.m_pItemFavorite)
          sd.m_pItemFavorite->setData(1, Qt::DisplayRole, sValue.GetData());
      }
      break;
    }
  }
}

void plQtMainWidget::on_ButtonConnect_clicked()
{
  QSettings Settings;
  const QString sServer = Settings.value("LastConnection", QLatin1String("localhost:1040")).toString();

  bool bOk = false;
  QString sRes = QInputDialog::getText(this, "Host", "Host Name or IP Address:\nDefault is 'localhost:1040'", QLineEdit::Normal, sServer, &bOk);

  if (!bOk)
    return;

  Settings.setValue("LastConnection", sRes);

  if (plTelemetry::ConnectToServer(sRes.toUtf8().data()) == PLASMA_SUCCESS)
  {
  }
}

void plQtMainWidget::SaveFavorites()
{
  QString sFile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  QDir dir;
  dir.mkpath(sFile);

  sFile.append("/Favourites.stats");

  QFile f(sFile);
  if (!f.open(QIODevice::WriteOnly))
    return;

  QDataStream stream(&f);

  const plUInt32 uiNumFavorites = m_Favorites.GetCount();
  stream << uiNumFavorites;

  for (plSet<plString>::Iterator it = m_Favorites.GetIterator(); it.IsValid(); ++it)
  {
    const QString s = it.Key().GetData();
    stream << s;
  }

  f.close();
}

void plQtMainWidget::LoadFavorites()
{
  QString sFile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  QDir dir;
  dir.mkpath(sFile);
  sFile.append("/Favourites.stats");

  QFile f(sFile);
  if (!f.open(QIODevice::ReadOnly))
    return;

  m_Favorites.Clear();

  QDataStream stream(&f);

  plUInt32 uiNumFavorites = 0;
  stream >> uiNumFavorites;

  for (plUInt32 i = 0; i < uiNumFavorites; ++i)
  {
    QString s;
    stream >> s;

    plString pls = s.toUtf8().data();

    m_Favorites.Insert(pls);
  }

  f.close();
}

void plQtMainWidget::ResetStats()
{
  m_Stats.Clear();
  TreeStats->clear();
  TreeFavorites->clear();
}

void plQtMainWidget::UpdateStats()
{
  static bool bWasConnected = false;
  const bool bIsConnected = plTelemetry::IsConnectedToServer();

  if (bIsConnected)
    LabelPing->setText(QString::fromUtf8("<p>Ping: %1ms</p>").arg((plUInt32)plTelemetry::GetPingToServer().GetMilliseconds()));

  if (bWasConnected == bIsConnected)
    return;

  bWasConnected = bIsConnected;

  if (!bIsConnected)
  {
    LabelPing->setText("<p>Ping: N/A</p>");
    LabelStatus->setText(
      "<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#ff0000;\">Not Connected</span></p>");
    LabelServer->setText("<p>Server: N/A</p>");
  }
  else
  {
    plStringBuilder tmp;

    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#00aa00;\">Connected</span></p>");
    LabelServer->setText(QString::fromUtf8("<p>Server: %1:%2</p>").arg(plTelemetry::GetServerIP().GetData(tmp)).arg(plTelemetry::s_uiPort));
  }
}


void plQtMainWidget::closeEvent(QCloseEvent* pEvent)
{
  QSettings Settings;

  Settings.beginGroup("MainWidget");

  Settings.setValue("SplitterState", splitter->saveState());
  Settings.setValue("SplitterGeometry", splitter->saveGeometry());

  Settings.endGroup();
}

QTreeWidgetItem* plQtMainWidget::CreateStat(plStringView sPath, bool bParent)
{
  plStringBuilder sCleanPath = sPath;
  if (sCleanPath.EndsWith("/"))
    sCleanPath.Shrink(0, 1);

  plMap<plString, StatData>::Iterator it = m_Stats.Find(sCleanPath.GetData());

  if (it.IsValid() && it.Value().m_pItem != nullptr)
    return it.Value().m_pItem;

  QTreeWidgetItem* pParent = nullptr;
  StatData& sd = m_Stats[sCleanPath.GetData()];

  {
    plStringBuilder sParentPath = sCleanPath.GetData();
    sParentPath.PathParentDirectory(1);

    sd.m_pItem = new QTreeWidgetItem();
    sd.m_pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | (bParent ? Qt::NoItemFlags : Qt::ItemIsUserCheckable));
    sd.m_pItem->setData(0, Qt::UserRole, QString(sCleanPath.GetData()));

    if (bParent)
      sd.m_pItem->setIcon(0, QIcon(":/Icons/Icons/StatGroup.svg"));
    else
      sd.m_pItem->setIcon(0, QIcon(":/Icons/Icons/Stat.svg"));

    if (!bParent)
      sd.m_pItem->setCheckState(0, Qt::Unchecked);

    if (!sParentPath.IsEmpty())
    {
      pParent = CreateStat(sParentPath.GetData(), true);
      pParent->addChild(sd.m_pItem);
      pParent->setExpanded(false);
    }
    else
    {
      TreeStats->addTopLevelItem(sd.m_pItem);
    }
  }

  {
    plString sFileName = sCleanPath.GetFileName();
    sd.m_pItem->setData(0, Qt::DisplayRole, sFileName.GetData());

    if (pParent)
      pParent->sortChildren(0, Qt::AscendingOrder);
    else
      TreeStats->sortByColumn(0, Qt::AscendingOrder);

    TreeStats->resizeColumnToContents(0);
  }

  return sd.m_pItem;
}

void plQtMainWidget::SetFavorite(const plString& sStat, bool bFavorite)
{
  StatData& sd = m_Stats[sStat];

  if (bFavorite)
  {
    m_Favorites.Insert(sStat);

    if (!sd.m_pItemFavorite)
    {
      sd.m_pItemFavorite = new QTreeWidgetItem();
      TreeFavorites->addTopLevelItem(sd.m_pItemFavorite);
      sd.m_pItemFavorite->setData(0, Qt::DisplayRole, sStat.GetData());
      sd.m_pItemFavorite->setData(1, Qt::DisplayRole, sd.m_Value.ConvertTo<plString>().GetData());
      sd.m_pItemFavorite->setIcon(0, QIcon(":/Icons/Icons/StatFavorite.svg"));

      TreeFavorites->resizeColumnToContents(0);
    }
  }
  else
  {
    if (sd.m_pItemFavorite)
    {
      m_Favorites.Remove(sStat);

      delete sd.m_pItemFavorite;
      sd.m_pItemFavorite = nullptr;
    }
  }
}

void plQtMainWidget::on_TreeStats_itemChanged(QTreeWidgetItem* item, int column)
{
  if (column == 0)
  {
    plString sPath = item->data(0, Qt::UserRole).toString().toUtf8().data();

    SetFavorite(sPath, (item->checkState(0) == Qt::Checked));
  }
}

void plQtMainWidget::on_TreeStats_customContextMenuRequested(const QPoint& p)
{
  if (!TreeStats->currentItem())
    return;

  QMenu mSub;
  mSub.setTitle("Show in");
  mSub.setIcon(QIcon(":/Icons/Icons/StatHistory.svg"));

  QMenu m;
  m.addMenu(&mSub);

  for (plInt32 i = 0; i < 10; ++i)
  {
    plQtMainWindow::s_pWidget->m_pActionShowStatIn[i]->setText(plQtMainWindow::s_pWidget->m_pStatHistoryWidgets[i]->LineName->text());
    mSub.addAction(plQtMainWindow::s_pWidget->m_pActionShowStatIn[i]);
  }

  if (TreeStats->currentItem()->childCount() > 0)
    mSub.setEnabled(false);

  m.exec(TreeStats->viewport()->mapToGlobal(p));
}


void plQtMainWidget::ShowStatIn(bool)
{
  if (!TreeStats->currentItem())
    return;

  QAction* pAction = (QAction*)sender();

  plInt32 iHistoryWidget = 0;
  for (iHistoryWidget = 0; iHistoryWidget < 10; ++iHistoryWidget)
  {
    if (plQtMainWindow::s_pWidget->m_pActionShowStatIn[iHistoryWidget] == pAction)
      goto found;
  }

  return;

found:

  plString sPath = TreeStats->currentItem()->data(0, Qt::UserRole).toString().toUtf8().data();

  plQtMainWindow::s_pWidget->m_pStatHistoryWidgets[iHistoryWidget]->AddStat(sPath);
}
