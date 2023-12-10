#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtCurve1DEditorWidget;

class plQtCurve1DAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtCurve1DAssetDocumentWindow(plDocument* pDocument);
  ~plQtCurve1DAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "Curve1DAsset"; }

private Q_SLOTS:
  void onInsertCpAt(plUInt32 uiCurveIdx, plInt64 tickX, double newPosY);
  void onCurveCpMoved(plUInt32 curveIdx, plUInt32 cpIdx, plInt64 iTickX, double newPosY);
  void onCurveCpDeleted(plUInt32 curveIdx, plUInt32 cpIdx);
  void onCurveTangentMoved(plUInt32 curveIdx, plUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void onLinkCurveTangents(plUInt32 curveIdx, plUInt32 cpIdx, bool bLink);
  void onCurveTangentModeChanged(plUInt32 curveIdx, plUInt32 cpIdx, bool rightTangent, int mode);

  void onCurveBeginOperation(QString name);
  void onCurveEndOperation(bool commit);
  void onCurveBeginCpChanges(QString name);
  void onCurveEndCpChanges();

private:
  void UpdatePreview();

  void SendLiveResourcePreview();
  void RestoreResource();

  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const plDocumentObjectStructureEvent& e);

  plQtCurve1DEditorWidget* m_pCurveEditor;
};

