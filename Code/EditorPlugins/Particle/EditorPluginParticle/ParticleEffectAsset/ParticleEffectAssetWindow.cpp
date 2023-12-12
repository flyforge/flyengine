#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Gravity.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Continuous.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomColor.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_VelocityCone.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>
#include <QBoxLayout>
#include <QComboBox>
#include <QInputDialog>
#include <QToolButton>
#include <SharedPluginAssets/Common/Messages.h>
#include <ToolsFoundation/Command/TreeCommands.h>

plQtParticleEffectAssetDocumentWindow::plQtParticleEffectAssetDocumentWindow(plAssetDocument* pDocument)
  : plQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtParticleEffectAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(plMakeDelegate(&plQtParticleEffectAssetDocumentWindow::StructureEventHandler, this));


  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "ParticleEffectAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "ParticleEffectAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ParticleEffectAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  plDocumentObject* pRootObject = pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0];

  // plQtDocumentPanel* pMainPropertyPanel = new plQtDocumentPanel(this);
  plQtDocumentPanel* pEffectPanel = new plQtDocumentPanel(this, pDocument);
  plQtDocumentPanel* pReactionsPanel = new plQtDocumentPanel(this, pDocument);
  plQtDocumentPanel* pSystemsPanel = new plQtDocumentPanel(this, pDocument);
  plQtDocumentPanel* pEmitterPanel = new plQtDocumentPanel(this, pDocument);
  plQtDocumentPanel* pInitializerPanel = new plQtDocumentPanel(this, pDocument);
  plQtDocumentPanel* pBehaviorPanel = new plQtDocumentPanel(this, pDocument);
  plQtDocumentPanel* pTypePanel = new plQtDocumentPanel(this, pDocument);

  // Property Grid
  //{
  //  pMainPropertyPanel->setObjectName("ParticleEffectAssetDockWidget");
  //  pMainPropertyPanel->setWindowTitle("Particle Effect Properties");
  //  pMainPropertyPanel->show();

  //  plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pMainPropertyPanel, pDocument);
  //  pMainPropertyPanel->setWidget(pPropertyGrid);

  //  addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pMainPropertyPanel);

  //  pDocument->GetSelectionManager()->SetSelection(pRootObject);
  //}

  // Particle Systems Panel
  {
    pSystemsPanel->setObjectName("ParticleEffectAssetDockWidget_Systems");
    pSystemsPanel->setWindowTitle("SYSTEMS");
    pSystemsPanel->show();

    QWidget* pMainWidget = new QWidget(pSystemsPanel);
    pMainWidget->setContentsMargins(0, 0, 0, 0);
    pMainWidget->setLayout(new QVBoxLayout(pMainWidget));
    pMainWidget->layout()->setContentsMargins(0, 0, 0, 0);

    {
      QWidget* pGroup = new QWidget(pMainWidget);
      pGroup->setContentsMargins(0, 0, 0, 0);
      pGroup->setLayout(new QHBoxLayout(pGroup));
      pGroup->layout()->setContentsMargins(0, 0, 0, 0);

      m_pSystemsCombo = new QComboBox(pSystemsPanel);
      connect(m_pSystemsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onSystemSelected(int)));

      m_pAddSystem = new QToolButton(pSystemsPanel);
      connect(m_pAddSystem, &QAbstractButton::clicked, this, &plQtParticleEffectAssetDocumentWindow::onAddSystem);

      m_pRemoveSystem = new QToolButton(pSystemsPanel);
      connect(m_pRemoveSystem, &QAbstractButton::clicked, this, &plQtParticleEffectAssetDocumentWindow::onRemoveSystem);

      m_pRenameSystem = new QToolButton(pSystemsPanel);
      connect(m_pRenameSystem, &QAbstractButton::clicked, this, &plQtParticleEffectAssetDocumentWindow::onRenameSystem);

      m_pAddSystem->setIcon(QIcon(":/GuiFoundation/Icons/Add.svg"));
      m_pRemoveSystem->setIcon(QIcon(":/GuiFoundation/Icons/Delete.svg"));
      m_pRenameSystem->setIcon(QIcon(":/GuiFoundation/Icons/Rename.svg"));

      pGroup->layout()->addWidget(m_pRenameSystem);
      pGroup->layout()->addWidget(m_pSystemsCombo);
      pGroup->layout()->addWidget(m_pAddSystem);
      pGroup->layout()->addWidget(m_pRemoveSystem);

      pMainWidget->layout()->addWidget(pGroup);
    }

    m_pPropertyGridSystems = new plQtPropertyGridWidget(pSystemsPanel, pDocument);
    m_pPropertyGridSystems->SetSelectionIncludeExcludeProperties(nullptr, "Name;Emitters;Initializers;Behaviors;Types");
    pMainWidget->layout()->addWidget(m_pPropertyGridSystems);

    if (!pRootObject->GetChildren().IsEmpty())
    {
      plDeque<const plDocumentObject*> sel;
      sel.PushBack(pRootObject->GetChildren()[0]);
      m_pPropertyGridSystems->SetSelection(sel);
    }

    pMainWidget->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding));
    pSystemsPanel->setWidget(pMainWidget);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pSystemsPanel);
  }

  // Effect Properties
  {
    pEffectPanel->setObjectName("ParticleEffectAssetDockWidget_Effect");
    pEffectPanel->setWindowTitle("EFFECT");
    pEffectPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pEffectPanel, pDocument, false);
    pEffectPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pEffectPanel);

    plDeque<const plDocumentObject*> sel;
    sel.PushBack(pRootObject);
    pPropertyGrid->SetSelectionIncludeExcludeProperties(nullptr, "EventReactions;ParticleSystems");
    pPropertyGrid->SetSelection(sel);
  }

  // Event Reactions
  {
    pReactionsPanel->setObjectName("ParticleEffectAssetDockWidget_Reactions");
    pReactionsPanel->setWindowTitle("EVENT REACTIONS");
    pReactionsPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pReactionsPanel, pDocument, false);
    pReactionsPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pReactionsPanel);

    plDeque<const plDocumentObject*> sel;
    sel.PushBack(pRootObject);
    pPropertyGrid->SetSelectionIncludeExcludeProperties("EventReactions");
    pPropertyGrid->SetSelection(sel);
  }

  // System Emitters
  {
    pEmitterPanel->setObjectName("ParticleEffectAssetDockWidget_Emitter");
    pEmitterPanel->setWindowTitle("EMITTER");
    pEmitterPanel->show();

    m_pPropertyGridEmitter = new plQtPropertyGridWidget(pEmitterPanel, pDocument, false);
    m_pPropertyGridEmitter->SetSelectionIncludeExcludeProperties("Emitters");
    pEmitterPanel->setWidget(m_pPropertyGridEmitter);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pEmitterPanel);
  }

  // System Initializers
  {
    pInitializerPanel->setObjectName("ParticleEffectAssetDockWidget_Initializer");
    pInitializerPanel->setWindowTitle("INITIALIZERS");
    pInitializerPanel->show();

    m_pPropertyGridInitializer = new plQtPropertyGridWidget(pInitializerPanel, pDocument, false);
    m_pPropertyGridInitializer->SetSelectionIncludeExcludeProperties("Initializers");
    pInitializerPanel->setWidget(m_pPropertyGridInitializer);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pInitializerPanel);
  }

  // System Behaviors
  {
    pBehaviorPanel->setObjectName("ParticleEffectAssetDockWidget_Behavior");
    pBehaviorPanel->setWindowTitle("BEHAVIOR");
    pBehaviorPanel->show();

    m_pPropertyGridBehavior = new plQtPropertyGridWidget(pBehaviorPanel, pDocument, false);
    m_pPropertyGridBehavior->SetSelectionIncludeExcludeProperties("Behaviors");
    pBehaviorPanel->setWidget(m_pPropertyGridBehavior);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pBehaviorPanel);
  }

  // System Types
  {
    pTypePanel->setObjectName("ParticleEffectAssetDockWidget_Type");
    pTypePanel->setWindowTitle("RENDERERS");
    pTypePanel->show();

    m_pPropertyGridType = new plQtPropertyGridWidget(pTypePanel, pDocument, false);
    m_pPropertyGridType->SetSelectionIncludeExcludeProperties("Types");
    pTypePanel->setWidget(m_pPropertyGridType);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pTypePanel);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(plVec3(-1.6f, 0, 0), plVec3(0, 0, 0), plVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new plQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(plVec3(0), plVec3(5.0f), plVec3(-2, 0, 0.5f), 1.0f);
    AddViewWidget(m_pViewWidget);
    plQtViewWidgetContainer* pContainer = new plQtViewWidgetContainer(this, m_pViewWidget, "ParticleEffectAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  m_pAssetDoc = static_cast<plParticleEffectAssetDocument*>(pDocument);

  tabifyDockWidget(pEffectPanel, pSystemsPanel);
  tabifyDockWidget(pEffectPanel, pReactionsPanel);

  tabifyDockWidget(pEmitterPanel, pInitializerPanel);
  tabifyDockWidget(pEmitterPanel, pBehaviorPanel);
  tabifyDockWidget(pEmitterPanel, pTypePanel);

  pSystemsPanel->raise();
  pEmitterPanel->raise();

  FinishWindowCreation();

  UpdateSystemList();
  SendLiveResourcePreview();

  GetParticleDocument()->m_Events.AddEventHandler(plMakeDelegate(&plQtParticleEffectAssetDocumentWindow::ParticleEventHandler, this));
}

plQtParticleEffectAssetDocumentWindow::~plQtParticleEffectAssetDocumentWindow()
{
  GetParticleDocument()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtParticleEffectAssetDocumentWindow::ParticleEventHandler, this));

  RestoreResource();

  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(plMakeDelegate(&plQtParticleEffectAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtParticleEffectAssetDocumentWindow::PropertyEventHandler, this));
}

