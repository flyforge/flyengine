#include <EditorTest/EditorTestPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorTest/SceneDocument/SceneDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <RendererCore/Lights/SphereReflectionProbeComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#include <QMimeData>

static PlasmaEditorSceneDocumentTest s_EditorSceneDocumentTest;

const char* PlasmaEditorSceneDocumentTest::GetTestName() const
{
  return "Scene Document Tests";
}

void PlasmaEditorSceneDocumentTest::SetupSubTests()
{
  AddSubTest("Layer Operations", SubTests::ST_LayerOperations);
  AddSubTest("Prefab Operations", SubTests::ST_PrefabOperations);
  AddSubTest("Component Operations", SubTests::ST_ComponentOperations);
  AddSubTest("Object Property Path", SubTests::ST_ObjectPropertyPath);
}

plResult PlasmaEditorSceneDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return PLASMA_FAILURE;

  if (SUPER::CreateAndLoadProject("SceneTestProject").Failed())
    return PLASMA_FAILURE;

  if (plStatus res = plAssetCurator::GetSingleton()->TransformAllAssets(plTransformFlags::None); res.Failed())
  {
    plLog::Error("Asset transform failed: {}", res.m_sMessage);
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult PlasmaEditorSceneDocumentTest::DeInitializeTest()
{
  m_pDoc = nullptr;
  m_pLayer = nullptr;
  m_SceneGuid.SetInvalid();
  m_LayerGuid.SetInvalid();

  if (SUPER::DeInitializeTest().Failed())
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

plTestAppRun PlasmaEditorSceneDocumentTest::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_LayerOperations:
      LayerOperations();
      break;
    case SubTests::ST_PrefabOperations:
      PrefabOperations();
      break;
    case SubTests::ST_ComponentOperations:
      ComponentOperations();
      break;
    // case SubTests::ST_ObjectPropertyPath:
    //   ObjectPropertyPath();
    //   break;
  }
  return plTestAppRun::Quit;
}

plResult PlasmaEditorSceneDocumentTest::CreateSimpleScene(const char* szSceneName)
{
  plStringBuilder sName;
  sName = m_sProjectPath;
  sName.AppendPath(szSceneName);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Create Document")
  {
    m_pDoc = static_cast<plScene2Document*>(m_pApplication->m_pEditorApp->CreateDocument(sName, plDocumentFlags::RequestWindow));
    if (!PLASMA_TEST_BOOL(m_pDoc != nullptr))
      return PLASMA_FAILURE;

    m_SceneGuid = m_pDoc->GetGuid();
    ProcessEvents();
    PLASMA_TEST_STATUS(m_pDoc->CreateLayer("Layer1", m_LayerGuid));
    m_pLayer = plDynamicCast<plLayerDocument*>(m_pDoc->GetLayerDocument(m_LayerGuid));
    if (!PLASMA_TEST_BOOL(m_pLayer != nullptr))
      return PLASMA_FAILURE;
  }
  return PLASMA_SUCCESS;
}

void PlasmaEditorSceneDocumentTest::CloseSimpleScene()
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Close Document")
  {
    bool bSaved = false;
    plTaskGroupID id = m_pDoc->SaveDocumentAsync(
      [&bSaved](plDocument* pDoc, plStatus res) {
        bSaved = true;
      },
      true);

    m_pDoc->GetDocumentManager()->CloseDocument(m_pDoc);
    PLASMA_TEST_BOOL(plTaskSystem::IsTaskGroupFinished(id));
    PLASMA_TEST_BOOL(bSaved);
    m_pDoc = nullptr;
    m_pLayer = nullptr;
    m_SceneGuid.SetInvalid();
    m_LayerGuid.SetInvalid();
  }
}

