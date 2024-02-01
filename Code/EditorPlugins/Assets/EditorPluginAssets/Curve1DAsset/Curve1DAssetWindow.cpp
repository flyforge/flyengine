#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>


plQtCurve1DAssetDocumentWindow::plQtCurve1DAssetDocumentWindow(plDocument* pDocument)
  : plQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(plMakeDelegate(&plQtCurve1DAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "Curve1DAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "Curve1DAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("Curve1DAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pCurveEditor = new plQtCurve1DEditorWidget(this);

  QWidget* pContainer = new QWidget(this);
  pContainer->setLayout(new QVBoxLayout());
  pContainer->layout()->addWidget(m_pCurveEditor);

  setCentralWidget(pContainer);

  connect(m_pCurveEditor, &plQtCurve1DEditorWidget::InsertCpEvent, this, &plQtCurve1DAssetDocumentWindow::onInsertCpAt);
  connect(m_pCurveEditor, &plQtCurve1DEditorWidget::CpMovedEvent, this, &plQtCurve1DAssetDocumentWindow::onCurveCpMoved);
  connect(m_pCurveEditor, &plQtCurve1DEditorWidget::CpDeletedEvent, this, &plQtCurve1DAssetDocumentWindow::onCurveCpDeleted);
  connect(m_pCurveEditor, &plQtCurve1DEditorWidget::TangentMovedEvent, this, &plQtCurve1DAssetDocumentWindow::onCurveTangentMoved);
  connect(m_pCurveEditor, &plQtCurve1DEditorWidget::TangentLinkEvent, this, &plQtCurve1DAssetDocumentWindow::onLinkCurveTangents);
  connect(m_pCurveEditor, &plQtCurve1DEditorWidget::CpTangentModeEvent, this, &plQtCurve1DAssetDocumentWindow::onCurveTangentModeChanged);

  connect(m_pCurveEditor, &plQtCurve1DEditorWidget::BeginOperationEvent, this, &plQtCurve1DAssetDocumentWindow::onCurveBeginOperation);
  connect(m_pCurveEditor, &plQtCurve1DEditorWidget::EndOperationEvent, this, &plQtCurve1DAssetDocumentWindow::onCurveEndOperation);
  connect(m_pCurveEditor, &plQtCurve1DEditorWidget::BeginCpChangesEvent, this, &plQtCurve1DAssetDocumentWindow::onCurveBeginCpChanges);
  connect(m_pCurveEditor, &plQtCurve1DEditorWidget::EndCpChangesEvent, this, &plQtCurve1DAssetDocumentWindow::onCurveEndCpChanges);

  if (false)
  {
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("Curve1DAssetDockWidget");
    pPropertyPanel->setWindowTitle("Curve1D Properties");
    pPropertyPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();
}

plQtCurve1DAssetDocumentWindow::~plQtCurve1DAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(plMakeDelegate(&plQtCurve1DAssetDocumentWindow::StructureEventHandler, this));

  RestoreResource();
}

void plQtCurve1DAssetDocumentWindow::onCurveBeginOperation(QString name)
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands(name.toUtf8().data());
}

void plQtCurve1DAssetDocumentWindow::onCurveEndOperation(bool commit)
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();

  UpdatePreview();
}

void plQtCurve1DAssetDocumentWindow::onCurveBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void plQtCurve1DAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdatePreview();
}

void plQtCurve1DAssetDocumentWindow::onInsertCpAt(plUInt32 uiCurveIdx, plInt64 tickX, double clickPosY)
{
  plCurve1DAssetDocument* pDoc = static_cast<plCurve1DAssetDocument*>(GetDocument());

  plCommandHistory* history = pDoc->GetCommandHistory();

  if (pDoc->GetPropertyObject()->GetTypeAccessor().GetCount("Curves") == 0)
  {
    // no curves allocated yet, add one

    plAddObjectCommand cmdAddCurve;
    cmdAddCurve.m_Parent = pDoc->GetPropertyObject()->GetGuid();
    cmdAddCurve.m_NewObjectGuid = plUuid::MakeUuid();
    cmdAddCurve.m_sParentProperty = "Curves";
    cmdAddCurve.m_pType = plGetStaticRTTI<plSingleCurveData>();
    cmdAddCurve.m_Index = -1;

    history->AddCommand(cmdAddCurve).AssertSuccess();
  }

  const plVariant curveGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Curves", uiCurveIdx);

  plAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = curveGuid.Get<plUuid>();
  cmdAdd.m_NewObjectGuid = plUuid::MakeUuid();
  cmdAdd.m_sParentProperty = "ControlPoints";
  cmdAdd.m_pType = plGetStaticRTTI<plCurveControlPointData>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd).AssertSuccess();

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = tickX;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = clickPosY;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "LeftTangent";
  cmdSet.m_NewValue = plVec2(-0.1f, 0.0f);
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "RightTangent";
  cmdSet.m_NewValue = plVec2(+0.1f, 0.0f);
  history->AddCommand(cmdSet).AssertSuccess();
}