const char* plQtParticleEffectAssetDocumentWindow::GetWindowLayoutGroupName() const
{
  return "ParticleEffectAsset2";
}

plParticleEffectAssetDocument* plQtParticleEffectAssetDocumentWindow::GetParticleDocument()
{
  return static_cast<plParticleEffectAssetDocument*>(GetDocument());
}

void plQtParticleEffectAssetDocumentWindow::SelectSystem(plDocumentObject* pObject)
{
  if (pObject == nullptr)
  {
    m_sSelectedSystem.Clear();

    m_pPropertyGridSystems->ClearSelection();
    m_pPropertyGridEmitter->ClearSelection();
    m_pPropertyGridInitializer->ClearSelection();
    m_pPropertyGridBehavior->ClearSelection();
    m_pPropertyGridType->ClearSelection();

    m_pSystemsCombo->setCurrentIndex(-1);
  }
  else
  {
    m_sSelectedSystem = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<plString>();

    plDeque<const plDocumentObject*> sel;
    sel.PushBack(pObject);
    GetDocument()->GetSelectionManager()->SetSelection(pObject);
    m_pPropertyGridSystems->SetSelection(sel);

    m_pPropertyGridEmitter->SetSelection(sel);
    m_pPropertyGridInitializer->SetSelection(sel);
    m_pPropertyGridBehavior->SetSelection(sel);
    m_pPropertyGridType->SetSelection(sel);

    m_pSystemsCombo->setCurrentText(m_sSelectedSystem.GetData());
  }
}