void PlasmaEditorSceneDocumentTest::LayerOperations()
{
  plStringBuilder sName;
  sName = m_sProjectPath;
  sName.AppendPath("LayerOperations.plScene");

  plScene2Document* pDoc = nullptr;
  plEventSubscriptionID layerEventsID = 0;
  plHybridArray<plScene2LayerEvent, 2> expectedEvents;
  plUuid sceneGuid;
  plUuid layer1Guid;
  plLayerDocument* pLayer1 = nullptr;

  auto TestLayerEvents = [&expectedEvents](const plScene2LayerEvent& e) {
    if (PLASMA_TEST_BOOL(!expectedEvents.IsEmpty()))
    {
      // If we pass in an invalid guid it's considered fine as we might not know the ID, e.g. when creating a layer.
      PLASMA_TEST_BOOL(!expectedEvents[0].m_layerGuid.IsValid() || expectedEvents[0].m_layerGuid == e.m_layerGuid);
      PLASMA_TEST_BOOL(expectedEvents[0].m_Type == e.m_Type);
      expectedEvents.RemoveAtAndCopy(0);
    }
  };

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Create Document")
  {
    pDoc = static_cast<plScene2Document*>(m_pApplication->m_pEditorApp->CreateDocument(sName, plDocumentFlags::RequestWindow));
    if (!PLASMA_TEST_BOOL(pDoc != nullptr))
      return;

    sceneGuid = pDoc->GetGuid();
    layerEventsID = pDoc->m_LayerEvents.AddEventHandler(TestLayerEvents);
    ProcessEvents();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Create Layer")
  {
    PLASMA_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    PLASMA_TEST_BOOL(pDoc->IsLayerVisible(sceneGuid));
    PLASMA_TEST_BOOL(pDoc->IsLayerLoaded(sceneGuid));

    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerAdded, plUuid()});
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerLoaded, plUuid()});
    PLASMA_TEST_STATUS(pDoc->CreateLayer("Layer1", layer1Guid));

    expectedEvents.PushBack({plScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));
    PLASMA_TEST_BOOL(pDoc->GetActiveLayer() == layer1Guid);
    PLASMA_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    PLASMA_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));
    pLayer1 = plDynamicCast<plLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    PLASMA_TEST_BOOL(pLayer1 != nullptr);

    plHybridArray<plSceneDocument*, 2> layers;
    pDoc->GetLoadedLayers(layers);
    PLASMA_TEST_INT(layers.GetCount(), 2);
    PLASMA_TEST_BOOL(layers.Contains(pLayer1));
    PLASMA_TEST_BOOL(layers.Contains(pDoc));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Undo/Redo Layer Creation")
  {
    // Undo / redo
    expectedEvents.PushBack({plScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    PLASMA_TEST_STATUS(pDoc->SetActiveLayer(sceneGuid));
    // Initial scene setup exists in the scene undo stack
    const plUInt32 uiInitialUndoStackSize = pDoc->GetCommandHistory()->GetUndoStackSize();
    PLASMA_TEST_BOOL(uiInitialUndoStackSize >= 1);
    PLASMA_TEST_INT(pDoc->GetSceneCommandHistory()->GetUndoStackSize(), uiInitialUndoStackSize);
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->GetCommandHistory()->Undo(1));
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerAdded, layer1Guid});
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->GetCommandHistory()->Redo(1));

    pLayer1 = plDynamicCast<plLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    PLASMA_TEST_BOOL(pLayer1 != nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Save and Close Document")
  {
    bool bSaved = false;
    plTaskGroupID id = pDoc->SaveDocumentAsync(
      [&bSaved](plDocument* pDoc, plStatus res) {
        bSaved = true;
      },
      true);

    pDoc->m_LayerEvents.RemoveEventHandler(layerEventsID);
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    PLASMA_TEST_BOOL(plTaskSystem::IsTaskGroupFinished(id));
    PLASMA_TEST_BOOL(bSaved);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Reload Document")
  {
    pDoc = static_cast<plScene2Document*>(m_pApplication->m_pEditorApp->OpenDocument(sName, plDocumentFlags::RequestWindow));
    if (!PLASMA_TEST_BOOL(pDoc != nullptr))
      return;
    layerEventsID = pDoc->m_LayerEvents.AddEventHandler(TestLayerEvents);
    ProcessEvents();

    PLASMA_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    PLASMA_TEST_BOOL(pDoc->IsLayerVisible(sceneGuid));
    PLASMA_TEST_BOOL(pDoc->IsLayerLoaded(sceneGuid));

    pLayer1 = plDynamicCast<plLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    plHybridArray<plSceneDocument*, 2> layers;
    pDoc->GetLoadedLayers(layers);
    PLASMA_TEST_INT(layers.GetCount(), 2);
    PLASMA_TEST_BOOL(layers.Contains(pLayer1));
    PLASMA_TEST_BOOL(layers.Contains(pDoc));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Toggle Layer Visibility")
  {
    PLASMA_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    expectedEvents.PushBack({plScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));
    PLASMA_TEST_BOOL(pDoc->GetActiveLayer() == layer1Guid);
    PLASMA_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    PLASMA_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));

    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerInvisible, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, false));
    PLASMA_TEST_BOOL(!pDoc->IsLayerVisible(layer1Guid));
    ProcessEvents();
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerVisible, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, true));
    PLASMA_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    ProcessEvents();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Toggle Layer Loaded")
  {
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerInvisible, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, false));

    expectedEvents.PushBack({plScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->SetLayerLoaded(layer1Guid, false));

    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->SetLayerLoaded(layer1Guid, true));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Delete Layer")
  {
    expectedEvents.PushBack({plScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));

    expectedEvents.PushBack({plScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->DeleteLayer(layer1Guid));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Undo/Redo Layer Deletion")
  {
    PLASMA_TEST_INT(pDoc->GetCommandHistory()->GetUndoStackSize(), 1);
    PLASMA_TEST_INT(pDoc->GetSceneCommandHistory()->GetUndoStackSize(), 1);
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerAdded, layer1Guid});
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->GetCommandHistory()->Undo(1));
    PLASMA_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    PLASMA_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));

    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({plScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    PLASMA_TEST_STATUS(pDoc->GetCommandHistory()->Redo(1));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Close Document")
  {
    bool bSaved = false;
    plTaskGroupID id = pDoc->SaveDocumentAsync(
      [&bSaved](plDocument* pDoc, plStatus res) {
        bSaved = true;
      },
      true);

    pDoc->m_LayerEvents.RemoveEventHandler(layerEventsID);
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    PLASMA_TEST_BOOL(plTaskSystem::IsTaskGroupFinished(id));
    PLASMA_TEST_BOOL(bSaved);
  }
}


void PlasmaEditorSceneDocumentTest::PrefabOperations()
{
  if (CreateSimpleScene("PrefabOperations.plScene").Failed())
    return;

  const plDocumentObject* pPrefab1 = nullptr;
  const plDocumentObject* pPrefab2 = nullptr;
  auto pAccessor = m_pDoc->GetObjectAccessor();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Drag&Drop Prefabs")
  {
    const char* szSpherePrefab = "{ a3ce5d3d-be5e-4bda-8820-b1ce3b3d33fd }";
    pPrefab1 = DropAsset(m_pDoc, szSpherePrefab);
    PLASMA_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
    PLASMA_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));
    pPrefab2 = DropAsset(m_pDoc, szSpherePrefab, true);
    PLASMA_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
    PLASMA_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Create Nodes and check default state")
  {
    const char* szSphereMesh = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }";
    const plDocumentObject* pSphere1 = DropAsset(m_pDoc, szSphereMesh);
    const plDocumentObject* pSphere2 = DropAsset(m_pDoc, szSphereMesh);

    pAccessor->StartTransaction("Modify objects");
    PLASMA_TEST_STATUS(pAccessor->SetValue(pSphere1, "Name", "Sphere1"));
    PLASMA_TEST_STATUS(pAccessor->SetValue(pSphere1, "LocalPosition", plVec3(1.0f, 0.0f, 0.0f)));
    PLASMA_TEST_STATUS(pAccessor->SetValue(pSphere1, "LocalRotation", plQuat(1.0f, 0.0f, 0.0f, 0.0f)));
    PLASMA_TEST_STATUS(pAccessor->SetValue(pSphere1, "LocalScaling", plVec3(1.0f, 2.0f, 3.0f)));
    PLASMA_TEST_STATUS(pAccessor->InsertValue(pSphere1, "Tags", "SkyLight", -1));
    const plDocumentObject* pMeshComponent = pAccessor->GetObject(pAccessor->Get<plVariantArray>(pSphere1, "Components")[0].Get<plUuid>());
    PLASMA_TEST_STATUS(pAccessor->InsertValue(pMeshComponent, "Materials", "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }", 0));

    plUuid pSphereRef;
    PLASMA_TEST_STATUS(pAccessor->AddObject(pSphere1, "Components", -1, plGetStaticRTTI<plSphereReflectionProbeComponent>(), pSphereRef));

    pAccessor->FinishTransaction();

    {
      // Check that modifications above changed properties from their default state.
      plHybridArray<plPropertySelection, 1> selection;
      selection.PushBack({pSphere1, plVariant()});
      plDefaultObjectState defaultState(pAccessor, selection);
      PLASMA_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");

      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("Name"));
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("LocalPosition"));
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("LocalRotation"));
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("LocalScaling"));
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("Tags"));
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("Components"));

      // Does default state match that of pSphere2 which is unmodified?
      auto MatchesDefaultValue = [&](plDefaultObjectState& ref_defaultState, const char* szProperty) {
        plVariant defaultValue = ref_defaultState.GetDefaultValue(szProperty);
        plVariant sphere2value;
        PLASMA_TEST_STATUS(pAccessor->GetValue(pSphere2, szProperty, sphere2value));
        PLASMA_TEST_BOOL(defaultValue == sphere2value);
      };

      MatchesDefaultValue(defaultState, "Name");
      MatchesDefaultValue(defaultState, "LocalPosition");
      MatchesDefaultValue(defaultState, "LocalRotation");
      MatchesDefaultValue(defaultState, "LocalScaling");
      MatchesDefaultValue(defaultState, "Tags");
    }

    {
      // pSphere2 should be unmodified except for the component array.
      plHybridArray<plPropertySelection, 1> selection;
      selection.PushBack({pSphere2, plVariant()});
      plDefaultObjectState defaultState(pAccessor, selection);
      PLASMA_TEST_BOOL(defaultState.IsDefaultValue("Name"));
      PLASMA_TEST_BOOL(defaultState.IsDefaultValue("LocalPosition"));
      PLASMA_TEST_BOOL(defaultState.IsDefaultValue("LocalRotation"));
      PLASMA_TEST_BOOL(defaultState.IsDefaultValue("LocalScaling"));
      PLASMA_TEST_BOOL(defaultState.IsDefaultValue("Tags"));
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("Components"));
    }

    {
      // Multi-selection should not be default if one in the selection is not.
      plHybridArray<plPropertySelection, 1> selection;
      selection.PushBack({pSphere1, plVariant()});
      selection.PushBack({pSphere2, plVariant()});
      plDefaultObjectState defaultState(pAccessor, selection);
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("Name"));
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("LocalPosition"));
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("LocalRotation"));
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("LocalScaling"));
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("Tags"));
      PLASMA_TEST_BOOL(!defaultState.IsDefaultValue("Components"));
    }

    {
      // Default state object array
      plHybridArray<plPropertySelection, 1> selection;
      selection.PushBack({pSphere1, plVariant()});
      plDefaultContainerState defaultState(pAccessor, selection, "Components");
      PLASMA_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");
      PLASMA_TEST_BOOL(defaultState.GetDefaultContainer() == plVariantArray());
      PLASMA_TEST_BOOL(defaultState.GetDefaultElement(0) == plUuid());
      PLASMA_TEST_BOOL(!defaultState.IsDefaultContainer());
      // We currently do not supporting reverting an index of a non-value type container. Thus, they are always the default state.
      PLASMA_TEST_BOOL(defaultState.IsDefaultElement(0));
      PLASMA_TEST_BOOL(defaultState.IsDefaultElement(1));
    }

    {
      // Default state value array
      plHybridArray<plPropertySelection, 1> selection;
      selection.PushBack({pMeshComponent, plVariant()});
      plDefaultContainerState defaultState(pAccessor, selection, "Materials");
      PLASMA_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");
      PLASMA_TEST_BOOL(defaultState.GetDefaultContainer() == plVariantArray());
      PLASMA_TEST_BOOL(defaultState.GetDefaultElement(0) == "");
      PLASMA_TEST_BOOL(!defaultState.IsDefaultContainer());
      PLASMA_TEST_BOOL(!defaultState.IsDefaultElement(0));

      plDefaultObjectState defaultObjectState(pAccessor, selection);
      PLASMA_TEST_STRING(defaultObjectState.GetStateProviderName(), "Attribute");
      PLASMA_TEST_BOOL(defaultObjectState.GetDefaultValue("Materials") == plVariantArray());
      PLASMA_TEST_BOOL(!defaultObjectState.IsDefaultValue("Materials"));
    }

    plDeque<const plDocumentObject*> selection;
    selection.PushBack(pSphere1);
    selection.PushBack(pSphere2);
    m_pDoc->GetSelectionManager()->SetSelection(selection);
  }

  plUuid prefabGuid;
  const plDocumentObject* pPrefab3 = nullptr;
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Create Prefab from Selection")
  {
    // ProcessEvents(999999999);

    plStringBuilder sPrefabName;
    sPrefabName = m_sProjectPath;
    sPrefabName.AppendPath("Spheres.plPrefab");
    PLASMA_TEST_BOOL(m_pDoc->CreatePrefabDocumentFromSelection(sPrefabName, plGetStaticRTTI<plGameObject>(), {}, {}, [](plAbstractObjectGraph& graph, plDynamicArray<plAbstractObjectNode*>&) { /* do nothing */ }).Succeeded());
    m_pDoc->ScheduleSendObjectSelection();
    pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
    PLASMA_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid()));
    PLASMA_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid(), &prefabGuid));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move Prefabs to Layer")
  {
    PLASMA_TEST_BOOL(pPrefab1->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());
    PLASMA_TEST_BOOL(pPrefab2->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());
    PLASMA_TEST_BOOL(pPrefab3->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());

    // Copy & paste should retain the order in the tree view, not the selection array so we push the elements in a random order here.
    plDeque<const plDocumentObject*> assets;
    assets.PushBack(pPrefab3);
    assets.PushBack(pPrefab1);
    assets.PushBack(pPrefab2);
    plDeque<const plDocumentObject*> newObjects;

    MoveObjectsToLayer(m_pDoc, assets, m_LayerGuid, newObjects);

    PLASMA_TEST_BOOL(m_pDoc->GetActiveLayer() == m_SceneGuid);
    PLASMA_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab1->GetGuid()) == nullptr);
    PLASMA_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab2->GetGuid()) == nullptr);
    PLASMA_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab3->GetGuid()) == nullptr);

    PLASMA_TEST_INT(newObjects.GetCount(), assets.GetCount());
    pPrefab1 = newObjects[0];
    pPrefab2 = newObjects[1];
    pPrefab3 = newObjects[2];

    PLASMA_TEST_STATUS(m_pDoc->SetActiveLayer(m_LayerGuid));

    PLASMA_TEST_BOOL(pPrefab1->GetDocumentObjectManager() == m_pLayer->GetObjectManager());
    PLASMA_TEST_BOOL(pPrefab2->GetDocumentObjectManager() == m_pLayer->GetObjectManager());
    PLASMA_TEST_BOOL(pPrefab3->GetDocumentObjectManager() == m_pLayer->GetObjectManager());

    PLASMA_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
    PLASMA_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));
    PLASMA_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
    PLASMA_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
    PLASMA_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid()));
    plUuid prefabGuidOut;
    PLASMA_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid(), &prefabGuidOut));
    PLASMA_TEST_BOOL(prefabGuid == prefabGuidOut);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Change Prefab Type")
  {
    plVariant oldIndex = pPrefab1->GetPropertyIndex();
    plDeque<const plDocumentObject*> selection;
    {
      selection.PushBack(pPrefab1);
      m_pDoc->ConvertToEditorPrefab(selection);
      pPrefab1 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      PLASMA_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
      PLASMA_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));
      PLASMA_TEST_BOOL(oldIndex == pPrefab1->GetPropertyIndex());
    }

    {
      oldIndex = pPrefab2->GetPropertyIndex();
      selection.Clear();
      selection.PushBack(pPrefab2);
      m_pDoc->ConvertToEnginePrefab(selection);
      pPrefab2 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      PLASMA_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
      PLASMA_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
      PLASMA_TEST_BOOL(oldIndex == pPrefab2->GetPropertyIndex());
    }

    {
      oldIndex = pPrefab3->GetPropertyIndex();
      selection.Clear();
      selection.PushBack(pPrefab3);
      m_pDoc->ConvertToEditorPrefab(selection);
      pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      PLASMA_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid()));
      plUuid prefabGuidOut;
      PLASMA_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid(), &prefabGuidOut));
      PLASMA_TEST_BOOL(prefabGuid == prefabGuidOut);
      PLASMA_TEST_BOOL(oldIndex == pPrefab3->GetPropertyIndex());
    }
  }

  auto IsObjectDefault = [&](const plDocumentObject* pChild) {
    plHybridArray<plPropertySelection, 1> selection;
    selection.PushBack({pChild, plVariant()});
    plDefaultObjectState defaultState(pAccessor, selection);
    // The root node of the prefab is not actually part of the prefab in the sense that it is just the container and does not actually exist in the prefab itself.
    const char* szExpectedProvider = pChild == pPrefab3 ? "Attribute" : "Prefab";
    PLASMA_TEST_STRING(defaultState.GetStateProviderName(), szExpectedProvider);

    plHybridArray<const plAbstractProperty*, 32> properties;
    pChild->GetType()->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(plPropertyFlags::Hidden | plPropertyFlags::ReadOnly))
        continue;

      PLASMA_TEST_BOOL(defaultState.IsDefaultValue(pProp));
    }
  };

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Modify Editor Prefab")
  {
    plVariant oldIndex = pPrefab3->GetPropertyIndex();
    const plAbstractProperty* pProp = pPrefab3->GetType()->FindPropertyByName("Children");
    const plAbstractProperty* pCompProp = pPrefab3->GetType()->FindPropertyByName("Components");

    // ProcessEvents(999999999);

    CheckHierarchy(pAccessor, pPrefab3, IsObjectDefault);

    // ProcessEvents(999999999);
    {
      m_pDoc->UpdatePrefabs();
      // Update prefabs replaces object instances with new ones with the same IDs so the old ones are in the undo history now.
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Remove part of the prefab
      plHybridArray<plVariant, 16> values;
      PLASMA_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      PLASMA_TEST_INT(values.GetCount(), 2);
      const plDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<plUuid>());
      const plDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<plUuid>());
      pAccessor->StartTransaction("Delete child0");
      pAccessor->RemoveObject(pChild1).AssertSuccess();
      pAccessor->FinishTransaction();
      PLASMA_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 1);
    }

    {
      m_pDoc->UpdatePrefabs();
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Revert prefab
      plDeque<const plDocumentObject*> selection;
      selection.PushBack(pPrefab3);
      m_pDoc->RevertPrefabs(selection);

      pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      plUuid prefabGuidOut;
      PLASMA_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid(), &prefabGuidOut));
      PLASMA_TEST_BOOL(prefabGuid == prefabGuidOut);
      PLASMA_TEST_BOOL(oldIndex == pPrefab3->GetPropertyIndex());
      PLASMA_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 2);
    }

    {
      // Modify the prefab
      plHybridArray<plVariant, 16> values;
      PLASMA_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      PLASMA_TEST_INT(values.GetCount(), 2);
      const plDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<plUuid>());
      const plDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<plUuid>());

      pAccessor->StartTransaction("Modify Prefab");
      plUuid compGuid;
      PLASMA_TEST_STATUS(pAccessor->AddObject(pChild0, "Components", -1, plRTTI::FindTypeByName("plBeamComponent"), compGuid));
      const plDocumentObject* pComp = pAccessor->GetObject(compGuid);

      const plDocumentObject* pChild1Comp = pAccessor->GetChildObject(pChild1, "Components", 0);
      PLASMA_TEST_STATUS(pAccessor->RemoveObject(pChild1Comp));
      pAccessor->FinishTransaction();

      PLASMA_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 2);
      PLASMA_TEST_INT(pAccessor->GetCount(pChild0, pCompProp), 3);
      PLASMA_TEST_INT(pAccessor->GetCount(pChild1, pCompProp), 0);

      // Check default states
      {
        plHybridArray<plPropertySelection, 1> selection;
        selection.PushBack({pChild0, plVariant()});
        plDefaultContainerState defaultObjectState(pAccessor, selection, "Components");
        PLASMA_TEST_STRING(defaultObjectState.GetStateProviderName(), "Prefab");
        PLASMA_TEST_BOOL(!defaultObjectState.IsDefaultContainer());
      }

      {
        plHybridArray<plPropertySelection, 1> selection;
        selection.PushBack({pChild1, plVariant()});
        plDefaultContainerState defaultObjectState(pAccessor, selection, "Components");
        PLASMA_TEST_STRING(defaultObjectState.GetStateProviderName(), "Prefab");
        PLASMA_TEST_BOOL(!defaultObjectState.IsDefaultContainer());
      }

      {
        plHybridArray<plPropertySelection, 1> selection;
        selection.PushBack({pComp, plVariant()});
        plDefaultObjectState defaultObjectState(pAccessor, selection);
        PLASMA_TEST_STRING(defaultObjectState.GetStateProviderName(), "Attribute");
      }
    }

    {
      m_pDoc->UpdatePrefabs();
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Revert via default state
      const plDocumentObject* pChild1 = pAccessor->GetChildObject(pPrefab3, "Children", 0);
      const plDocumentObject* pChild2 = pAccessor->GetChildObject(pPrefab3, "Children", 1);
      {
        plHybridArray<plPropertySelection, 1> selection;
        selection.PushBack({pChild1, plVariant()});
        selection.PushBack({pChild2, plVariant()});
        plDefaultContainerState defaultState(pAccessor, selection, "Components");

        pAccessor->StartTransaction("Revert children");
        defaultState.RevertContainer().AssertSuccess();
        pAccessor->FinishTransaction();
      }
    }

    {
      // Verify prefab was reverted
      plHybridArray<plVariant, 16> values;
      PLASMA_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      PLASMA_TEST_INT(values.GetCount(), 2);
      const plDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<plUuid>());
      const plDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<plUuid>());

      values.Clear();
      PLASMA_TEST_STATUS(pAccessor->GetValues(pChild0, "Components", values));
      PLASMA_TEST_INT(values.GetCount(), 2);

      PLASMA_TEST_STATUS(pAccessor->GetValues(pChild1, "Components", values));
      PLASMA_TEST_INT(values.GetCount(), 1);

      CheckHierarchy(pAccessor, pPrefab3, IsObjectDefault);
    }
  }

  ProcessEvents(10);
  CloseSimpleScene();
}

