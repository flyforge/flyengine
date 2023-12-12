#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Dialogs/CurveEditDlg.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

QByteArray plQtCurveEditDlg::s_LastDialogGeometry;

plQtCurveEditDlg::plQtCurveEditDlg(plObjectAccessorBase* pObjectAccessor, const plDocumentObject* pCurveObject, QWidget* parent)
  : QDialog(parent)
{
  m_pObjectAccessor = pObjectAccessor;
  m_pCurveObject = pCurveObject;

  setupUi(this);

  plQtCurve1DEditorWidget* pEdit = CurveEditor;

  connect(pEdit, &plQtCurve1DEditorWidget::CpMovedEvent, this, &plQtCurveEditDlg::OnCpMovedEvent);
  connect(pEdit, &plQtCurve1DEditorWidget::CpDeletedEvent, this, &plQtCurveEditDlg::OnCpDeletedEvent);
  connect(pEdit, &plQtCurve1DEditorWidget::TangentMovedEvent, this, &plQtCurveEditDlg::OnTangentMovedEvent);
  connect(pEdit, &plQtCurve1DEditorWidget::InsertCpEvent, this, &plQtCurveEditDlg::OnInsertCpEvent);
  connect(pEdit, &plQtCurve1DEditorWidget::TangentLinkEvent, this, &plQtCurveEditDlg::OnTangentLinkEvent);
  connect(pEdit, &plQtCurve1DEditorWidget::CpTangentModeEvent, this, &plQtCurveEditDlg::OnCpTangentModeEvent);
  connect(pEdit, &plQtCurve1DEditorWidget::BeginCpChangesEvent, this, &plQtCurveEditDlg::OnBeginCpChangesEvent);
  connect(pEdit, &plQtCurve1DEditorWidget::EndCpChangesEvent, this, &plQtCurveEditDlg::OnEndCpChangesEvent);
  connect(pEdit, &plQtCurve1DEditorWidget::BeginOperationEvent, this, &plQtCurveEditDlg::OnBeginOperationEvent);
  connect(pEdit, &plQtCurve1DEditorWidget::EndOperationEvent, this, &plQtCurveEditDlg::OnEndOperationEvent);

  m_pShortcutUndo = new QShortcut(QKeySequence("Ctrl+Z"), this);
  m_pShortcutRedo = new QShortcut(QKeySequence("Ctrl+Y"), this);

  connect(m_pShortcutUndo, &QShortcut::activated, this, &plQtCurveEditDlg::on_actionUndo_triggered);
  connect(m_pShortcutRedo, &QShortcut::activated, this, &plQtCurveEditDlg::on_actionRedo_triggered);

  m_Curves.m_Curves.PushBack(PLASMA_DEFAULT_NEW(plSingleCurveData));

  RetrieveCurveState();

  m_uiActionsUndoBaseline = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->GetUndoStackSize();
}

void plQtCurveEditDlg::RetrieveCurveState()
{
  auto& curve = m_Curves.m_Curves.PeekBack();

  plInt32 iNumPoints = 0;
  m_pObjectAccessor->GetCount(m_pCurveObject, "ControlPoints", iNumPoints).IgnoreResult();
  curve->m_ControlPoints.SetCount(iNumPoints);

  plVariant v;

  // get a local representation of the curve once, so that we can update the preview more efficiently
  for (plInt32 i = 0; i < iNumPoints; ++i)
  {
    const plDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", i);

    m_pObjectAccessor->GetValue(pPoint, "Tick", v).IgnoreResult();
    curve->m_ControlPoints[i].m_iTick = v.ConvertTo<plInt32>();

    m_pObjectAccessor->GetValue(pPoint, "Value", v).IgnoreResult();
    curve->m_ControlPoints[i].m_fValue = v.ConvertTo<double>();

    m_pObjectAccessor->GetValue(pPoint, "LeftTangent", v).IgnoreResult();
    curve->m_ControlPoints[i].m_LeftTangent = v.ConvertTo<plVec2>();

    m_pObjectAccessor->GetValue(pPoint, "RightTangent", v).IgnoreResult();
    curve->m_ControlPoints[i].m_RightTangent = v.ConvertTo<plVec2>();

    m_pObjectAccessor->GetValue(pPoint, "Linked", v).IgnoreResult();
    curve->m_ControlPoints[i].m_bTangentsLinked = v.ConvertTo<bool>();

    m_pObjectAccessor->GetValue(pPoint, "LeftTangentMode", v).IgnoreResult();
    curve->m_ControlPoints[i].m_LeftTangentMode = (plCurveTangentMode::Enum)v.ConvertTo<plInt32>();

    m_pObjectAccessor->GetValue(pPoint, "RightTangentMode", v).IgnoreResult();
    curve->m_ControlPoints[i].m_RightTangentMode = (plCurveTangentMode::Enum)v.ConvertTo<plInt32>();
  }
}

