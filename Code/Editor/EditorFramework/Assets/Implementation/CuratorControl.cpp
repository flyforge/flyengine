#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Assets/CuratorControl.moc.h>

plQtCuratorControl::plQtCuratorControl(QWidget* pParent, plQtAssetCuratorPanel* pCuratorPanel, QSplitter* pCuratorSplitter)
  : QAbstractButton(pParent)
  , m_bScheduled(false)
  , m_pBackgroundProcess(nullptr)
  , m_iCuratorWidth(0)
  , m_pCuratorSplitter(pCuratorSplitter)
  , m_pCuratorPanel(pCuratorPanel)
{
  QHBoxLayout* pLayout = new QHBoxLayout();
  setLayout(pLayout);
  layout()->setContentsMargins(0, 0, 0, 0);
  m_pBackgroundProcess = new QToolButton(this);
  m_pBackgroundProcess->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
  pLayout->addWidget(m_pBackgroundProcess, 0, Qt::AlignLeft);
  connect(m_pBackgroundProcess, &QAbstractButton::clicked, this, &plQtCuratorControl::BackgroundProcessClicked);
  pLayout->addSpacing(200);

  connect(this, SIGNAL(clicked()), this, SLOT(AnimateCurator()));

  UpdateBackgroundProcessState();
  plAssetCurator::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plQtCuratorControl::AssetCuratorEvents, this));
  plAssetProcessor::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plQtCuratorControl::AssetProcessorEvents, this));
  plToolsProject::GetSingleton()->s_Events.AddEventHandler(plMakeDelegate(&plQtCuratorControl::ProjectEvents, this));

  m_pCuratorAnimation = new QPropertyAnimation(this, "curatorWidth");

  setCuratorWidth(0);
}

plQtCuratorControl::~plQtCuratorControl()
{
  plAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtCuratorControl::AssetCuratorEvents, this));
  plAssetProcessor::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtCuratorControl::AssetProcessorEvents, this));
  plToolsProject::GetSingleton()->s_Events.RemoveEventHandler(plMakeDelegate(&plQtCuratorControl::ProjectEvents, this));
}

void plQtCuratorControl::setCuratorWidth(int newWidth)
{
  m_iCuratorWidth = newWidth;
  QList<int> sizes = m_pCuratorSplitter->sizes();
  int totalWidth = m_pCuratorSplitter->width();
  sizes[2] = m_iCuratorWidth;
  m_pCuratorSplitter->setSizes(sizes);
  m_pCuratorPanel->setVisible(m_iCuratorWidth);
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
  colors[plAssetInfo::TransformState::MissingDependency] = plToQtColor(plColorScheme::DarkUI(plColorScheme::Red));
  colors[plAssetInfo::TransformState::MissingReference] = plToQtColor(plColorScheme::DarkUI(plColorScheme::Orange));
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
  s.Format("[Un: {0}, Imp: {4}, Tr: {1}, Th: {2}, Err: {3}]", sections[plAssetInfo::TransformState::Unknown],
    sections[plAssetInfo::TransformState::NeedsTransform], sections[plAssetInfo::TransformState::NeedsThumbnail],
    sections[plAssetInfo::TransformState::MissingDependency] + sections[plAssetInfo::TransformState::MissingReference] +
      sections[plAssetInfo::TransformState::TransformError],
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


void plQtCuratorControl::AnimateCurator()
{
  int width = m_pCuratorPanel->geometry().width();

  // if open
  if (width)
  {
    m_pCuratorAnimation->setStartValue(width);
    m_pCuratorAnimation->setEndValue(0);

  }
  else
  {
    m_pCuratorAnimation->setStartValue(0);
    m_pCuratorAnimation->setEndValue(500);
  }
  m_pCuratorAnimation->start(QAbstractAnimation::KeepWhenStopped);
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
    s.Format("Unknown: {0}\nImport Needed: {6}\nTransform Needed: {1}\nThumbnail Needed: {2}\nMissing Dependency: {3}\nMissing Reference: {4}\nFailed "
             "Transform: {5}",
      sections[plAssetInfo::TransformState::Unknown], sections[plAssetInfo::TransformState::NeedsTransform],
      sections[plAssetInfo::TransformState::NeedsThumbnail], sections[plAssetInfo::TransformState::MissingDependency],
      sections[plAssetInfo::TransformState::MissingReference], sections[plAssetInfo::TransformState::TransformError], sections[plAssetInfo::TransformState::NeedsImport]);
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
