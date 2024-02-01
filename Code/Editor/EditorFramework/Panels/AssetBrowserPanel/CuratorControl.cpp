#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>

plQtCuratorControl::plQtCuratorControl(QWidget* pParent)
  : QWidget(pParent)

{
  QHBoxLayout* pLayout = new QHBoxLayout();
  setLayout(pLayout);
  layout()->setContentsMargins(0, 0, 0, 0);
  m_pBackgroundProcess = new QToolButton(this);
  pLayout->addWidget(m_pBackgroundProcess);
  connect(m_pBackgroundProcess, &QAbstractButton::clicked, this, &plQtCuratorControl::BackgroundProcessClicked);
  pLayout->addSpacing(200);

  UpdateBackgroundProcessState();
  plAssetCurator::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plQtCuratorControl::AssetCuratorEvents, this));
  plAssetProcessor::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plQtCuratorControl::AssetProcessorEvents, this));
  plToolsProject::GetSingleton()->s_Events.AddEventHandler(plMakeDelegate(&plQtCuratorControl::ProjectEvents, this));
}

plQtCuratorControl::~plQtCuratorControl()
{
  plAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtCuratorControl::AssetCuratorEvents, this));
  plAssetProcessor::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtCuratorControl::AssetProcessorEvents, this));
  plToolsProject::GetSingleton()->s_Events.RemoveEventHandler(plMakeDelegate(&plQtCuratorControl::ProjectEvents, this));
}

void plQtCuratorControl::paintEvent(QPaintEvent* e)
{
  QRect rect = contentsRect();
  QRect rectButton = m_pBackgroundProcess->geometry();
  rect.setLeft(rectButton.right());

  QPainter painter(this);
  painter.setPen(QPen(Qt::NoPen));

  plUInt32 uiNumAssets;
  plHybridArray<plUInt32, plAssetInfo::TransformState::COUNT> sections;
  plAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);
  QColor colors[plAssetInfo::TransformState::COUNT];
  colors[plAssetInfo::TransformState::Unknown] = plToQtColor(plColorScheme::DarkUI(plColorScheme::Gray));
  colors[plAssetInfo::TransformState::NeedsImport] = plToQtColor(plColorScheme::DarkUI(plColorScheme::Yellow));
  colors[plAssetInfo::TransformState::NeedsTransform] = plToQtColor(plColorScheme::DarkUI(plColorScheme::Blue));
  colors[plAssetInfo::TransformState::NeedsThumbnail] = plToQtColor(plColorScheme::DarkUI(float(plColorScheme::Blue + plColorScheme::Green) * 0.5f * plColorScheme::s_fIndexNormalizer));
  colors[plAssetInfo::TransformState::UpToDate] = plToQtColor(plColorScheme::DarkUI(plColorScheme::Green));
  colors[plAssetInfo::TransformState::MissingTransformDependency] = plToQtColor(plColorScheme::DarkUI(plColorScheme::Red));
  colors[plAssetInfo::TransformState::MissingThumbnailDependency] = plToQtColor(plColorScheme::DarkUI(plColorScheme::Orange));
  colors[plAssetInfo::TransformState::CircularDependency] = plToQtColor(plColorScheme::DarkUI(plColorScheme::Red));
  colors[plAssetInfo::TransformState::TransformError] = plToQtColor(plColorScheme::DarkUI(plColorScheme::Red));

  const float fTotalCount = uiNumAssets;
  const plInt32 iTargetWidth = rect.width();
  plInt32 iCurrentCount = 0;
  for (plInt32 i = 0; i < plAssetInfo::TransformState::COUNT; ++i)
  {
    plInt32 iStartX = plInt32((iCurrentCount / fTotalCount) * iTargetWidth);
    iCurrentCount += sections[i];
    plInt32 iEndX = plInt32((iCurrentCount / fTotalCount) * iTargetWidth);

    if (sections[i])
    {
      QRect area = rect;
      area.setLeft(rect.left() + iStartX);
      area.setRight(rect.left() + iEndX);
      painter.setBrush(QBrush(colors[i]));
      painter.drawRect(area);
    }
  }

  plStringBuilder s;
  s.SetFormat("[Un: {0}, Imp: {4}, Tr: {1}, Th: {2}, Err: {3}]", sections[plAssetInfo::TransformState::Unknown],
    sections[plAssetInfo::TransformState::NeedsTransform], sections[plAssetInfo::TransformState::NeedsThumbnail],
    sections[plAssetInfo::TransformState::MissingTransformDependency] + sections[plAssetInfo::TransformState::MissingThumbnailDependency] +
      sections[plAssetInfo::TransformState::TransformError] + sections[plAssetInfo::TransformState::CircularDependency],
    sections[plAssetInfo::TransformState::NeedsImport]);

  painter.setPen(QPen(Qt::white));
  painter.drawText(rect, s.GetData(), QTextOption(Qt::AlignCenter));
}

