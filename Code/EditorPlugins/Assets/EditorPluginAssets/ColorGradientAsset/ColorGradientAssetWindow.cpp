#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>

plQtColorGradientAssetDocumentWindow::plQtColorGradientAssetDocumentWindow(plDocument* pDocument)
  : plQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtColorGradientAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(plMakeDelegate(&plQtColorGradientAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "ColorGradientAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "ColorGradientAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ColorGradientAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_bShowFirstTime = true;
  m_pGradientEditor = new plQtColorGradientEditorWidget(this);

  QWidget* pContainer = new QWidget(this);
  pContainer->setLayout(new QVBoxLayout());
  pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
  pContainer->layout()->addWidget(m_pGradientEditor);
  pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  setCentralWidget(pContainer);

  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::ColorCpAdded, this, &plQtColorGradientAssetDocumentWindow::onGradientColorCpAdded);
  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::ColorCpMoved, this, &plQtColorGradientAssetDocumentWindow::onGradientColorCpMoved);
  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::ColorCpDeleted, this, &plQtColorGradientAssetDocumentWindow::onGradientColorCpDeleted);
  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::ColorCpChanged, this, &plQtColorGradientAssetDocumentWindow::onGradientColorCpChanged);

  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::AlphaCpAdded, this, &plQtColorGradientAssetDocumentWindow::onGradientAlphaCpAdded);
  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::AlphaCpMoved, this, &plQtColorGradientAssetDocumentWindow::onGradientAlphaCpMoved);
  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::AlphaCpDeleted, this, &plQtColorGradientAssetDocumentWindow::onGradientAlphaCpDeleted);
  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::AlphaCpChanged, this, &plQtColorGradientAssetDocumentWindow::onGradientAlphaCpChanged);

  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::IntensityCpAdded, this, &plQtColorGradientAssetDocumentWindow::onGradientIntensityCpAdded);
  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::IntensityCpMoved, this, &plQtColorGradientAssetDocumentWindow::onGradientIntensityCpMoved);
  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::IntensityCpDeleted, this, &plQtColorGradientAssetDocumentWindow::onGradientIntensityCpDeleted);
  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::IntensityCpChanged, this, &plQtColorGradientAssetDocumentWindow::onGradientIntensityCpChanged);

  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::BeginOperation, this, &plQtColorGradientAssetDocumentWindow::onGradientBeginOperation);
  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::EndOperation, this, &plQtColorGradientAssetDocumentWindow::onGradientEndOperation);

  connect(m_pGradientEditor, &plQtColorGradientEditorWidget::NormalizeRange, this, &plQtColorGradientAssetDocumentWindow::onGradientNormalizeRange);

  // property grid, if needed
  if (false)
  {
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("ColorGradientAssetDockWidget");
    pPropertyPanel->setWindowTitle("COLORGRADIENT PROPERTIES");
    pPropertyPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();
}

plQtColorGradientAssetDocumentWindow::~plQtColorGradientAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtColorGradientAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(plMakeDelegate(&plQtColorGradientAssetDocumentWindow::StructureEventHandler, this));

  RestoreResource();
}

void plQtColorGradientAssetDocumentWindow::onGradientColorCpAdded(double posX, const plColorGammaUB& color)
{
  plColorGradientAssetDocument* pDoc = static_cast<plColorGradientAssetDocument*>(GetDocument());

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Color Control Point");

  plAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "ColorCPs";
  cmdAdd.m_pType = plGetStaticRTTI<plColorControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd).IgnoreResult();

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = pDoc->GetProperties()->TickFromTime(plTime::Seconds(posX));
  history->AddCommand(cmdSet).IgnoreResult();

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue = color.r;
  history->AddCommand(cmdSet).IgnoreResult();

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue = color.g;
  history->AddCommand(cmdSet).IgnoreResult();

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue = color.b;
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}


void plQtColorGradientAssetDocumentWindow::onGradientAlphaCpAdded(double posX, plUInt8 alpha)
{
  plColorGradientAssetDocument* pDoc = static_cast<plColorGradientAssetDocument*>(GetDocument());

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Alpha Control Point");

  plAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "AlphaCPs";
  cmdAdd.m_pType = plGetStaticRTTI<plAlphaControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd).IgnoreResult();

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = pDoc->GetProperties()->TickFromTime(plTime::Seconds(posX));
  history->AddCommand(cmdSet).IgnoreResult();

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}


void plQtColorGradientAssetDocumentWindow::onGradientIntensityCpAdded(double posX, float intensity)
{
  plColorGradientAssetDocument* pDoc = static_cast<plColorGradientAssetDocument*>(GetDocument());

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Intensity Control Point");

  plAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "IntensityCPs";
  cmdAdd.m_pType = plGetStaticRTTI<plIntensityControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd).IgnoreResult();

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = pDoc->GetProperties()->TickFromTime(plTime::Seconds(posX));
  history->AddCommand(cmdSet).IgnoreResult();

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}

void plQtColorGradientAssetDocumentWindow::MoveCP(plInt32 idx, double newPosX, const char* szArrayName)
{
  plColorGradientAssetDocument* pDoc = static_cast<plColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  plVariant objGuid = pProp->GetTypeAccessor().GetValue(szArrayName, idx);

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Move Control Point");

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = pDoc->GetProperties()->TickFromTime(plTime::Seconds(newPosX));
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}

void plQtColorGradientAssetDocumentWindow::onGradientColorCpMoved(plInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "ColorCPs");
}

void plQtColorGradientAssetDocumentWindow::onGradientAlphaCpMoved(plInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "AlphaCPs");
}