void plQtParticleEffectAssetDocumentWindow::onSystemSelected(int index)
{
  if (index >= 0)
  {
    plDocumentObject* pObject = static_cast<plDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

    SelectSystem(pObject);
  }
  else
  {
    SelectSystem(nullptr);
  }
}

void plQtParticleEffectAssetDocumentWindow::onAddSystem(bool)
{
  bool ok = false;
  QString sName;

  while (true)
  {
    sName = QInputDialog::getText(this, "New Particle System", "Name:", QLineEdit::Normal, QString(), &ok);

    if (!ok)
      return;

    if (sName.isEmpty())
    {
      plQtUiServices::GetSingleton()->MessageBoxInformation("Invalid particle system name.");
      continue;
    }

    if (m_ParticleSystems.Find(sName.toUtf8().data()).IsValid())
    {
      plQtUiServices::GetSingleton()->MessageBoxInformation("A particle system with this name exists already.");
      continue;
    }

    break;
  }

  plDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  GetDocument()->GetObjectAccessor()->StartTransaction("Add Particle System");
  plUuid systemGuid;
  systemGuid.CreateNewUuid();

  {
    plAddObjectCommand cmd;
    cmd.m_Parent = pRootObject->GetGuid();
    cmd.m_Index = -1;
    cmd.m_pType = plGetStaticRTTI<plParticleSystemDescriptor>();
    cmd.m_NewObjectGuid = systemGuid;
    cmd.m_sParentProperty = "ParticleSystems";

    if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
    {
      GetDocument()->GetObjectAccessor()->CancelTransaction();
      return;
    }
  }

  m_sSelectedSystem = sName.toUtf8().data();

  {
    plSetObjectPropertyCommand cmd;
    cmd.m_Object = systemGuid;
    cmd.m_NewValue = sName.toUtf8().data();
    cmd.m_sProperty = "Name";
    cmd.m_Index = 0;

    if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
    {
      GetDocument()->GetObjectAccessor()->CancelTransaction();
      return;
    }
  }

  // default system setup
  {
    const plDocumentObject* pSystemObject = GetDocument()->GetObjectAccessor()->GetObject(systemGuid);

    // default life
    {
      const plHybridArray<plDocumentObject*, 8>& children = pSystemObject->GetChildren();

      for (auto pChild : children)
      {
        if (plStringUtils::IsEqual(pChild->GetParentProperty(), "LifeTime"))
        {
          plSetObjectPropertyCommand cmd;
          cmd.m_Object = pChild->GetGuid();
          cmd.m_NewValue = plTime::Seconds(1);
          cmd.m_sProperty = "Value";
          cmd.m_Index = 0;

          if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
          {
            GetDocument()->GetObjectAccessor()->CancelTransaction();
            return;
          }

          break;
        }
      }
    }

    // add emitter
    {
      plAddObjectCommand cmd;
      cmd.m_Parent = systemGuid;
      cmd.m_Index = -1;
      cmd.m_pType = plGetStaticRTTI<plParticleEmitterFactory_Continuous>();
      cmd.m_sParentProperty = "Emitters";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }
    }

    // add cone velocity initializer
    {
      plAddObjectCommand cmd;
      cmd.m_Parent = systemGuid;
      cmd.m_Index = -1;
      cmd.m_pType = plGetStaticRTTI<plParticleInitializerFactory_VelocityCone>();
      cmd.m_sParentProperty = "Initializers";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }

      const plDocumentObject* pConeObject = GetDocument()->GetObjectAccessor()->GetObject(cmd.m_NewObjectGuid);

      // default speed
      {
        const plHybridArray<plDocumentObject*, 8>& children = pConeObject->GetChildren();

        for (auto pChild : children)
        {
          if (plStringUtils::IsEqual(pChild->GetParentProperty(), "Speed"))
          {
            plSetObjectPropertyCommand cmd2;
            cmd2.m_Object = pChild->GetGuid();
            cmd2.m_NewValue = 4.0f;
            cmd2.m_sProperty = "Value";
            cmd2.m_Index = 0;

            if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
            {
              GetDocument()->GetObjectAccessor()->CancelTransaction();
              return;
            }

            break;
          }
        }
      }
    }

    // add color initializer
    {
      plAddObjectCommand cmd;
      cmd.m_Parent = systemGuid;
      cmd.m_Index = -1;
      cmd.m_pType = plGetStaticRTTI<plParticleInitializerFactory_RandomColor>();
      cmd.m_sParentProperty = "Initializers";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }

      // color 1
      {
        plSetObjectPropertyCommand cmd2;
        cmd2.m_Object = cmd.m_NewObjectGuid;
        cmd2.m_sProperty = "Color1";
        cmd2.m_NewValue = plColor::Red;

        if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
        {
          GetDocument()->GetObjectAccessor()->CancelTransaction();
          return;
        }
      }

      // color 2
      {
        plSetObjectPropertyCommand cmd2;
        cmd2.m_Object = cmd.m_NewObjectGuid;
        cmd2.m_sProperty = "Color2";
        cmd2.m_NewValue = plColor::Yellow;

        if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
        {
          GetDocument()->GetObjectAccessor()->CancelTransaction();
          return;
        }
      }
    }

    // add gravity behavior
    {
      plAddObjectCommand cmd;
      cmd.m_Parent = systemGuid;
      cmd.m_Index = -1;
      cmd.m_pType = plGetStaticRTTI<plParticleBehaviorFactory_Gravity>();
      cmd.m_sParentProperty = "Behaviors";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }
    }

    // add quad renderer
    {
      plAddObjectCommand cmd;
      cmd.m_Parent = systemGuid;
      cmd.m_Index = -1;
      cmd.m_pType = plGetStaticRTTI<plParticleTypeQuadFactory>();
      cmd.m_sParentProperty = "Types";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }
    }
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();
}

