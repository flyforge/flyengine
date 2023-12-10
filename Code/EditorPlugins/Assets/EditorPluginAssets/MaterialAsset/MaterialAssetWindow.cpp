#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

////////////////////////////////////////////////////////////////////////
// plMaterialModelAction
////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMaterialModelAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plMaterialModelAction::plMaterialModelAction(const plActionContext& context, const char* szName, const char* szIconPath)
  : plEnumerationMenuAction(context, szName, szIconPath)
{
  InitEnumerationType(plGetStaticRTTI<plMaterialAssetPreview>());
}

plInt64 plMaterialModelAction::GetValue() const
{
  return static_cast<const plMaterialAssetDocument*>(m_Context.m_pDocument)->m_PreviewModel.GetValue();
}

void plMaterialModelAction::Execute(const plVariant& value)
{
  ((plMaterialAssetDocument*)m_Context.m_pDocument)->m_PreviewModel.SetValue(value.ConvertTo<plInt32>());
}

//////////////////////////////////////////////////////////////////////////
// plMaterialAssetActions
//////////////////////////////////////////////////////////////////////////

plActionDescriptorHandle plMaterialAssetActions::s_hMaterialModelAction;

void plMaterialAssetActions::RegisterActions()
{
  s_hMaterialModelAction = PLASMA_REGISTER_DYNAMIC_MENU("MaterialAsset.Model", plMaterialModelAction, ":/EditorFramework/Icons/Perspective.svg");
}

void plMaterialAssetActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hMaterialModelAction);
}

void plMaterialAssetActions::MapToolbarActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hMaterialModelAction, "", 45.0f);
}


//////////////////////////////////////////////////////////////////////////
// plQtMaterialAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////


plInt32 plQtMaterialAssetDocumentWindow::s_iNodeConfigWatchers = 0;
plDirectoryWatcher* plQtMaterialAssetDocumentWindow::s_pNodeConfigWatcher = nullptr;


plQtMaterialAssetDocumentWindow::plQtMaterialAssetDocumentWindow(plMaterialAssetDocument* pDocument)
  : plQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtMaterialAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plQtMaterialAssetDocumentWindow::SelectionEventHandler, this));

  pDocument->m_VisualShaderEvents.AddEventHandler(plMakeDelegate(&plQtMaterialAssetDocumentWindow::VisualShaderEventHandler, this));

  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "MaterialAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "MaterialAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("MaterialAssetWindowToolBar");
    addToolBar(pToolBar);
  }


  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(plVec3(+1.6f, 0.5f, 0.3f), plVec3(0, 0, 0), plVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90, 0.01f, 100.0f);

    m_pViewWidget = new plQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureFixed(plVec3(0), plVec3(0.0f), plVec3(+0.23f, -0.04f, 0.02f));

    AddViewWidget(m_pViewWidget);
    plQtViewWidgetContainer* pContainer = new plQtViewWidgetContainer(nullptr, m_pViewWidget, "MaterialAssetViewToolBar");

    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("MaterialAssetDockWidget");
    pPropertyPanel->setWindowTitle("Material Properties");
    pPropertyPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
  }

  // Visual Shader Editor
  {
    m_pVsePanel = new plQtDocumentPanel(this, pDocument);
    m_pVsePanel->setObjectName("VisualShaderDockWidget");
    m_pVsePanel->setWindowTitle("Visual Shader Editor");

    QSplitter* pSplitter = new QSplitter(Qt::Orientation::Horizontal, m_pVsePanel);

    m_pScene = new plQtVisualShaderScene(this);
    m_pScene->InitScene(static_cast<const plDocumentNodeManager*>(pDocument->GetObjectManager()));

    m_pNodeView = new plQtNodeView(m_pVsePanel);
    m_pNodeView->SetScene(m_pScene);
    pSplitter->addWidget(m_pNodeView);

    QWidget* pRightGroup = new QWidget(m_pVsePanel);
    pRightGroup->setLayout(new QVBoxLayout());

    QWidget* pButtonGroup = new QWidget(m_pVsePanel);
    pButtonGroup->setLayout(new QHBoxLayout());

    m_pOutputLine = new QTextEdit(m_pVsePanel);
    m_pOutputLine->setText("Transform the material asset to compile the Visual Shader.");
    m_pOutputLine->setReadOnly(true);

    m_pOpenShaderButton = new QPushButton(m_pVsePanel);
    m_pOpenShaderButton->setText("Open Shader File");
    connect(m_pOpenShaderButton, &QPushButton::clicked, this, &plQtMaterialAssetDocumentWindow::OnOpenShaderClicked);

    pButtonGroup->layout()->setContentsMargins(0, 0, 0, 0);
    pButtonGroup->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    pButtonGroup->layout()->addWidget(m_pOpenShaderButton);

    pRightGroup->layout()->setContentsMargins(0, 0, 0, 0);
    pRightGroup->layout()->addWidget(m_pOutputLine);
    pRightGroup->layout()->addWidget(pButtonGroup);

    pSplitter->addWidget(pRightGroup);

    pSplitter->setStretchFactor(0, 10);
    pSplitter->setStretchFactor(1, 1);

    m_bVisualShaderEnabled = false;
    m_pVsePanel->setWidget(pSplitter);
    m_pVsePanel->setVisible(false);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pVsePanel);

    m_pVsePanel->setVisible(false);
  }

  pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  UpdatePreview();

  UpdateNodeEditorVisibility();

  FinishWindowCreation();
}