plQtCurveEditDlg::~plQtCurveEditDlg()
{
  s_LastDialogGeometry = saveGeometry();
}

void plQtCurveEditDlg::SetCurveColor(const plColor& color)
{
  m_Curves.m_Curves.PeekBack()->m_CurveColor = color;
}

void plQtCurveEditDlg::SetCurveExtents(double fLower, bool bLowerFixed, double fUpper, bool bUpperFixed)
{
  m_fLowerExtents = fLower;
  m_fUpperExtents = fUpper;
  m_bLowerFixed = bLowerFixed;
  m_bUpperFixed = bUpperFixed;
}

void plQtCurveEditDlg::SetCurveRanges(double fLower, double fUpper)
{
  m_fLowerRange = fLower;
  m_fUpperRange = fUpper;
}

void plQtCurveEditDlg::reject()
{
  // ignore
}

void plQtCurveEditDlg::accept()
{
  // ignore
}

void plQtCurveEditDlg::cancel()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  cmd.Undo(cmd.GetUndoStackSize() - m_uiActionsUndoBaseline).IgnoreResult();

  QDialog::reject();
}

void plQtCurveEditDlg::UpdatePreview()
{
  plQtCurve1DEditorWidget* pEdit = CurveEditor;
  pEdit->SetCurveExtents(m_fLowerExtents, m_fUpperExtents, m_bLowerFixed, m_bUpperFixed);
  pEdit->SetCurveRanges(m_fLowerRange, m_fUpperRange);
  pEdit->SetCurves(m_Curves);
}

void plQtCurveEditDlg::closeEvent(QCloseEvent*)
{
  cancel();
}

void plQtCurveEditDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  UpdatePreview();
}

void plQtCurveEditDlg::OnCpMovedEvent(plUInt32 curveIdx, plUInt32 cpIdx, plInt64 iTickX, double newPosY)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (cp.m_iTick != iTickX || cp.m_fValue != newPosY)
    {
      cp.m_iTick = iTickX;
      cp.m_fValue = newPosY;
    }
  }

  // update the actual object
  {
    const plDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", cpIdx);

    m_pObjectAccessor->SetValue(pPoint, "Tick", iTickX).IgnoreResult();
    m_pObjectAccessor->SetValue(pPoint, "Value", newPosY).IgnoreResult();
  }
}

void plQtCurveEditDlg::OnCpDeletedEvent(plUInt32 curveIdx, plUInt32 cpIdx)
{
  // update the local representation
  {
    m_Curves.m_Curves[curveIdx]->m_ControlPoints.RemoveAtAndCopy(cpIdx);
  }

  // update the actual object
  {
    const plDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", cpIdx);
    m_pObjectAccessor->RemoveObject(pPoint).IgnoreResult();
  }
}