void plQtParticleEffectAssetDocumentWindow::onRemoveSystem(bool)
{
  const int index = m_pSystemsCombo->findText(m_sSelectedSystem.GetData());
  if (index < 0)
    return;

  const plDocumentObject* pObject = static_cast<plDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

  plDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  GetDocument()->GetObjectAccessor()->StartTransaction("Rename Particle System");

  plRemoveObjectCommand cmd;
  cmd.m_Object = pObject->GetGuid();

  if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
  {
    GetDocument()->GetObjectAccessor()->CancelTransaction();
    return;
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();
}

void plQtParticleEffectAssetDocumentWindow::onRenameSystem(bool)
{
  const int index = m_pSystemsCombo->findText(m_sSelectedSystem.GetData());
  if (index < 0)
    return;

  const plDocumentObject* pObject = static_cast<plDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

  bool ok = false;
  const QString sOrgName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<plString>().GetData();
  QString sName;

  while (true)
  {
    sName = QInputDialog::getText(this, "Rename Particle System", "Name:", QLineEdit::Normal, sOrgName, &ok);

    if (!ok || sName == sOrgName)
      return;

    if (sName.isEmpty())
    {
      plQtUiServices::GetSingleton()->MessageBoxInformation("Invalid particle system name.");
      continue;
    }

    if (m_ParticleSystems.Find(sName.toUtf8().data()).IsValid())
    {
      plQtUiServices::GetSingleton()->MessageBoxInformation("A particle system with this name exists already.");
      continue;
    }

    break;
  }

  plDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  m_sSelectedSystem = sName.toUtf8().data();

  GetDocument()->GetObjectAccessor()->StartTransaction("Rename Particle System");

  plSetObjectPropertyCommand cmd2;
  cmd2.m_Object = pObject->GetGuid();
  cmd2.m_NewValue = sName.toUtf8().data();
  cmd2.m_sProperty = "Name";
  cmd2.m_Index = 0;

  if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
  {
    GetDocument()->GetObjectAccessor()->CancelTransaction();
    return;
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();
}

void plQtParticleEffectAssetDocumentWindow::SendLiveResourcePreview()
{
  if (PlasmaEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  plResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Particle Effect";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plContiguousMemoryStreamStorage streamStorage;
  plMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  plStringBuilder sAbsFilePath = GetParticleDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("plParticleEffect");

  // Write Header
  memoryWriter << sAbsFilePath;
  const plUInt64 uiHash = plAssetCurator::GetSingleton()->GetAssetDependencyHash(GetParticleDocument()->GetGuid());
  plAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetParticleDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  GetParticleDocument()->WriteResource(memoryWriter);
  msg.m_Data = plArrayPtr<const plUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtParticleEffectAssetDocumentWindow::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "Name" || e.m_sProperty == "ParticleSystems")
  {
    UpdateSystemList();
  }

  SendLiveResourcePreview();
}

void plQtParticleEffectAssetDocumentWindow::StructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case plDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case plDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    case plDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      UpdateSystemList();
      SendLiveResourcePreview();
      break;

    default:
      break;
  }
}