plQtMaterialAssetDocumentWindow::~plQtMaterialAssetDocumentWindow()
{
  GetMaterialDocument()->m_VisualShaderEvents.RemoveEventHandler(plMakeDelegate(&plQtMaterialAssetDocumentWindow::VisualShaderEventHandler, this));

  RestoreResource();

  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtMaterialAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtMaterialAssetDocumentWindow::PropertyEventHandler, this));

  const bool bCustom = GetMaterialDocument()->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<plInt64>() == plMaterialShaderMode::Custom;

  if (bCustom)
  {
    SetupDirectoryWatcher(false);
  }
}

void plQtMaterialAssetDocumentWindow::SetupDirectoryWatcher(bool needIt)
{
  if (needIt)
  {
    ++s_iNodeConfigWatchers;

    if (s_pNodeConfigWatcher == nullptr)
    {
      s_pNodeConfigWatcher = PLASMA_DEFAULT_NEW(plDirectoryWatcher);

      plStringBuilder sSearchDir = plApplicationServices::GetSingleton()->GetApplicationDataFolder();
      sSearchDir.AppendPath("VisualShader");

      if (s_pNodeConfigWatcher->OpenDirectory(sSearchDir, plDirectoryWatcher::Watch::Writes).Failed())
        plLog::Warning("Could not register a file system watcher for changes to '{0}'", sSearchDir);
    }
  }
  else
  {
    --s_iNodeConfigWatchers;

    if (s_iNodeConfigWatchers == 0)
    {
      PLASMA_DEFAULT_DELETE(s_pNodeConfigWatcher);
    }
  }
}

plMaterialAssetDocument* plQtMaterialAssetDocumentWindow::GetMaterialDocument()
{
  return static_cast<plMaterialAssetDocument*>(GetDocument());
}

void plQtMaterialAssetDocumentWindow::InternalRedraw()
{
  plEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  if (s_pNodeConfigWatcher)
  {
    s_pNodeConfigWatcher->EnumerateChanges(plMakeDelegate(&plQtMaterialAssetDocumentWindow::OnVseConfigChanged, this));
  }
  plQtEngineDocumentWindow::InternalRedraw();
}


void plQtMaterialAssetDocumentWindow::showEvent(QShowEvent* event)
{
  plQtEngineDocumentWindow::showEvent(event);

  m_pVsePanel->setVisible(m_bVisualShaderEnabled);
}

void plQtMaterialAssetDocumentWindow::OnOpenShaderClicked(bool)
{
  plAssetDocumentManager* pManager = (plAssetDocumentManager*)GetMaterialDocument()->GetDocumentManager();

  plString sAutoGenShader = pManager->GetAbsoluteOutputFileName(GetMaterialDocument()->GetAssetDocumentTypeDescriptor(), GetMaterialDocument()->GetDocumentPath(), plMaterialAssetDocumentManager::s_szShaderOutputTag);

  if (plOSFile::ExistsFile(sAutoGenShader))
  {
    plQtUiServices::OpenFileInDefaultProgram(sAutoGenShader);
  }
  else
  {
    plStringBuilder msg;
    msg.Format("The auto generated file does not exist (yet).\nThe supposed location is '{0}'", sAutoGenShader);

    plQtUiServices::GetSingleton()->MessageBoxInformation(msg);
  }
}