void plQtColorGradientAssetDocumentWindow::onGradientIntensityCpMoved(plInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "IntensityCPs");
}

void plQtColorGradientAssetDocumentWindow::RemoveCP(plInt32 idx, const char* szArrayName)
{
  plColorGradientAssetDocument* pDoc = static_cast<plColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  plVariant objGuid = pProp->GetTypeAccessor().GetValue(szArrayName, idx);

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  plRemoveObjectCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<plUuid>();
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}

void plQtColorGradientAssetDocumentWindow::onGradientColorCpDeleted(plInt32 idx)
{
  RemoveCP(idx, "ColorCPs");
}


void plQtColorGradientAssetDocumentWindow::onGradientAlphaCpDeleted(plInt32 idx)
{
  RemoveCP(idx, "AlphaCPs");
}


void plQtColorGradientAssetDocumentWindow::onGradientIntensityCpDeleted(plInt32 idx)
{
  RemoveCP(idx, "IntensityCPs");
}


void plQtColorGradientAssetDocumentWindow::onGradientColorCpChanged(plInt32 idx, const plColorGammaUB& color)
{
  plColorGradientAssetDocument* pDoc = static_cast<plColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();
  plVariant objGuid = pProp->GetTypeAccessor().GetValue("ColorCPs", idx);

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Color");

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue = color.r;
  history->AddCommand(cmdSet).IgnoreResult();

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue = color.g;
  history->AddCommand(cmdSet).IgnoreResult();

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue = color.b;
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}


void plQtColorGradientAssetDocumentWindow::onGradientAlphaCpChanged(plInt32 idx, plUInt8 alpha)
{
  plColorGradientAssetDocument* pDoc = static_cast<plColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();
  plVariant objGuid = pProp->GetTypeAccessor().GetValue("AlphaCPs", idx);

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Alpha");

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}

void plQtColorGradientAssetDocumentWindow::onGradientIntensityCpChanged(plInt32 idx, float intensity)
{
  plColorGradientAssetDocument* pDoc = static_cast<plColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();
  plVariant objGuid = pProp->GetTypeAccessor().GetValue("IntensityCPs", idx);

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Intensity");

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}


void plQtColorGradientAssetDocumentWindow::onGradientBeginOperation()
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Gradient");
}


void plQtColorGradientAssetDocumentWindow::onGradientEndOperation(bool commit)
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}


void plQtColorGradientAssetDocumentWindow::onGradientNormalizeRange()
{
  if (plQtUiServices::GetSingleton()->MessageBoxQuestion("This will adjust the positions of all control points, such that the minimum is at 0 and the maximum at 1.\n\nContinue?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) !=
      QMessageBox::StandardButton::Yes)
    return;

  plColorGradientAssetDocument* pDoc = static_cast<plColorGradientAssetDocument*>(GetDocument());

  plColorGradient GradientData;
  pDoc->GetProperties()->FillGradientData(GradientData);

  double minX, maxX;
  if (!GradientData.GetExtents(minX, maxX))
    return;

  if ((minX == 0 && maxX == 1) || (minX >= maxX))
    return;

  plCommandHistory* history = GetDocument()->GetCommandHistory();

  const float rangeNorm = 1.0f / (maxX - minX);

  history->StartTransaction("Normalize Gradient Range");

  plUInt32 numRgb, numAlpha, numInt;
  GradientData.GetNumControlPoints(numRgb, numAlpha, numInt);

  for (plUInt32 i = 0; i < numRgb; ++i)
  {
    float x = GradientData.GetColorControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "ColorCPs");
  }

  for (plUInt32 i = 0; i < numAlpha; ++i)
  {
    float x = GradientData.GetAlphaControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "AlphaCPs");
  }

  for (plUInt32 i = 0; i < numInt; ++i)
  {
    float x = GradientData.GetIntensityControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "IntensityCPs");
  }

  history->FinishTransaction();

  m_pGradientEditor->FrameGradient();
}

void plQtColorGradientAssetDocumentWindow::UpdatePreview()
{
  plColorGradient GradientData;

  plColorGradientAssetDocument* pDoc = static_cast<plColorGradientAssetDocument*>(GetDocument());
  pDoc->GetProperties()->FillGradientData(GradientData);

  m_pGradientEditor->SetColorGradient(GradientData);

  if (m_bShowFirstTime)
  {
    m_bShowFirstTime = false;
    m_pGradientEditor->FrameGradient();
  }

  SendLiveResourcePreview();
}

void plQtColorGradientAssetDocumentWindow::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void plQtColorGradientAssetDocumentWindow::StructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  UpdatePreview();
}

void plQtColorGradientAssetDocumentWindow::SendLiveResourcePreview()
{
  if (PlasmaEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  plResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Color Gradient";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plContiguousMemoryStreamStorage streamStorage;
  plMemoryStreamWriter memoryWriter(&streamStorage);

  plColorGradientAssetDocument* pDoc = plDynamicCast<plColorGradientAssetDocument*>(GetDocument());

  // Write Path
  plStringBuilder sAbsFilePath = pDoc->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("plColorGradient");

  // Write Header
  memoryWriter << sAbsFilePath;
  const plUInt64 uiHash = plAssetCurator::GetSingleton()->GetAssetDependencyHash(pDoc->GetGuid());
  plAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, pDoc->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  pDoc->WriteResource(memoryWriter);
  msg.m_Data = plArrayPtr<const plUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtColorGradientAssetDocumentWindow::RestoreResource()
{
  plRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Color Gradient";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