void plQtParticleEffectAssetDocumentWindow::ParticleEventHandler(const plParticleEffectAssetEvent& e)
{
  switch (e.m_Type)
  {
    case plParticleEffectAssetEvent::RestartEffect:
    {
      PlasmaEditorEngineRestartSimulationMsg msg;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;

    case plParticleEffectAssetEvent::AutoRestartChanged:
    {
      PlasmaEditorEngineLoopAnimationMsg msg;
      msg.m_bLoop = GetParticleDocument()->GetAutoRestart();
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;

    default:
      break;
  }
}

void plQtParticleEffectAssetDocumentWindow::UpdateSystemList()
{
  plMap<plString, plDocumentObject*> newParticleSystems;

  plDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  plStringBuilder s;

  for (plDocumentObject* pChild : pRootObject->GetChildren())
  {
    if (plStringUtils::IsEqual(pChild->GetParentProperty(), "ParticleSystems"))
    {
      s = pChild->GetTypeAccessor().GetValue("Name").ConvertTo<plString>();
      newParticleSystems[s] = pChild;
    }
  }

  // early out
  if (m_ParticleSystems == newParticleSystems)
    return;

  m_ParticleSystems.Swap(newParticleSystems);

  {
    plQtScopedBlockSignals _1(m_pSystemsCombo);
    m_pSystemsCombo->clear();

    for (auto it = m_ParticleSystems.GetIterator(); it.IsValid(); ++it)
    {
      m_pSystemsCombo->addItem(it.Key().GetData(), QVariant::fromValue<void*>(it.Value()));
    }
  }

  if (!m_ParticleSystems.Find(m_sSelectedSystem).IsValid())
    m_sSelectedSystem.Clear();

  if (m_sSelectedSystem.IsEmpty() && !m_ParticleSystems.IsEmpty())
    m_sSelectedSystem = m_ParticleSystems.GetIterator().Key();

  if (!m_ParticleSystems.IsEmpty())
  {
    SelectSystem(m_ParticleSystems[m_sSelectedSystem]);
  }
  else
  {
    SelectSystem(nullptr);
  }

  const bool hasSelection = !m_ParticleSystems.IsEmpty();

  m_pSystemsCombo->setEnabled(hasSelection);
  m_pRemoveSystem->setEnabled(hasSelection);
  m_pRenameSystem->setEnabled(hasSelection);
}


void plQtParticleEffectAssetDocumentWindow::InternalRedraw()
{
  PlasmaEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  plQtEngineDocumentWindow::InternalRedraw();
}


void plQtParticleEffectAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (PlasmaEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    plSimulationSettingsMsgToEngine msg;
    msg.m_bSimulateWorld = !GetParticleDocument()->GetSimulationPaused();
    msg.m_fSimulationSpeed = GetParticleDocument()->GetSimulationSpeed();
    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void plQtParticleEffectAssetDocumentWindow::RestoreResource()
{
  plRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Particle Effect";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