void plQtMaterialAssetDocumentWindow::UpdatePreview()
{
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  plResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Material";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plContiguousMemoryStreamStorage streamStorage;
  plMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  plStringBuilder sAbsFilePath = GetMaterialDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("plMaterialBin");
  // Write Header
  memoryWriter << sAbsFilePath;
  const plUInt64 uiHash = plAssetCurator::GetSingleton()->GetAssetDependencyHash(GetMaterialDocument()->GetGuid());
  plAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetMaterialDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  if (GetMaterialDocument()->WriteMaterialAsset(memoryWriter, plAssetCurator::GetSingleton()->GetActiveAssetProfile(), false).Failed())
    return;

  msg.m_Data = plArrayPtr<const plUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtMaterialAssetDocumentWindow::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (e.m_pObject == GetMaterialDocument()->GetPropertyObject() && e.m_sProperty == "ShaderMode")
  {
    UpdateNodeEditorVisibility();
  }

  UpdatePreview();

  if (e.m_sProperty == "ShaderMode" ||
      e.m_sProperty == "BLEND_MODE" ||
      e.m_sProperty == "BaseMaterial")
  {
    plDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "InvalidateCache";

    GetEditorEngineConnection()->SendMessage(&msg);
  }
}


void plQtMaterialAssetDocumentWindow::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // delayed execution
    QTimer::singleShot(1, [this]() {
      // Check again if the selection is empty. This could have changed due to the delayed execution.
      if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
      {
        GetDocument()->GetSelectionManager()->SetSelection(GetMaterialDocument()->GetPropertyObject());
      } });
  }
}

void plQtMaterialAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const plMaterialAssetDocument* pDoc = static_cast<const plMaterialAssetDocument*>(GetDocument());

    plDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewModel";
    msg.m_iValue = pDoc->m_PreviewModel.GetValue();

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void plQtMaterialAssetDocumentWindow::RestoreResource()
{
  plRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Material";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtMaterialAssetDocumentWindow::UpdateNodeEditorVisibility()
{
  const bool bCustom = GetMaterialDocument()->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<plInt64>() == plMaterialShaderMode::Custom;

  m_pVsePanel->setVisible(bCustom);

  // when this is called during construction, it seems to be overridden again (probably by the dock widget code or the splitter)
  // by delaying it a bit, we have the last word
  QTimer::singleShot(100, this, [this, bCustom]() { m_pVsePanel->setVisible(bCustom); });

  if (m_bVisualShaderEnabled != bCustom)
  {
    m_bVisualShaderEnabled = bCustom;

    SetupDirectoryWatcher(bCustom);
  }
}

void plQtMaterialAssetDocumentWindow::OnVseConfigChanged(plStringView sFilename, plDirectoryWatcherAction action, plDirectoryWatcherType type)
{
  if (type != plDirectoryWatcherType::File || !plPathUtils::HasExtension(sFilename, "DDL"))
    return;

  // lalala ... this is to allow writes to the file to 'hopefully' finish before we try to read it
  plThreadUtils::Sleep(plTime::MakeFromMilliseconds(100));

  plVisualShaderTypeRegistry::GetSingleton()->UpdateNodeData(sFilename);

  // TODO: We write an invalid hash in the file, should maybe compute the correct one on the fly
  // but that would involve the asset curator which would also save / transform everything which is
  // not what we want.
  plAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(0, GetMaterialDocument()->GetAssetTypeVersion());
  GetMaterialDocument()->RecreateVisualShaderFile(AssetHeader).LogFailure();
}

void plQtMaterialAssetDocumentWindow::VisualShaderEventHandler(const plMaterialVisualShaderEvent& e)
{
  plStringBuilder text;

  if (e.m_Type == plMaterialVisualShaderEvent::VisualShaderNotUsed)
  {
    text = "<span style=\"color:#bbbb00;\">Visual Shader is not used by the material.</span><br><br>Change the ShaderMode in the asset "
           "properties to enable Visual Shader mode.";
  }
  else
  {
    if (e.m_Type == plMaterialVisualShaderEvent::TransformSucceeded)
      text = "<span style=\"color:#00ff00;\">Visual Shader was transformed successfully.</span><br><br>";
    else
      text = "<span style=\"color:#ff8800;\">Visual Shader is invalid:</span><br><br>";

    plStringBuilder err = e.m_sTransformError;

    plHybridArray<plStringView, 16> lines;
    err.Split(false, lines, "\n");

    for (const plStringView& line : lines)
    {
      if (line.StartsWith("Error:"))
        text.AppendFormat("<span style=\"color:#ff2200;\">{0}</span><br>", line);
      else if (line.StartsWith("Warning:"))
        text.AppendFormat("<span style=\"color:#ffaa00;\">{0}</span><br>", line);
      else
        text.Append(line);
    }
    UpdatePreview();
  }

  m_pOutputLine->setAcceptRichText(true);
  m_pOutputLine->setHtml(text.GetData());
}
