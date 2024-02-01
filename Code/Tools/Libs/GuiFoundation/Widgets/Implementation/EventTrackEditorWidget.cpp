#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <QGraphicsItem>
#include <QGraphicsSceneEvent>
#include <QInputDialog>
#include <QMenu>
#include <QPainterPath>

plQtEventTrackEditorWidget::plQtEventTrackEditorWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  EventTrackEdit->SetGridBarWidget(GridBarWidget);

  connect(EventTrackEdit, &plQtEventTrackWidget::DeleteControlPointsEvent, this, &plQtEventTrackEditorWidget::onDeleteControlPoints);
  connect(EventTrackEdit, &plQtEventTrackWidget::DoubleClickEvent, this, &plQtEventTrackEditorWidget::onDoubleClick);
  connect(EventTrackEdit, &plQtEventTrackWidget::MoveControlPointsEvent, this, &plQtEventTrackEditorWidget::onMoveControlPoints);
  connect(EventTrackEdit, &plQtEventTrackWidget::BeginOperationEvent, this, &plQtEventTrackEditorWidget::onBeginOperation);
  connect(EventTrackEdit, &plQtEventTrackWidget::EndOperationEvent, this, &plQtEventTrackEditorWidget::onEndOperation);
  // connect(EventTrackEdit, &plQtEventTrackWidget::ScaleControlPointsEvent, this, &plQtEventTrackEditorWidget::onScaleControlPoints);
  connect(EventTrackEdit, &plQtEventTrackWidget::ContextMenuEvent, this, &plQtEventTrackEditorWidget::onContextMenu);
  connect(EventTrackEdit, &plQtEventTrackWidget::SelectionChangedEvent, this, &plQtEventTrackEditorWidget::onSelectionChanged);

  LinePosition->setEnabled(false);

  DetermineAvailableEvents();
}

plQtEventTrackEditorWidget::~plQtEventTrackEditorWidget() = default;

void plQtEventTrackEditorWidget::SetData(const plEventTrackData& trackData, double fMinCurveLength)
{
  plQtScopedUpdatesDisabled ud(this);
  plQtScopedBlockSignals bs(this);

  m_pData = &trackData;
  EventTrackEdit->SetData(&trackData, fMinCurveLength);

  UpdateSpinBoxes();
}

void plQtEventTrackEditorWidget::SetScrubberPosition(plUInt64 uiTick)
{
  EventTrackEdit->SetScrubberPosition(uiTick / 4800.0);
}

void plQtEventTrackEditorWidget::SetScrubberPosition(plTime time)
{
  EventTrackEdit->SetScrubberPosition(time.GetSeconds());
}

void plQtEventTrackEditorWidget::ClearSelection()
{
  EventTrackEdit->ClearSelection();
}

void plQtEventTrackEditorWidget::FrameCurve()
{
  EventTrackEdit->FrameCurve();
}

void plQtEventTrackEditorWidget::on_AddEventButton_clicked()
{
  QString name = QInputDialog::getText(this, "Add Type", "Event Type Name:");

  m_EventSet.AddAvailableEvent(name.toUtf8().data());

  if (m_EventSet.IsModified())
  {
    m_EventSet.WriteToDDL(":project/Editor/Events.ddl").IgnoreResult();

    FillEventComboBox(name.toUtf8().data());
  }
}

void plQtEventTrackEditorWidget::onDeleteControlPoints()
{
  plHybridArray<plUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (selection.IsEmpty())
    return;

  EventTrackEdit->ClearSelection();

  Q_EMIT BeginCpChangesEvent("Delete Events");

  selection.Sort([](plUInt32 lhs, plUInt32 rhs) -> bool { return lhs > rhs; });

  // delete sorted from back to front to prevent point indices becoming invalidated
  for (plUInt32 pt : selection)
  {
    Q_EMIT CpDeletedEvent(pt);
  }

  Q_EMIT EndCpChangesEvent();
}

void plQtEventTrackEditorWidget::onDoubleClick(double scenePosX, double epsilon)
{
  InsertCpAt(scenePosX, plMath::Abs(epsilon));
}

void plQtEventTrackEditorWidget::onMoveControlPoints(double x)
{
  m_fControlPointMove += x;

  plHybridArray<plUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Move Events");

  for (const auto& cpSel : selection)
  {
    auto& cp = m_DataCopy.m_ControlPoints[cpSel];

    double newPos = cp.GetTickAsTime().GetSeconds() + m_fControlPointMove;
    newPos = plMath::Max(newPos, 0.0);

    Q_EMIT CpMovedEvent(cpSel, m_pData->TickFromTime(plTime::MakeFromSeconds(newPos)));
  }

  Q_EMIT EndCpChangesEvent();
}

