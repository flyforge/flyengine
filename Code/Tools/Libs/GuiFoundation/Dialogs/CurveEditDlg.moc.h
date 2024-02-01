#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <GuiFoundation/ui_CurveEditDlg.h>
#include <QDialog>

class plCurveGroupData;
class plObjectAccessorBase;
class plDocumentObject;

class PL_GUIFOUNDATION_DLL plQtCurveEditDlg : public QDialog, Ui_CurveEditDlg
{
  Q_OBJECT
public:
  plQtCurveEditDlg(plObjectAccessorBase* pObjectAccessor, const plDocumentObject* pCurveObject, QWidget* pParent);
  ~plQtCurveEditDlg();

  static QByteArray GetLastDialogGeometry() { return s_LastDialogGeometry; }

  void SetCurveColor(const plColor& color);
  void SetCurveExtents(double fLower, bool bLowerFixed, double fUpper, bool bUpperFixed);
  void SetCurveRanges(double fLower, double fUpper);

  virtual void reject() override;
  virtual void accept() override;

  void cancel();

Q_SIGNALS:

private Q_SLOTS:
  void OnCpMovedEvent(plUInt32 curveIdx, plUInt32 cpIdx, plInt64 iTickX, double newPosY);
  void OnCpDeletedEvent(plUInt32 curveIdx, plUInt32 cpIdx);
  void OnTangentMovedEvent(plUInt32 curveIdx, plUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void OnInsertCpEvent(plUInt32 uiCurveIdx, plInt64 tickX, double value);
  void OnTangentLinkEvent(plUInt32 curveIdx, plUInt32 cpIdx, bool bLink);
  void OnCpTangentModeEvent(plUInt32 curveIdx, plUInt32 cpIdx, bool rightTangent, int mode); // plCurveTangentMode

  void OnBeginCpChangesEvent(QString name);
  void OnEndCpChangesEvent();

  void OnBeginOperationEvent(QString name);
  void OnEndOperationEvent(bool commit);

  void on_actionUndo_triggered();
  void on_actionRedo_triggered();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();

private:
  static QByteArray s_LastDialogGeometry;

  void RetrieveCurveState();
  void UpdatePreview();

  double m_fLowerRange = -plMath::HighValue<double>();
  double m_fUpperRange = plMath::HighValue<double>();
  double m_fLowerExtents = 0.0;
  double m_fUpperExtents = 1.0;
  bool m_bLowerFixed = false;
  bool m_bUpperFixed = false;
  bool m_bCurveLengthIsFixed = false;
  plCurveGroupData m_Curves;
  plUInt32 m_uiActionsUndoBaseline = 0;

  QShortcut* m_pShortcutUndo = nullptr;
  QShortcut* m_pShortcutRedo = nullptr;

  plObjectAccessorBase* m_pObjectAccessor = nullptr;
  const plDocumentObject* m_pCurveObject = nullptr;

protected:
  virtual void closeEvent(QCloseEvent* e) override;
  virtual void showEvent(QShowEvent* e) override;
};