void plQtCuratorControl::UpdateBackgroundProcessState()
{
  plAssetProcessor::ProcessTaskState state = plAssetProcessor::GetSingleton()->GetProcessTaskState();
  switch (state)
  {
    case plAssetProcessor::ProcessTaskState::Stopped:
      m_pBackgroundProcess->setToolTip("Start background asset processing");
      m_pBackgroundProcess->setIcon(plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingStart.svg"));
      break;
    case plAssetProcessor::ProcessTaskState::Running:
      m_pBackgroundProcess->setToolTip("Stop background asset processing");
      m_pBackgroundProcess->setIcon(plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingPause.svg"));
      break;
    case plAssetProcessor::ProcessTaskState::Stopping:
      m_pBackgroundProcess->setToolTip("Force stop background asset processing");
      m_pBackgroundProcess->setIcon(plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingForceStop.svg"));
      break;
    default:
      break;
  }

  m_pBackgroundProcess->setCheckable(true);
  m_pBackgroundProcess->setChecked(state == plAssetProcessor::ProcessTaskState::Running);
}

void plQtCuratorControl::BackgroundProcessClicked(bool checked)
{
  plAssetProcessor::ProcessTaskState state = plAssetProcessor::GetSingleton()->GetProcessTaskState();

  if (state == plAssetProcessor::ProcessTaskState::Stopped)
  {
    plAssetCurator::GetSingleton()->CheckFileSystem();
    plAssetProcessor::GetSingleton()->StartProcessTask();
  }
  else
  {
    bool bForce = state == plAssetProcessor::ProcessTaskState::Stopping;
    plAssetProcessor::GetSingleton()->StopProcessTask(bForce);
  }
}

void plQtCuratorControl::SlotUpdateTransformStats()
{
  m_bScheduled = false;

  plUInt32 uiNumAssets;
  plHybridArray<plUInt32, plAssetInfo::TransformState::COUNT> sections;
  plAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

  plStringBuilder s;

  if (uiNumAssets > 0)
  {
    s.SetFormat("Unknown: {0}\nImport Needed: {1}\nTransform Needed: {2}\nThumbnail Needed: {3}\nMissing Dependency: {4}\nMissing Reference: {5}\nCircular Dependency: {6}\nFailed Transform: {7}",
      sections[plAssetInfo::TransformState::Unknown],
      sections[plAssetInfo::TransformState::NeedsImport],
      sections[plAssetInfo::TransformState::NeedsTransform],
      sections[plAssetInfo::TransformState::NeedsThumbnail],
      sections[plAssetInfo::TransformState::MissingTransformDependency],
      sections[plAssetInfo::TransformState::MissingThumbnailDependency],
      sections[plAssetInfo::TransformState::CircularDependency],
      sections[plAssetInfo::TransformState::TransformError]);
    setToolTip(s.GetData());
  }
  else
  {
    setToolTip("");
  }
  update();
}

void plQtCuratorControl::ScheduleUpdateTransformStats()
{
  if (m_bScheduled)
    return;

  m_bScheduled = true;

  QTimer::singleShot(200, this, SLOT(SlotUpdateTransformStats()));
}

void plQtCuratorControl::AssetCuratorEvents(const plAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case plAssetCuratorEvent::Type::AssetUpdated:
      ScheduleUpdateTransformStats();
      break;
    default:
      break;
  }
}

void plQtCuratorControl::AssetProcessorEvents(const plAssetProcessorEvent& e)
{
  switch (e.m_Type)
  {
    case plAssetProcessorEvent::Type::ProcessTaskStateChanged:
    {
      QMetaObject::invokeMethod(this, "UpdateBackgroundProcessState", Qt::QueuedConnection);
    }
    break;
    default:
      break;
  }
}

void plQtCuratorControl::ProjectEvents(const plToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case plToolsProjectEvent::Type::ProjectClosing:
    case plToolsProjectEvent::Type::ProjectClosed:
    case plToolsProjectEvent::Type::ProjectOpened:
      ScheduleUpdateTransformStats();
      break;

    default:
      break;
  }
}