// void plQtEventTrackEditorWidget::onScaleControlPoints(QPointF refPt, double scaleX, double scaleY)
//{
//  const auto selection = EventTrackEdit->GetSelection();
//
//  if (selection.IsEmpty())
//    return;
//
//  const plVec2d ref(refPt.x(), refPt.y());
//  const plVec2d scale(scaleX, scaleY);
//
//  Q_EMIT BeginCpChangesEvent("Scale Points");
//
//  for (const auto& cpSel : selection)
//  {
//    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
//    plVec2d newPos = ref + (plVec2d(cp.GetTickAsTime(), cp.m_fValue) - ref).CompMul(scale);
//    newPos.x = plMath::Max(newPos.x, 0.0);
//    newPos.y = plMath::Clamp(newPos.y, -100000.0, +100000.0);
//
//    Q_EMIT CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(newPos.x), newPos.y);
//  }
//
//  Q_EMIT EndCpChangesEvent();
//}

void plQtEventTrackEditorWidget::onBeginOperation(QString name)
{
  m_fControlPointMove = 0;
  m_DataCopy = *m_pData;

  Q_EMIT BeginOperationEvent(name);
}

void plQtEventTrackEditorWidget::onEndOperation(bool commit)
{
  Q_EMIT EndOperationEvent(commit);
}

void plQtEventTrackEditorWidget::onContextMenu(QPoint pos, QPointF scenePos)
{
  m_ContextMenuScenePos = scenePos;

  QMenu m(this);
  m.setDefaultAction(m.addAction("Add Event", this, SLOT(onAddPoint())));

  plHybridArray<plUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (!selection.IsEmpty())
  {
    m.addAction("Delete Events", QKeySequence(Qt::Key_Delete), this, SLOT(onDeleteControlPoints()));
  }

  m.addSeparator();

  m.addAction("Frame", QKeySequence(Qt::ControlModifier | Qt::Key_F), this, [this]() { FrameCurve(); });

  m.exec(pos);
}

void plQtEventTrackEditorWidget::onAddPoint()
{
  InsertCpAt(m_ContextMenuScenePos.x(), 0.0f);
}

void plQtEventTrackEditorWidget::InsertCpAt(double posX, double epsilon)
{
  int curveIdx = 0, cpIdx = 0;
  posX = plMath::Max(posX, 0.0);

  Q_EMIT InsertCpEvent(m_pData->TickFromTime(plTime::MakeFromSeconds(posX)), ComboType->currentText().toUtf8().data());
}

void plQtEventTrackEditorWidget::onSelectionChanged()
{
  UpdateSpinBoxes();
}

void plQtEventTrackEditorWidget::UpdateSpinBoxes()
{
  plHybridArray<plUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  plQtScopedBlockSignals _1(LinePosition, SelectedTypeLabel);

  if (selection.IsEmpty())
  {
    LinePosition->setText(QString());
    LinePosition->setEnabled(false);
    SelectedTypeLabel->setText("Event: none");
    return;
  }

  const double fPos = m_pData->m_ControlPoints[selection[0]].GetTickAsTime().GetSeconds();

  LinePosition->setEnabled(true);

  plStringBuilder labelText("Event: ", m_pData->m_ControlPoints[selection[0]].m_sEvent.GetString());

  bool bMultipleTicks = false;
  for (plUInt32 i = 1; i < selection.GetCount(); ++i)
  {
    const plString& sName = m_pData->m_ControlPoints[selection[i]].m_sEvent.GetString();
    const double fPos2 = m_pData->m_ControlPoints[selection[i]].GetTickAsTime().GetSeconds();

    if (!labelText.FindSubString(sName))
    {
      labelText.Append(", ", sName);
    }

    if (fPos2 != fPos)
    {
      bMultipleTicks = true;
      break;
    }
  }

  LinePosition->setText(bMultipleTicks ? QString() : QString::number(fPos, 'f', 2));
  SelectedTypeLabel->setText(labelText.GetData());
}

void plQtEventTrackEditorWidget::DetermineAvailableEvents()
{
  m_EventSet.ReadFromDDL(":project/Editor/Events.ddl").IgnoreResult();

  FillEventComboBox(nullptr);
}

void plQtEventTrackEditorWidget::FillEventComboBox(const char* szCurrent)
{
  QString prev = szCurrent;

  if (prev.isEmpty())
    prev = ComboType->currentText();

  ComboType->clear();

  for (const plString& type : m_EventSet.GetAvailableEvents())
  {
    ComboType->addItem(type.GetData());
  }

  ComboType->setCurrentText(prev);
}

void plQtEventTrackEditorWidget::on_LinePosition_editingFinished()
{
  QString sValue = LinePosition->text();

  bool ok = false;
  const double value = sValue.toDouble(&ok);
  if (!ok)
    return;

  if (value < 0)
    return;

  plHybridArray<plUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);
  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Set Event Time");

  plInt64 tick = m_pData->TickFromTime(plTime::MakeFromSeconds(value));

  for (const auto& cpSel : selection)
  {
    if (m_pData->m_ControlPoints[cpSel].m_iTick != tick)
      Q_EMIT CpMovedEvent(cpSel, tick);
  }

  Q_EMIT EndCpChangesEvent();
}
