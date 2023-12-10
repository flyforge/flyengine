#pragma once

#include <Foundation/Math/CurveFunctions.h>
#include <Foundation/Tracks/Curve1D.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_Curve1DEditorWidget.h>

#include <QWidget>

class PLASMA_GUIFOUNDATION_DLL plQtCurve1DEditorWidget : public QWidget, public Ui_Curve1DEditorWidget
{
  Q_OBJECT

public:
  explicit plQtCurve1DEditorWidget(QWidget* pParent);
  ~plQtCurve1DEditorWidget();

  void SetCurveExtents(double fLowerBound, double fUpperBound, bool bLowerIsFixed, bool bUpperIsFixed);
  void SetCurveRanges(double fLowerRange, double fUpperRange);

  void SetCurves(const plCurveGroupData& curveData);
  void SetScrubberPosition(plUInt64 uiTick);
  void ClearSelection();

  void FrameCurve();
  void FrameSelection();
  void MakeRepeatable(bool bAdjustLastPoint);
  void NormalizeCurveX(plUInt32 uiActiveCurve);
  void NormalizeCurveY(plUInt32 uiActiveCurve);
  void ClearAllPoints();
  void MirrorHorizontally(plUInt32 uiActiveCurve);
  void MirrorVertically(plUInt32 uiActiveCurve);

Q_SIGNALS:
  void CpMovedEvent(plUInt32 uiCurveIdx, plUInt32 uiIdx, plInt64 iTickX, double fNewPosY);
  void CpDeletedEvent(plUInt32 uiCurveIdx, plUInt32 uiIdx);
  void TangentMovedEvent(plUInt32 uiCurveIdx, plUInt32 uiIdx, float fNewPosX, float fNewPosY, bool bRightTangent);
  void InsertCpEvent(plUInt32 uiCurveIdx, plInt64 iTickX, double value);
  void TangentLinkEvent(plUInt32 uiCurveIdx, plUInt32 uiIdx, bool bLink);
  void CpTangentModeEvent(plUInt32 uiCurveIdx, plUInt32 uiIdx, bool bRightTangent, int iMode); // plCurveTangentMode

  void BeginCpChangesEvent(QString sName);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString sName);
  void EndOperationEvent(bool bCommit);

private Q_SLOTS:
  void on_LinePosition_editingFinished();
  void on_LineValue_editingFinished();
  void onDeleteControlPoints();
  void onDoubleClick(const QPointF& scenePos, const QPointF& epsilon);
  void onMoveControlPoints(double x, double y);
  void onMoveTangents(float x, float y);
  void onBeginOperation(QString name);
  void onEndOperation(bool commit);
  void onScaleControlPoints(QPointF refPt, double scaleX, double scaleY);
  void onContextMenu(QPoint pos, QPointF scenePos);
  void onAddPoint();
  void onLinkTangents();
  void onBreakTangents();
  void onFlattenTangents();
  void onSelectionChanged();
  void onMoveCurve(plInt32 iCurve, double moveY);
  void onGenerateCurve(plCurveFunction::Enum function, bool inverse);
  void onSaveAsPreset();
  void onLoadPreset();

private:
  void InsertCpAt(double posX, double value, plVec2d epsilon);
  bool PickCurveAt(double x, double y, double fMaxDistanceY, plInt32& out_iCurveIdx, double& out_ValueY) const;
  bool PickControlPointAt(double x, double y, plVec2d vMaxDistance, plInt32& out_iCurveIdx, plInt32& out_iCpIdx) const;
  void UpdateSpinBoxes();
  void SetTangentMode(plCurveTangentMode::Enum mode, bool bLeft, bool bRight);
  void ClampPoint(double& x, double& y) const;
  void SaveCurvePreset(const char* szFile) const;
  plResult LoadCurvePreset(const char* szFile);
  void FindAllPresets();

  double m_fCurveDuration;
  plVec2 m_vTangentMove;
  plVec2d m_vControlPointMove;
  plCurveGroupData m_Curves;
  plCurveGroupData m_CurvesBackup;
  QPointF m_ContextMenuScenePos;

  static plDynamicArray<plString> s_CurvePresets;
};
