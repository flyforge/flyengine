#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Tracks/Curve1D.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

#include <QBrush>
#include <QPen>
#include <QWidget>

class plQGridBarWidget;
class QRubberBand;

class PL_GUIFOUNDATION_DLL plQtCurveEditWidget : public QWidget
{
  Q_OBJECT

public:
  plQtCurveEditWidget(QWidget* pParent);

  double m_fLowerRange = -plMath::HighValue<double>();
  double m_fUpperRange = plMath::HighValue<double>();
  double m_fLowerExtent = 0.0;
  double m_fUpperExtent = 1.0;
  bool m_bLowerExtentFixed = false;
  bool m_bUpperExtentFixed = false;

  void SetCurves(plCurveGroupData* pCurveEditData);
  void SetGridBarWidget(plQGridBarWidget* pGridBar) { m_pGridBar = pGridBar; }

  void SetScrubberPosition(double fPosition);

  double GetMinCurveExtent() const { return m_fMinExtentValue; }
  double GetMaxCurveExtent() const { return m_fMaxExtentValue; }

  void FrameCurve();
  void FrameSelection();
  void Frame(double fOffsetX, double fOffsetY, double fWidth, double fHeight);

  QPoint MapFromScene(const QPointF& pos) const;
  QPoint MapFromScene(const plVec2d& vPos) const { return MapFromScene(QPointF(vPos.x, vPos.y)); }
  QPointF MapToScene(const QPoint& pos) const;
  plVec2 MapDirFromScene(const plVec2& vPos) const;

  void ClearSelection();
  void SelectAll();
  const plDynamicArray<plSelectedCurveCP>& GetSelection() const { return m_SelectedCPs; }
  bool IsSelected(const plSelectedCurveCP& cp) const;
  void SetSelection(const plSelectedCurveCP& cp);
  void ToggleSelected(const plSelectedCurveCP& cp);
  void SetSelected(const plSelectedCurveCP& cp, bool bSet);

  bool GetSelectedTangent(plInt32& out_iCurve, plInt32& out_iPoint, bool& out_bLeftTangent) const;

Q_SIGNALS:
  void DoubleClickEvent(const QPointF& scenePos, const QPointF& epsilon);
  void DeleteControlPointsEvent();
  void MoveControlPointsEvent(double fMoveX, double fMoveY);
  void MoveTangentsEvent(double fMoveX, double fMoveY);
  void BeginOperationEvent(QString sName);
  void EndOperationEvent(bool bCommit);
  void ScaleControlPointsEvent(const QPointF& centerPos, double fScaleX, double fScaleY);
  void ContextMenuEvent(QPoint pos, QPointF scenePos);
  void SelectionChangedEvent();
  void MoveCurveEvent(plInt32 iCurve, double fMoveY);

protected:
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void keyPressEvent(QKeyEvent* e) override;

private:
  enum class ClickTarget
  {
    Nothing,
    SelectedPoint,
    TangentHandle
  };
  enum class EditState
  {
    None,
    DraggingPoints,
    DraggingPointsHorz,
    DraggingPointsVert,
    DraggingTangents,
    MultiSelect,
    RightClick,
    Panning,
    ScaleLeftRight,
    ScaleUpDown,
    DraggingCurve
  };
  enum class SelectArea
  {
    None,
    Center,
    Top,
    Bottom,
    Left,
    Right
  };

  void PaintCurveSegments(QPainter* painter, float fOffsetX, plUInt8 alpha) const;
  void PaintOutsideAreaOverlay(QPainter* painter) const;
  void PaintControlPoints(QPainter* painter) const;
  void PaintSelectedControlPoints(QPainter* painter) const;
  void PaintSelectedTangentLines(QPainter* painter) const;
  void PaintSelectedTangentHandles(QPainter* painter) const;
  void PaintMultiSelectionSquare(QPainter* painter) const;
  void PaintScrubber(QPainter& p) const;
  void RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity);
  void RenderSideLinesAndText(QPainter* painter, const QRectF& viewportSceneRect);
  void RenderValueRanges(QPainter* painter);
  QRectF ComputeViewportSceneRect() const;
  bool PickCpAt(const QPoint& pos, float fMaxPixelDistance, plSelectedCurveCP& out_Result) const;
  ClickTarget DetectClickTarget(const QPoint& pos);
  void ExecMultiSelection(plDynamicArray<plSelectedCurveCP>& out_Selection);
  bool CombineSelectionAdd(plDynamicArray<plSelectedCurveCP>& inout_Selection, const plDynamicArray<plSelectedCurveCP>& change);
  bool CombineSelectionRemove(plDynamicArray<plSelectedCurveCP>& inout_Selection, const plDynamicArray<plSelectedCurveCP>& change);
  bool CombineSelectionToggle(plDynamicArray<plSelectedCurveCP>& inout_Selection, const plDynamicArray<plSelectedCurveCP>& change);
  void ComputeSelectionRect();
  SelectArea WhereIsPoint(QPoint pos) const;
  plInt32 PickCurveAt(QPoint pos) const;
  void ClampZoomPan();

  plQGridBarWidget* m_pGridBar = nullptr;

  EditState m_State = EditState::None;
  plInt32 m_iDraggedCurve;

  plCurveGroupData* m_pCurveEditData;
  plHybridArray<plCurve1D, 4> m_Curves;
  plHybridArray<plCurve1D, 4> m_CurvesSorted;
  plHybridArray<plVec2d, 4> m_CurveExtents;
  double m_fMinExtentValue;
  double m_fMaxExtentValue;
  double m_fMinValue, m_fMaxValue;


  QPointF m_SceneTranslation;
  QPointF m_SceneToPixelScale;
  QPoint m_LastMousePos;

  QBrush m_ControlPointBrush;
  QBrush m_SelectedControlPointBrush;
  QPen m_TangentLinePen;
  QBrush m_TangentHandleBrush;

  plDynamicArray<plSelectedCurveCP> m_SelectedCPs;
  plInt32 m_iSelectedTangentCurve = -1;
  plInt32 m_iSelectedTangentPoint = -1;
  bool m_bSelectedTangentLeft = false;
  bool m_bBegunChanges = false;
  bool m_bFrameBeforePaint = true;

  QPoint m_MultiSelectionStart;
  QRect m_MultiSelectRect;
  QRectF m_SelectionBRect;
  QPointF m_ScaleReferencePoint;
  QPointF m_ScaleStartPoint;
  QPointF m_TotalPointDrag;
  QRubberBand* m_pRubberband = nullptr;

  bool m_bShowScrubber = false;
  double m_fScrubberPosition = 0;
};
