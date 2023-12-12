#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_EventTrackEditorWidget.h>

#include <QWidget>

class PLASMA_GUIFOUNDATION_DLL plQtEventTrackEditorWidget : public QWidget, public Ui_EventTrackEditorWidget
{
  Q_OBJECT

public:
  explicit plQtEventTrackEditorWidget(QWidget* pParent);
  ~plQtEventTrackEditorWidget();

  void SetData(const plEventTrackData& data, double fMinCurveLength);
  void SetScrubberPosition(plUInt64 uiTick);
  void SetScrubberPosition(plTime time);
  void ClearSelection();

  void FrameCurve();

Q_SIGNALS:
  void CpMovedEvent(plUInt32 cpIdx, plInt64 iTickX);
  void CpDeletedEvent(plUInt32 cpIdx);
  void InsertCpEvent(plInt64 tickX, const char* value);

  void BeginCpChangesEvent(QString name);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString name);
  void EndOperationEvent(bool commit);

private Q_SLOTS:
  void on_LinePosition_editingFinished();
  void on_AddEventButton_clicked();
  void onDeleteControlPoints();
  void onDoubleClick(double scenePosX, double epsilon);
  void onMoveControlPoints(double x);
  void onBeginOperation(QString name);
  void onEndOperation(bool commit);
  // void onScaleControlPoints(QPointF refPt, double scaleX);
  void onContextMenu(QPoint pos, QPointF scenePos);
  void onAddPoint();
  void onSelectionChanged();

private:
  void InsertCpAt(double posX, double epsilon);
  void UpdateSpinBoxes();
  void DetermineAvailableEvents();
  void FillEventComboBox(const char* szCurrent = nullptr);

  const plEventTrackData* m_pData = nullptr;
  plEventTrackData m_DataCopy;

  double m_fControlPointMove;
  QPointF m_ContextMenuScenePos;
  plEventSet m_EventSet;
};