void PlasmaEditorSceneDocumentTest::ComponentOperations()
{
  if (CreateSimpleScene("ComponentOperations.plScene").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();

  const plDocumentObject* pRoot = CreateGameObject(m_pDoc);

  plDeque<const plDocumentObject*> selection;
  selection.PushBack(pRoot);
  m_pDoc->GetSelectionManager()->SetSelection(selection);

  auto CreateComponent = [&](const plRTTI* pType, const plDocumentObject* pParent) -> const plDocumentObject* {
    plUuid compGuid;
    PLASMA_TEST_STATUS(pAccessor->AddObject(pParent, "Components", -1, pType, compGuid));
    return pAccessor->GetObject(compGuid);
  };

  auto IsObjectDefault = [&](const plDocumentObject* pChild) {
    plHybridArray<plPropertySelection, 1> selection;
    selection.PushBack({pChild, plVariant()});
    plDefaultObjectState defaultState(pAccessor, selection);

    plHybridArray<const plAbstractProperty*, 32> properties;
    pChild->GetType()->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(plPropertyFlags::Hidden | plPropertyFlags::ReadOnly))
        continue;

      plVariant defaultValue = defaultState.GetDefaultValue(pProp);
      PLASMA_TEST_BOOL(plDefaultStateProvider::DoesVariantMatchProperty(defaultValue, pProp));
      plVariant currentValue;
      PLASMA_TEST_STATUS(pAccessor->GetValue(pChild, pProp, currentValue));
      PLASMA_TEST_BOOL(plDefaultStateProvider::DoesVariantMatchProperty(currentValue, pProp));
      PLASMA_TEST_BOOL(defaultValue == currentValue);
      PLASMA_TEST_BOOL(defaultState.IsDefaultValue(pProp));
    }
  };

  plDynamicArray<const plRTTI*> componentTypes;
  plRTTI::ForEachDerivedType<plComponent>([&](const plRTTI* pRtti) { componentTypes.PushBack(pRtti); });

  plSet<const plRTTI*> blacklist;
  // The scene already has one and the code asserts otherwise. There needs to be a general way of preventing two settings components from existing at the same time.
  blacklist.Insert(plRTTI::FindTypeByName("plSkyLightComponent"));

  pAccessor->StartTransaction("Modify objects");

  for (auto pType : componentTypes)
  {
    if (pType->GetTypeFlags().IsSet(plTypeFlags::Abstract) || blacklist.Contains(pType))
      continue;

    auto pComp = CreateComponent(pType, pRoot);

    CheckHierarchy(pAccessor, pComp, IsObjectDefault);
  }

  pAccessor->FinishTransaction();
  
      ProcessEvents(10);
  CloseSimpleScene();
}


void PlasmaEditorSceneDocumentTest::CheckHierarchy(plObjectAccessorBase* pAccessor, const plDocumentObject* pRoot, plDelegate<void(const plDocumentObject* pChild)> functor)
{
  plDeque<const plDocumentObject*> objects;
  objects.PushBack(pRoot);
  while (!objects.IsEmpty())
  {
    const plDocumentObject* pCurrent = objects[0];
    objects.PopFront();
    {
      functor(pCurrent);
    }

    for (auto pChild : pCurrent->GetChildren())
    {
      objects.PushBack(pChild);
    }
  }
}