void plQtCurveEditDlg::OnTangentMovedEvent(plUInt32 curveIdx, plUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (rightTangent)
      cp.m_RightTangent.Set(newPosX, newPosY);
    else
      cp.m_LeftTangent.Set(newPosX, newPosY);
  }

  // update the actual object
  {
    const plDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", cpIdx);

    if (rightTangent)
      m_pObjectAccessor->SetValue(pPoint, "RightTangent", plVec2(newPosX, newPosY)).IgnoreResult();
    else
      m_pObjectAccessor->SetValue(pPoint, "LeftTangent", plVec2(newPosX, newPosY)).IgnoreResult();
  }
}

void plQtCurveEditDlg::OnInsertCpEvent(plUInt32 curveIdx, plInt64 tickX, double value)
{
  // update the local representation
  {
    plCurveControlPointData cp;
    cp.m_iTick = tickX;
    cp.m_fValue = value;

    m_Curves.m_Curves[curveIdx]->m_ControlPoints.PushBack(cp);
  }

  // update the actual object
  {
    plUuid guid;
    m_pObjectAccessor->AddObject(m_pCurveObject, "ControlPoints", -1, plGetStaticRTTI<plCurveControlPointData>(), guid).IgnoreResult();

    const plDocumentObject* pPoint = m_pObjectAccessor->GetObject(guid);

    m_pObjectAccessor->SetValue(pPoint, "Tick", tickX).IgnoreResult();
    m_pObjectAccessor->SetValue(pPoint, "Value", value).IgnoreResult();
  }
}

void plQtCurveEditDlg::OnTangentLinkEvent(plUInt32 curveIdx, plUInt32 cpIdx, bool bLink)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];
    cp.m_bTangentsLinked = bLink;
  }

  // update the actual object
  {
    const plDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", cpIdx);

    m_pObjectAccessor->SetValue(pPoint, "Linked", bLink).IgnoreResult();
  }
}

void plQtCurveEditDlg::OnCpTangentModeEvent(plUInt32 curveIdx, plUInt32 cpIdx, bool rightTangent, int mode)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (rightTangent)
      cp.m_RightTangentMode = (plCurveTangentMode::Enum)mode;
    else
      cp.m_LeftTangentMode = (plCurveTangentMode::Enum)mode;
  }

  // update the actual object
  {
    const plDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", cpIdx);

    if (rightTangent)
      m_pObjectAccessor->SetValue(pPoint, "RightTangentMode", mode).IgnoreResult();
    else
      m_pObjectAccessor->SetValue(pPoint, "LeftTangentMode", mode).IgnoreResult();
  }
}

void plQtCurveEditDlg::OnBeginCpChangesEvent(QString name)
{
  m_pObjectAccessor->StartTransaction(name.toUtf8().data());
}

void plQtCurveEditDlg::OnEndCpChangesEvent()
{
  m_pObjectAccessor->FinishTransaction();

  UpdatePreview();
}

void plQtCurveEditDlg::OnBeginOperationEvent(QString name)
{
  m_pObjectAccessor->BeginTemporaryCommands(name.toUtf8().data());
}

void plQtCurveEditDlg::OnEndOperationEvent(bool commit)
{
  if (commit)
    m_pObjectAccessor->FinishTemporaryCommands();
  else
    m_pObjectAccessor->CancelTemporaryCommands();

  UpdatePreview();
}

void plQtCurveEditDlg::on_actionUndo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();

  if (cmd.CanUndo() && cmd.GetUndoStackSize() > m_uiActionsUndoBaseline)
  {
    cmd.Undo().IgnoreResult();

    RetrieveCurveState();
    UpdatePreview();
  }
}

void plQtCurveEditDlg::on_actionRedo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();

  if (cmd.CanRedo())
  {
    cmd.Redo().IgnoreResult();

    RetrieveCurveState();
    UpdatePreview();
  }
}

void plQtCurveEditDlg::on_ButtonOk_clicked()
{
  QDialog::accept();
}

void plQtCurveEditDlg::on_ButtonCancel_clicked()
{
  cancel();
}