void plQtCurve1DAssetDocumentWindow::onCurveCpMoved(plUInt32 curveIdx, plUInt32 cpIdx, plInt64 iTickX, double newPosY)
{
  iTickX = plMath::Max<plInt64>(iTickX, 0);

  plCurve1DAssetDocument* pDoc = static_cast<plCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const plVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const plDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<plUuid>());
  const plVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = newPosY;
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void plQtCurve1DAssetDocumentWindow::onCurveCpDeleted(plUInt32 curveIdx, plUInt32 cpIdx)
{
  plCurve1DAssetDocument* pDoc = static_cast<plCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const plVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const plDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<plUuid>());
  const plVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  plRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<plUuid>();
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void plQtCurve1DAssetDocumentWindow::onCurveTangentMoved(plUInt32 curveIdx, plUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  plCurve1DAssetDocument* pDoc = static_cast<plCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const plVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const plDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<plUuid>());
  const plVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<plUuid>();

  // clamp tangents to one side
  if (rightTangent)
    newPosX = plMath::Max(newPosX, 0.0f);
  else
    newPosX = plMath::Min(newPosX, 0.0f);

  cmdSet.m_sProperty = rightTangent ? "RightTangent" : "LeftTangent";
  cmdSet.m_NewValue = plVec2(newPosX, newPosY);
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void plQtCurve1DAssetDocumentWindow::onLinkCurveTangents(plUInt32 curveIdx, plUInt32 cpIdx, bool bLink)
{
  plCurve1DAssetDocument* pDoc = static_cast<plCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const plVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const plDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<plUuid>());
  const plVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  plSetObjectPropertyCommand cmdLink;
  cmdLink.m_Object = cpGuid.Get<plUuid>();
  cmdLink.m_sProperty = "Linked";
  cmdLink.m_NewValue = bLink;
  GetDocument()->GetCommandHistory()->AddCommand(cmdLink).AssertSuccess();

  if (bLink)
  {
    const plVec2 leftTangent = pDoc->GetProperties()->m_Curves[curveIdx]->m_ControlPoints[cpIdx].m_LeftTangent;
    const plVec2 rightTangent(-leftTangent.x, -leftTangent.y);

    onCurveTangentMoved(curveIdx, cpIdx, rightTangent.x, rightTangent.y, true);
  }
}

void plQtCurve1DAssetDocumentWindow::onCurveTangentModeChanged(plUInt32 curveIdx, plUInt32 cpIdx, bool rightTangent, int mode)
{
  plCurve1DAssetDocument* pDoc = static_cast<plCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const plVariant curveGuid = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const plDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<plUuid>());
  const plVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  plSetObjectPropertyCommand cmd;
  cmd.m_Object = cpGuid.Get<plUuid>();
  cmd.m_sProperty = rightTangent ? "RightTangentMode" : "LeftTangentMode";
  cmd.m_NewValue = mode;
  GetDocument()->GetCommandHistory()->AddCommand(cmd).AssertSuccess();

  // sync current curve back
  if (false)
  {
    // generally works, but would need some work to make it perfect

    plCurve1D curve;
    pDoc->GetProperties()->m_Curves[curveIdx]->ConvertToRuntimeData(curve);
    curve.SortControlPoints();
    curve.ApplyTangentModes();

    for (plUInt32 i = 0; i < curve.GetNumControlPoints(); ++i)
    {
      const auto& cp = curve.GetControlPoint(i);
      if (cp.m_uiOriginalIndex == cpIdx)
      {
        if (rightTangent)
          onCurveTangentMoved(curveIdx, cpIdx, cp.m_RightTangent.x, cp.m_RightTangent.y, true);
        else
          onCurveTangentMoved(curveIdx, cpIdx, cp.m_LeftTangent.x, cp.m_LeftTangent.y, false);

        break;
      }
    }
  }
}

void plQtCurve1DAssetDocumentWindow::UpdatePreview()
{
  plCurve1DAssetDocument* pDoc = static_cast<plCurve1DAssetDocument*>(GetDocument());

  m_pCurveEditor->SetCurveExtents(0, 0.1f, true, false);
  m_pCurveEditor->SetCurves(*pDoc->GetProperties());

  SendLiveResourcePreview();
}

void plQtCurve1DAssetDocumentWindow::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void plQtCurve1DAssetDocumentWindow::StructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  UpdatePreview();
}

void plQtCurve1DAssetDocumentWindow::SendLiveResourcePreview()
{
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  plResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Curve1D";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plContiguousMemoryStreamStorage streamStorage;
  plMemoryStreamWriter memoryWriter(&streamStorage);

  plCurve1DAssetDocument* pDoc = plDynamicCast<plCurve1DAssetDocument*>(GetDocument());

  // Write Path
  plStringBuilder sAbsFilePath = pDoc->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("plCurve1D");

  // Write Header
  memoryWriter << sAbsFilePath;
  const plUInt64 uiHash = plAssetCurator::GetSingleton()->GetAssetDependencyHash(pDoc->GetGuid());
  plAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, pDoc->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  pDoc->WriteResource(memoryWriter);
  msg.m_Data = plArrayPtr<const plUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtCurve1DAssetDocumentWindow::RestoreResource()
{
  plRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Curve1D";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
