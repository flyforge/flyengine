#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Dialogs/DeltaTransformDlg.moc.h>
#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <QClipboard>
#include <RendererCore/Components/CameraComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectDirectAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneDocument, 7, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

void plSceneDocument_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  static const plRTTI* pRtti = plRTTI::FindTypeByName("plGameObject");

  if (e.m_pObject->GetDocumentObjectManager()->GetDocument()->GetDocumentTypeName() != "Prefab")
    return;

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  auto pParent = e.m_pObject->GetParent();
  if (pParent != nullptr)
  {
    if (pParent->GetTypeAccessor().GetType() == pRtti)
      return;
  }

  const plString name = e.m_pObject->GetTypeAccessor().GetValue("Name").ConvertTo<plString>();
  if (name != "<Prefab-Root>")
    return;

  auto& props = *e.m_pPropertyStates;
  props["Name"].m_sNewLabelText = "Prefab.NameLabel";
  props["Active"].m_Visibility = plPropertyUiState::Invisible;
  props["LocalPosition"].m_Visibility = plPropertyUiState::Invisible;
  props["LocalRotation"].m_Visibility = plPropertyUiState::Invisible;
  props["LocalScaling"].m_Visibility = plPropertyUiState::Invisible;
  props["LocalUniformScaling"].m_Visibility = plPropertyUiState::Invisible;
  props["GlobalKey"].m_Visibility = plPropertyUiState::Invisible;
  props["Tags"].m_Visibility = plPropertyUiState::Invisible;
}

plSceneDocument::plSceneDocument(plStringView sDocumentPath, DocumentType documentType)
  : plGameObjectDocument(sDocumentPath, PL_DEFAULT_NEW(plSceneObjectManager))
{
  m_DocumentType = documentType;
  m_GameMode = GameMode::Off;
  SetAddAmbientLight(IsPrefab());

  m_GameModeData[GameMode::Off].m_bRenderSelectionOverlay = true;
  m_GameModeData[GameMode::Off].m_bRenderShapeIcons = true;
  m_GameModeData[GameMode::Off].m_bRenderVisualizers = true;

  m_GameModeData[GameMode::Simulate].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Simulate].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Simulate].m_bRenderVisualizers = false;

  m_GameModeData[GameMode::Play].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Play].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Play].m_bRenderVisualizers = false;
}


void plSceneDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  // (Local mirror only mirrors settings)
  m_ObjectMirror.SetFilterFunction([pManager = GetObjectManager()](const plDocumentObject* pObject, plStringView sProperty) -> bool { return pManager->IsUnderRootProperty("Settings", pObject, sProperty); });
  // (Remote IPC mirror only sends scene)
  m_pMirror->SetFilterFunction([pManager = GetObjectManager()](const plDocumentObject* pObject, plStringView sProperty) -> bool { return pManager->IsUnderRootProperty("Children", pObject, sProperty); });

  EnsureSettingsObjectExist();

  m_DocumentObjectMetaData->m_DataModifiedEvent.AddEventHandler(plMakeDelegate(&plSceneDocument::DocumentObjectMetaDataEventHandler, this));
  plToolsProject::s_Events.AddEventHandler(plMakeDelegate(&plSceneDocument::ToolsProjectEventHandler, this));
  plEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(plMakeDelegate(&plSceneDocument::EngineConnectionEventHandler, this));

  m_ObjectMirror.InitSender(GetObjectManager());
  m_ObjectMirror.InitReceiver(&m_Context);
  m_ObjectMirror.SendDocument();
}

plSceneDocument::~plSceneDocument()
{
  m_DocumentObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(plMakeDelegate(&plSceneDocument::DocumentObjectMetaDataEventHandler, this));

  plToolsProject::s_Events.RemoveEventHandler(plMakeDelegate(&plSceneDocument::ToolsProjectEventHandler, this));

  plEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(plMakeDelegate(&plSceneDocument::EngineConnectionEventHandler, this));

  m_ObjectMirror.Clear();
  m_ObjectMirror.DeInit();
}

void plSceneDocument::GroupSelection()
{
  const auto& sel = GetSelectionManager()->GetSelection();
  const plUInt32 numSel = sel.GetCount();
  if (numSel <= 1)
    return;

  plVec3 vCenter(0.0f);
  const plDocumentObject* pCommonParent = sel[0]->GetParent();

  // this happens for top-level objects, their parent object is an plDocumentRootObject
  if (pCommonParent->GetType() != plGetStaticRTTI<plGameObject>())
  {
    pCommonParent = nullptr;
  }

  for (const auto& item : sel)
  {
    vCenter += GetGlobalTransform(item).m_vPosition;

    if (pCommonParent != item->GetParent())
    {
      pCommonParent = nullptr;
    }
  }

  vCenter /= numSel;

  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Group Selection");

  plUuid groupObj = plUuid::MakeUuid();

  plAddObjectCommand cmdAdd;
  cmdAdd.m_NewObjectGuid = groupObj;
  cmdAdd.m_pType = plGetStaticRTTI<plGameObject>();
  cmdAdd.m_Index = -1;
  cmdAdd.m_sParentProperty = "Children";

  pHistory->AddCommand(cmdAdd).AssertSuccess();

  // put the new group object under the shared parent
  if (pCommonParent != nullptr)
  {
    plMoveObjectCommand cmdMove;
    cmdMove.m_NewParent = pCommonParent->GetGuid();
    cmdMove.m_Index = -1;
    cmdMove.m_sParentProperty = "Children";

    cmdMove.m_Object = cmdAdd.m_NewObjectGuid;
    pHistory->AddCommand(cmdMove).AssertSuccess();
  }

  auto pGroupObject = GetObjectManager()->GetObject(cmdAdd.m_NewObjectGuid);
  SetGlobalTransform(pGroupObject, plTransform(vCenter), TransformationChanges::Translation);

  plMoveObjectCommand cmdMove;
  cmdMove.m_NewParent = cmdAdd.m_NewObjectGuid;
  cmdMove.m_Index = -1;
  cmdMove.m_sParentProperty = "Children";

  for (const auto& item : sel)
  {
    cmdMove.m_Object = item->GetGuid();
    pHistory->AddCommand(cmdMove).AssertSuccess();
  }

  pHistory->FinishTransaction();

  const plDocumentObject* pGroupObj = GetObjectManager()->GetObject(groupObj);

  GetSelectionManager()->SetSelection(pGroupObj);

  ShowDocumentStatus(plFmt("Grouped {} objects", numSel));
}


void plSceneDocument::DuplicateSpecial()
{
  if (GetSelectionManager()->IsSelectionEmpty())
    return;

  plQtDuplicateDlg dlg(nullptr);
  if (dlg.exec() == QDialog::Rejected)
    return;

  plMap<plUuid, plUuid> parents;

  plAbstractObjectGraph graph;
  CopySelectedObjects(graph, &parents);

  plStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", plConversionUtils::ToString(it.Key(), tmp1), plConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  plContiguousMemoryStreamStorage streamStorage;
  plMemoryStreamWriter memoryWriter(&streamStorage);

  plAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

  plDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes = temp;
  cmd.m_uiNumberOfCopies = dlg.s_uiNumberOfCopies;
  cmd.m_vAccumulativeTranslation = dlg.s_vTranslationStep;
  cmd.m_vAccumulativeRotation = dlg.s_vRotationStep;
  cmd.m_vRandomRotation = dlg.s_vRandomRotation;
  cmd.m_vRandomTranslation = dlg.s_vRandomTranslation;
  cmd.m_bGroupDuplicates = dlg.s_bGroupCopies;
  cmd.m_iRevolveAxis = dlg.s_iRevolveAxis;
  cmd.m_fRevolveRadius = dlg.s_fRevolveRadius;
  cmd.m_RevolveStartAngle = plAngle::MakeFromDegree(dlg.s_iRevolveStartAngle);
  cmd.m_RevolveAngleStep = plAngle::MakeFromDegree(dlg.s_iRevolveAngleStep);

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Special");

  if (history->AddCommand(cmd).m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}


void plSceneDocument::DeltaTransform()
{
  if (GetSelectionManager()->IsSelectionEmpty())
    return;

  plQtDeltaTransformDlg dlg(nullptr, this);
  dlg.exec();
}

void plSceneDocument::SnapObjectToCamera()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = plQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr)
    return;

  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != plSceneViewPerspective::Perspective)
  {
    ShowDocumentStatus("Note: This operation can only be performed in perspective views.");
    return;
  }

  const auto& camera = ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  plMat3 mRot;

  plTransform transform;
  transform.m_vScale.Set(1.0f);
  transform.m_vPosition = camera.GetCenterPosition();
  mRot.SetColumn(0, camera.GetCenterDirForwards());
  mRot.SetColumn(1, camera.GetCenterDirRight());
  mRot.SetColumn(2, camera.GetCenterDirUp());
  transform.m_qRotation = plQuat::MakeFromMat3(mRot);

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Snap Object to Camera");
  {
    for (const plDocumentObject* pObject : selection)
    {
      SetGlobalTransform(pObject, transform, TransformationChanges::Translation | TransformationChanges::Rotation);
    }
  }
  pHistory->FinishTransaction();
}


void plSceneDocument::AttachToObject()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = plQtEngineViewWidget::GetInteractionContext();
  if (ctxt.m_pLastHoveredViewWidget == nullptr || ctxt.m_pLastPickingResult == nullptr || !ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
    return;

  if (GetObjectManager()->GetObject(ctxt.m_pLastPickingResult->m_PickedObject) == nullptr)
  {
    plQtUiServices::GetSingleton()->MessageBoxStatus(plStatus(PL_FAILURE), "Target object belongs to a different document.");
    return;
  }

  plMoveObjectCommand cmd;
  cmd.m_sParentProperty = "Children";
  cmd.m_NewParent = ctxt.m_pLastPickingResult->m_PickedObject;
  cmd.m_Index = -1;

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Attach to Object");
  {
    for (const plDocumentObject* pObject : selection)
    {
      cmd.m_Object = pObject->GetGuid();

      auto res = pHistory->AddCommand(cmd);
      if (res.Failed())
      {
        plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Attach to object failed");
        pHistory->CancelTransaction();
        return;
      }
    }
  }
  pHistory->FinishTransaction();
}

void plSceneDocument::DetachFromParent()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  plMoveObjectCommand cmd;
  cmd.m_sParentProperty = "Children";

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Detach from Parent");
  {
    for (const plDocumentObject* pObject : selection)
    {
      cmd.m_Object = pObject->GetGuid();

      auto res = pHistory->AddCommand(cmd);
      if (res.Failed())
      {
        plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Detach from parent failed");
        pHistory->CancelTransaction();
        return;
      }
    }
  }
  pHistory->FinishTransaction();

  ShowDocumentStatus(plFmt("Detached {} objects", selection.GetCount()));

  // reapply the selection to fix tree views etc. after the re-parenting
  plDeque<const plDocumentObject*> prevSelection = selection;
  GetSelectionManager()->Clear();
  GetSelectionManager()->SetSelection(prevSelection);
}

void plSceneDocument::CopyReference()
{
  if (GetSelectionManager()->GetSelection().GetCount() != 1)
    return;

  const plUuid guid = GetSelectionManager()->GetSelection()[0]->GetGuid();

  plStringBuilder sGuid;
  plConversionUtils::ToString(guid, sGuid);

  QApplication::clipboard()->setText(sGuid.GetData());

  plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(plFmt("Copied Object Reference: {}", sGuid), plTime::MakeFromSeconds(5));
}

plStatus plSceneDocument::CreateEmptyObject(bool bAttachToParent, bool bAtPickedPosition)
{
  auto history = GetCommandHistory();

  history->StartTransaction("Create Node");

  plAddObjectCommand cmdAdd;
  cmdAdd.m_pType = plGetStaticRTTI<plGameObject>();
  cmdAdd.m_sParentProperty = "Children";
  cmdAdd.m_Index = -1;

  plUuid NewNode;

  const auto& Sel = GetSelectionManager()->GetSelection();

  if (Sel.IsEmpty() || !bAttachToParent)
  {
    cmdAdd.m_NewObjectGuid = plUuid::MakeUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    auto res = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }
  else
  {
    cmdAdd.m_NewObjectGuid = plUuid::MakeUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    cmdAdd.m_Parent = Sel[0]->GetGuid();
    auto res = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  const auto& ctxt = plQtEngineViewWidget::GetInteractionContext();

  if (!bAttachToParent && bAtPickedPosition && ctxt.m_pLastPickingResult && !ctxt.m_pLastPickingResult->m_vPickedPosition.IsNaN())
  {
    plVec3 position = ctxt.m_pLastPickingResult->m_vPickedPosition;

    plSnapProvider::SnapTranslation(position);

    plSetObjectPropertyCommand cmdSet;
    cmdSet.m_NewValue = position;
    cmdSet.m_Object = NewNode;
    cmdSet.m_sProperty = "LocalPosition";

    auto res = history->AddCommand(cmdSet);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  // Add a dummy shape icon component, which enables picking
  {
    plAddObjectCommand cmdAdd;
    cmdAdd.m_pType = plRTTI::FindTypeByName("plShapeIconComponent");
    cmdAdd.m_sParentProperty = "Components";
    cmdAdd.m_Index = -1;
    cmdAdd.m_Parent = NewNode;

    auto res = history->AddCommand(cmdAdd);
  }

  history->FinishTransaction();

  GetSelectionManager()->SetSelection(GetObjectManager()->GetObject(NewNode));
  return plStatus(PL_SUCCESS);
}

void plSceneDocument::DuplicateSelection()
{
  plMap<plUuid, plUuid> parents;

  plAbstractObjectGraph graph;
  CopySelectedObjects(graph, &parents);

  plStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", plConversionUtils::ToString(it.Key(), tmp1), plConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  plContiguousMemoryStreamStorage streamStorage;
  plMemoryStreamWriter memoryWriter(&streamStorage);

  plAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

  plDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes = temp;

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Selection");

  if (history->AddCommand(cmd).m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}

void plSceneDocument::ToggleHideSelectedObjects()
{
  auto sel = GetSelectionManager()->GetSelection();

  for (auto pItem : sel)
  {
    if (!pItem->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
      continue;

    ApplyRecursive(pItem, [this](const plDocumentObject* pObj)
    {
      auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObj->GetGuid());
      pMeta->m_bHidden = !pMeta->m_bHidden;
      m_DocumentObjectMetaData->EndModifyMetaData(plDocumentObjectMetaData::HiddenFlag);
    });
  }
}

void plSceneDocument::ShowOrHideSelectedObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  auto sel = GetSelectionManager()->GetSelection();

  for (auto pItem : sel)
  {
    if (!pItem->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
      continue;

    ApplyRecursive(pItem, [this, bHide](const plDocumentObject* pObj)
      {
      // if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
      // return;

      auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObj->GetGuid());
      if (pMeta->m_bHidden != bHide)
      {
        pMeta->m_bHidden = bHide;
        m_DocumentObjectMetaData->EndModifyMetaData(plDocumentObjectMetaData::HiddenFlag);
      }
      else
        m_DocumentObjectMetaData->EndModifyMetaData(0); });
  }
}

void plSceneDocument::HideUnselectedObjects()
{
  ShowOrHideAllObjects(ShowOrHide::Hide);

  ShowOrHideSelectedObjects(ShowOrHide::Show);
}

void plSceneDocument::SetGameMode(GameMode::Enum mode)
{
  if (m_GameMode == mode)
    return;

  // store settings of recently active mode
  m_GameModeData[m_GameMode] = m_CurrentMode;

  m_GameMode = mode;

  switch (m_GameMode)
  {
    case GameMode::Off:
      ShowDocumentStatus("Game Mode: Off");
      break;
    case GameMode::Simulate:
      ShowDocumentStatus("Game Mode: Simulate");
      break;
    case GameMode::Play:
      ShowDocumentStatus("Game Mode: Play");
      break;
  }

  SetRenderSelectionOverlay(m_GameModeData[m_GameMode].m_bRenderSelectionOverlay);
  SetRenderShapeIcons(m_GameModeData[m_GameMode].m_bRenderShapeIcons);
  SetRenderVisualizers(m_GameModeData[m_GameMode].m_bRenderVisualizers);

  if (m_GameMode == GameMode::Off)
  {
    // reset the game world
    SendGameWorldToEngine();
  }

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::GameModeChanged;
  m_GameObjectEvents.Broadcast(e);

  ScheduleSendObjectSelection();
}

plStatus plSceneDocument::CreatePrefabDocumentFromSelection(plStringView sFile, const plRTTI* pRootType, plDelegate<void(plAbstractObjectNode*)> adjustGraphNodeCB /* = {} */, plDelegate<void(plDocumentObject*)> adjustNewNodesCB /* = {} */, plDelegate<void(plAbstractObjectGraph& graph, plDynamicArray<plAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB /* = {} */)
{
  auto Selection = GetSelectionManager()->GetTopLevelSelection(pRootType);

  if (Selection.IsEmpty())
    return plStatus("To create a prefab, the selection must not be empty");

  const plTransform tReference = QueryLocalTransform(Selection.PeekBack());

  plVariantArray varChildren;

  auto centerNodes = [tReference, &varChildren](plAbstractObjectNode* pGraphNode)
  {
    if (auto pPosition = pGraphNode->FindProperty("LocalPosition"))
    {
      plVec3 pos = pPosition->m_Value.ConvertTo<plVec3>();
      pos -= tReference.m_vPosition;

      pGraphNode->ChangeProperty("LocalPosition", pos);
    }

    if (auto pRotation = pGraphNode->FindProperty("LocalRotation"))
    {
      plQuat rot = pRotation->m_Value.ConvertTo<plQuat>();
      rot = tReference.m_qRotation.GetInverse() * rot;

      pGraphNode->ChangeProperty("LocalRotation", rot);
    }

    varChildren.PushBack(pGraphNode->GetGuid());
  };

  auto adjustResult = [tReference, this](plDocumentObject* pObject)
  {
    const plTransform tOld = QueryLocalTransform(pObject);

    plSetObjectPropertyCommand cmd;
    cmd.m_Object = pObject->GetGuid();

    cmd.m_sProperty = "LocalPosition";
    cmd.m_NewValue = tOld.m_vPosition + tReference.m_vPosition;
    GetCommandHistory()->AddCommand(cmd).AssertSuccess();

    cmd.m_sProperty = "LocalRotation";
    cmd.m_NewValue = tReference.m_qRotation * tOld.m_qRotation;
    GetCommandHistory()->AddCommand(cmd).AssertSuccess();
  };

  auto finalizeGraph = [this, &varChildren](plAbstractObjectGraph& ref_graph, plDynamicArray<plAbstractObjectNode*>& ref_graphRootNodes) {
    if (ref_graphRootNodes.GetCount() == 1)
    {
      ref_graphRootNodes[0]->ChangeProperty("Name", "<Prefab-Root>");
    }
    else
    {
      const plRTTI* pRtti = plGetStaticRTTI<plGameObject>();

      plAbstractObjectNode* pRoot = ref_graph.AddNode(plUuid::MakeUuid(), pRtti->GetTypeName(), pRtti->GetTypeVersion());
      pRoot->AddProperty("Name", "<Prefab-Root>");
      pRoot->AddProperty("Children", varChildren);

      ref_graphRootNodes.Clear();
      ref_graphRootNodes.PushBack(pRoot);
    }
  };

  if (!adjustGraphNodeCB.IsValid())
    adjustGraphNodeCB = centerNodes;
  if (!adjustNewNodesCB.IsValid())
    adjustNewNodesCB = adjustResult;
  if (!finalizeGraphCB.IsValid())
    finalizeGraphCB = finalizeGraph;

  return SUPER::CreatePrefabDocumentFromSelection(sFile, pRootType, adjustGraphNodeCB, adjustNewNodesCB, finalizeGraphCB);
}

bool plSceneDocument::CanEngineProcessBeRestarted() const
{
  return m_GameMode == GameMode::Off;
}

void plSceneDocument::StartSimulateWorld()
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  {
    plGameObjectDocumentEvent e;
    e.m_Type = plGameObjectDocumentEvent::Type::GameMode_StartingSimulate;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  SetGameMode(GameMode::Simulate);
}


void plSceneDocument::TriggerGameModePlay(bool bUsePickedPositionAsStart)
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  {
    plGameObjectDocumentEvent e;
    e.m_Type = plGameObjectDocumentEvent::Type::GameMode_StartingPlay;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  UpdateObjectDebugTargets();

  // attempt to start PTG
  // do not change state here
  {
    plGameModeMsgToEngine msg;
    msg.m_bEnablePTG = true;
    msg.m_bUseStartPosition = false;

    if (bUsePickedPositionAsStart)
    {
      const auto& ctxt = plQtEngineViewWidget::GetInteractionContext();

      if (ctxt.m_pLastHoveredViewWidget != nullptr && ctxt.m_pLastHoveredViewWidget->GetDocumentWindow()->GetDocument() == this)
      {
        msg.m_bUseStartPosition = true;
        msg.m_vStartPosition = ctxt.m_pLastPickingResult->m_vPickedPosition;

        plVec3 vPickDir = ctxt.m_pLastPickingResult->m_vPickedPosition - ctxt.m_pLastPickingResult->m_vPickingRayStart;
        vPickDir.z = 0;
        vPickDir.NormalizeIfNotZero(plVec3(1, 0, 0)).IgnoreResult();

        msg.m_vStartDirection = vPickDir;
      }
    }

    GetEditorEngineConnection()->SendMessage(&msg);
  }
}


bool plSceneDocument::StopGameMode()
{
  if (m_GameMode == GameMode::Off)
    return false;

  if (m_GameMode == GameMode::Simulate)
  {
    // we can set that state immediately
    SetGameMode(GameMode::Off);
  }

  if (m_GameMode == GameMode::Play)
  {
    // attempt to stop PTG
    // do not change any state, that will be done by the response msg
    {
      plGameModeMsgToEngine msg;
      msg.m_bEnablePTG = false;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
  }

  {
    plGameObjectDocumentEvent e;
    e.m_Type = plGameObjectDocumentEvent::Type::GameMode_Stopped;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  return true;
}

void plSceneDocument::ShowOrHideAllObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  ApplyRecursive(GetObjectManager()->GetRootObject(), [this, bHide](const plDocumentObject* pObj)
    {
    // if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
    // return;

    plUInt32 uiFlags = 0;

    auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObj->GetGuid());

    if (pMeta->m_bHidden != bHide)
    {
      pMeta->m_bHidden = bHide;
      uiFlags = plDocumentObjectMetaData::HiddenFlag;
    }

    m_DocumentObjectMetaData->EndModifyMetaData(uiFlags); });
}
void plSceneDocument::GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/plEditor.plAbstractGraph");
}

bool plSceneDocument::CopySelectedObjects(plAbstractObjectGraph& ref_graph, plStringBuilder& out_sMimeType) const
{
  out_sMimeType = "application/plEditor.plAbstractGraph";
  return CopySelectedObjects(ref_graph, nullptr);
}

bool plSceneDocument::CopySelectedObjects(plAbstractObjectGraph& ref_graph, plMap<plUuid, plUuid>* out_pParents) const
{
  if (GetSelectionManager()->GetSelection().GetCount() == 0)
    return false;

  // Serialize selection to graph
  auto Selection = GetSelectionManager()->GetTopLevelSelection();

  plDocumentObjectConverterWriter writer(&ref_graph, GetObjectManager());

  // TODO: objects are required to be named root but this is not enforced or obvious by the interface.
  for (plUInt32 i = 0; i < Selection.GetCount(); i++)
  {
    auto item = Selection[i];
    plAbstractObjectNode* pNode = writer.AddObjectToGraph(item, "root");
    pNode->AddProperty("__GlobalTransform", GetGlobalTransform(item));
    pNode->AddProperty("__Order", i);
  }

  if (out_pParents != nullptr)
  {
    out_pParents->Clear();

    for (auto item : Selection)
    {
      (*out_pParents)[item->GetGuid()] = item->GetParent()->GetGuid();
    }
  }

  AttachMetaDataBeforeSaving(ref_graph);

  return true;
}

bool plSceneDocument::PasteAt(const plArrayPtr<PasteInfo>& info, const plVec3& vPasteAt)
{
  plVec3 vAvgPos(0.0f);

  for (const PasteInfo& pi : info)
  {
    if (pi.m_pObject->GetTypeAccessor().GetType() != plGetStaticRTTI<plGameObject>())
      return false;

    vAvgPos += pi.m_pObject->GetTypeAccessor().GetValue("LocalPosition").Get<plVec3>();
  }

  vAvgPos /= info.GetCount();

  for (const PasteInfo& pi : info)
  {
    const plVec3 vLocalPos = pi.m_pObject->GetTypeAccessor().GetValue("LocalPosition").Get<plVec3>();
    pi.m_pObject->GetTypeAccessor().SetValue("LocalPosition", vLocalPos - vAvgPos + vPasteAt);

    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", pi.m_Index);
    }
  }

  return true;
}

bool plSceneDocument::PasteAtOrignalPosition(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph)
{
  for (const PasteInfo& pi : info)
  {
    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", pi.m_Index);
    }
    if (auto* pNode = objectGraph.GetNode(pi.m_pObject->GetGuid()))
    {
      if (auto* pProperty = pNode->FindProperty("__GlobalTransform"))
      {
        if (pProperty->m_Value.IsA<plTransform>())
        {
          SetGlobalTransform(pi.m_pObject, pProperty->m_Value.Get<plTransform>(), TransformationChanges::All);
        }
      }
    }
  }

  return true;
}

bool plSceneDocument::Paste(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType)
{
  const auto& ctxt = plQtEngineViewWidget::GetInteractionContext();

  if (bAllowPickedPosition && ctxt.m_pLastPickingResult && ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
  {
    plVec3 pos = ctxt.m_pLastPickingResult->m_vPickedPosition;
    plSnapProvider::SnapTranslation(pos);

    if (!PasteAt(info, pos))
      return false;
  }
  else
  {
    if (!PasteAtOrignalPosition(info, objectGraph))
      return false;
  }

  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);
  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);

  // set the pasted objects as the new selection
  {
    auto pSelMan = GetSelectionManager();

    plDeque<const plDocumentObject*> NewSelection;

    for (const PasteInfo& pi : info)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }

  return true;
}

bool plSceneDocument::DuplicateSelectedObjects(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bSetSelected)
{
  if (!PasteAtOrignalPosition(info, objectGraph))
    return false;

  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);
  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);

  // set the pasted objects as the new selection
  if (bSetSelected)
  {
    auto pSelMan = GetSelectionManager();

    plDeque<const plDocumentObject*> NewSelection;

    for (const PasteInfo& pi : info)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }

  return true;
}

void plSceneDocument::EnsureSettingsObjectExist()
{
  // Settings object was changed to have a base class and each document type has a different implementation.
  const plRTTI* pSettingsType = nullptr;
  switch (m_DocumentType)
  {
    case plSceneDocument::DocumentType::Scene:
      pSettingsType = plGetStaticRTTI<plSceneDocumentSettings>();
      break;
    case plSceneDocument::DocumentType::Prefab:
      pSettingsType = plGetStaticRTTI<plPrefabDocumentSettings>();
      break;
    case plSceneDocument::DocumentType::Layer:
      pSettingsType = plGetStaticRTTI<plLayerDocumentSettings>();
      break;
  }

  auto pRoot = GetObjectManager()->GetRootObject();
  // Use the plObjectDirectAccessor instead of calling GetObjectAccessor because we do not want
  // undo ops for this operation.
  plObjectDirectAccessor accessor(GetObjectManager());
  plVariant value;
  PL_VERIFY(accessor.plObjectAccessorBase::GetValue(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  plUuid id = value.Get<plUuid>();
  if (!id.IsValid())
  {
    PL_VERIFY(accessor.plObjectAccessorBase::AddObject(pRoot, "Settings", plVariant(), pSettingsType, id).Succeeded(), "Adding scene settings object to root failed.");
  }
  else
  {
    plDocumentObject* pSettings = GetObjectManager()->GetObject(id);
    PL_VERIFY(pSettings, "Document corrupt, root references a non-existing object");
    if (pSettings->GetType() != pSettingsType)
    {
      accessor.RemoveObject(pSettings).AssertSuccess();
      GetObjectManager()->DestroyObject(pSettings);
      PL_VERIFY(accessor.plObjectAccessorBase::AddObject(pRoot, "Settings", plVariant(), pSettingsType, id).Succeeded(), "Adding scene settings object to root failed.");
    }
  }
}

const plDocumentObject* plSceneDocument::GetSettingsObject() const
{
  auto pRoot = GetObjectManager()->GetRootObject();
  plVariant value;
  PL_VERIFY(GetObjectAccessor()->GetValue(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  plUuid id = value.Get<plUuid>();
  return GetObjectManager()->GetObject(id);
}

const plSceneDocumentSettingsBase* plSceneDocument::GetSettingsBase() const
{
  return static_cast<const plSceneDocumentSettingsBase*>(m_ObjectMirror.GetNativeObjectPointer(GetSettingsObject()));
}

plStatus plSceneDocument::CreateExposedProperty(const plDocumentObject* pObject, const plAbstractProperty* pProperty, plVariant index, plExposedSceneProperty& out_key) const
{
  const plDocumentObject* pNodeComponent = plObjectPropertyPath::FindParentNodeComponent(pObject);
  if (!pObject)
    return plStatus("No parent node or component found.");

  plObjectPropertyPathContext context = {pNodeComponent, GetObjectAccessor(), "Children"};
  plPropertyReference propertyRef = {pObject->GetGuid(), pProperty, index};
  plStringBuilder sPropertyPath;
  plStatus res = plObjectPropertyPath::CreatePropertyPath(context, propertyRef, sPropertyPath);
  if (res.Failed())
    return res;

  out_key.m_Object = pNodeComponent->GetGuid();
  out_key.m_sPropertyPath = sPropertyPath;
  return plStatus(PL_SUCCESS);
}

plStatus plSceneDocument::AddExposedParameter(const char* szName, const plDocumentObject* pObject, const plAbstractProperty* pProperty, plVariant index)
{
  if (m_DocumentType != DocumentType::Prefab)
    return plStatus("Exposed parameters are only supported in prefab documents.");

  if (FindExposedParameter(pObject, pProperty, index) != -1)
    return plStatus("Exposed parameter already exists.");

  plExposedSceneProperty key;
  plStatus res = CreateExposedProperty(pObject, pProperty, index, key);
  if (res.Failed())
    return res;

  plUuid id;
  res = GetObjectAccessor()->AddObject(GetSettingsObject(), "ExposedProperties", -1, plGetStaticRTTI<plExposedSceneProperty>(), id);
  if (res.Failed())
    return res;
  const plDocumentObject* pParam = GetObjectManager()->GetObject(id);
  GetObjectAccessor()->SetValue(pParam, "Name", szName).LogFailure();
  GetObjectAccessor()->SetValue(pParam, "Object", key.m_Object).LogFailure();
  GetObjectAccessor()->SetValue(pParam, "PropertyPath", plVariant(key.m_sPropertyPath)).LogFailure();
  return plStatus(PL_SUCCESS);
}

plInt32 plSceneDocument::FindExposedParameter(const plDocumentObject* pObject, const plAbstractProperty* pProperty, plVariant index)
{
  PL_ASSERT_DEV(m_DocumentType == DocumentType::Prefab, "Exposed properties are only supported in prefab documents.");

  plExposedSceneProperty key;
  plStatus res = CreateExposedProperty(pObject, pProperty, index, key);
  if (res.Failed())
    return -1;

  const plPrefabDocumentSettings* settings = GetSettings<plPrefabDocumentSettings>();
  for (plUInt32 i = 0; i < settings->m_ExposedProperties.GetCount(); i++)
  {
    const auto& param = settings->m_ExposedProperties[i];
    if (param.m_Object == key.m_Object && param.m_sPropertyPath == key.m_sPropertyPath)
      return (plInt32)i;
  }
  return -1;
}

plStatus plSceneDocument::RemoveExposedParameter(plInt32 iIndex)
{
  plVariant value;
  auto res = GetObjectAccessor()->GetValue(GetSettingsObject(), "ExposedProperties", value, iIndex);
  if (res.Failed())
    return res;

  plUuid id = value.Get<plUuid>();
  return GetObjectAccessor()->RemoveObject(GetObjectManager()->GetObject(id));
}


void plSceneDocument::StoreFavoriteCamera(plUInt8 uiSlot)
{
  PL_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  plQuadViewPreferencesUser* pPreferences = plPreferences::QueryPreferences<plQuadViewPreferencesUser>(this);
  auto& cam = pPreferences->m_FavoriteCamera[uiSlot];

  auto* pView = plQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView)
  {
    const auto& camera = pView->m_pViewConfig->m_Camera;

    cam.m_PerspectiveMode = pView->m_pViewConfig->m_Perspective;
    cam.m_vCamPos = camera.GetCenterPosition();
    cam.m_vCamDir = camera.GetCenterDirForwards();
    cam.m_vCamUp = camera.GetCenterDirUp();

    // make sure the data gets saved
    pPreferences->TriggerPreferencesChangedEvent();
  }
}

void plSceneDocument::RestoreFavoriteCamera(plUInt8 uiSlot)
{
  PL_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  plQuadViewPreferencesUser* pPreferences = plPreferences::QueryPreferences<plQuadViewPreferencesUser>(this);
  auto& cam = pPreferences->m_FavoriteCamera[uiSlot];

  auto* pView = plQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return;

  plVec3 vCamPos = cam.m_vCamPos;
  plVec3 vCamDir = cam.m_vCamDir;
  plVec3 vCamUp = cam.m_vCamUp;

  // if the projection mode of the view is orthographic, ignore the direction of the stored favorite camera
  // if we apply a favorite that was saved in an orthographic view, and we apply it to a perspective view,
  // we want to ignore one of the axis, as the respective orthographic position can be arbitrary
  switch (pView->m_pViewConfig->m_Perspective)
  {
    case plSceneViewPerspective::Orthogonal_Front:
    case plSceneViewPerspective::Orthogonal_Right:
    case plSceneViewPerspective::Orthogonal_Top:
      vCamDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards();
      vCamUp = pView->m_pViewConfig->m_Camera.GetCenterDirUp();
      break;

    case plSceneViewPerspective::Perspective:
    {
      const plVec3 vOldPos = pView->m_pViewConfig->m_Camera.GetCenterPosition();

      switch (cam.m_PerspectiveMode)
      {
        case plSceneViewPerspective::Orthogonal_Front:
          vCamPos.x = vOldPos.x;
          break;
        case plSceneViewPerspective::Orthogonal_Right:
          vCamPos.y = vOldPos.y;
          break;
        case plSceneViewPerspective::Orthogonal_Top:
          vCamPos.z = vOldPos.z;
          break;
        case plSceneViewPerspective::Perspective:
          break;
      }

      break;
    }
  }

  pView->InterpolateCameraTo(vCamPos, vCamDir, pView->m_pViewConfig->m_Camera.GetFovOrDim(), &vCamUp);
}

plResult plSceneDocument::JumpToLevelCamera(plUInt8 uiSlot, bool bImmediate)
{
  PL_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  auto* pView = plQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return PL_FAILURE;

  auto* pObjMan = GetObjectManager();

  plHybridArray<plDocumentObject*, 8> stack;
  stack.PushBack(pObjMan->GetRootObject());

  const plRTTI* pCamType = plGetStaticRTTI<plCameraComponent>();
  const plDocumentObject* pCamObj = nullptr;

  while (!stack.IsEmpty())
  {
    const plDocumentObject* pObj = stack.PeekBack();
    stack.PopBack();

    stack.PushBackRange(pObj->GetChildren());

    if (pObj->GetType() == pCamType)
    {
      plInt32 iShortcut = pObj->GetTypeAccessor().GetValue("EditorShortcut").ConvertTo<plInt32>();

      if (iShortcut == uiSlot)
      {
        pCamObj = pObj->GetParent();
        break;
      }
    }
  }

  if (pCamObj == nullptr)
    return PL_FAILURE;

  const plTransform tCam = GetGlobalTransform(pCamObj);

  plVec3 vCamDir = tCam.m_qRotation * plVec3(1, 0, 0);
  plVec3 vCamUp = tCam.m_qRotation * plVec3(0, 0, 1);

  // if the projection mode of the view is orthographic, ignore the direction of the level camera
  switch (pView->m_pViewConfig->m_Perspective)
  {
    case plSceneViewPerspective::Orthogonal_Front:
    case plSceneViewPerspective::Orthogonal_Right:
    case plSceneViewPerspective::Orthogonal_Top:
      vCamDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards();
      vCamUp = pView->m_pViewConfig->m_Camera.GetCenterDirUp();
      break;

    case plSceneViewPerspective::Perspective:
      break;
  }

  pView->InterpolateCameraTo(tCam.m_vPosition, vCamDir, pView->m_pViewConfig->m_Camera.GetFovOrDim(), &vCamUp, bImmediate);

  return PL_SUCCESS;
}

plResult plSceneDocument::CreateLevelCamera(plUInt8 uiSlot)
{
  PL_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  auto* pView = plQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return PL_FAILURE;

  if (pView->m_pViewConfig->m_Perspective != plSceneViewPerspective::Perspective)
    return PL_FAILURE;

  const auto* pRootObj = GetObjectManager()->GetRootObject();

  const plVec3 vPos = pView->m_pViewConfig->m_Camera.GetCenterPosition();
  const plVec3 vDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards().GetNormalized();
  const plVec3 vUp = pView->m_pViewConfig->m_Camera.GetCenterDirUp().GetNormalized();

  auto* pAccessor = GetObjectAccessor();
  pAccessor->StartTransaction("Create Level Camera");

  plUuid camObjGuid;
  if (pAccessor->AddObject(pRootObj, "Children", -1, plGetStaticRTTI<plGameObject>(), camObjGuid).Failed())
  {
    pAccessor->CancelTransaction();
    return PL_FAILURE;
  }

  plMat3 mRot;
  mRot.SetColumn(0, vDir);
  mRot.SetColumn(1, vUp.CrossRH(vDir).GetNormalized());
  mRot.SetColumn(2, vUp);
  plQuat qRot;
  qRot = plQuat::MakeFromMat3(mRot);
  qRot.Normalize();

  SetGlobalTransform(pAccessor->GetObject(camObjGuid), plTransform(vPos, qRot), TransformationChanges::Translation | TransformationChanges::Rotation);

  plUuid camCompGuid;
  if (pAccessor->AddObject(pAccessor->GetObject(camObjGuid), "Components", -1, plGetStaticRTTI<plCameraComponent>(), camCompGuid).Failed())
  {
    pAccessor->CancelTransaction();
    return PL_FAILURE;
  }

  if (pAccessor->SetValue(pAccessor->GetObject(camCompGuid), "EditorShortcut", uiSlot).Failed())
  {
    pAccessor->CancelTransaction();
    return PL_FAILURE;
  }

  pAccessor->FinishTransaction();
  return PL_SUCCESS;
}

void plSceneDocument::DocumentObjectMetaDataEventHandler(const plObjectMetaData<plUuid, plDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & plDocumentObjectMetaData::HiddenFlag) != 0)
  {
    plObjectTagMsgToEngine msg;
    msg.m_bSetTag = e.m_pValue->m_bHidden;
    msg.m_sTag = "EditorHidden";
    msg.m_bApplyOnAllChildren = true;

    SendObjectMsg(GetObjectManager()->GetObject(e.m_ObjectKey), &msg);
  }
}

void plSceneDocument::EngineConnectionEventHandler(const plEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case plEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    case plEditorEngineProcessConnection::Event::Type::ProcessShutdown:
    case plEditorEngineProcessConnection::Event::Type::ProcessStarted:
      SetGameMode(GameMode::Off);
      break;

    default:
      break;
  }
}


void plSceneDocument::ToolsProjectEventHandler(const plToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case plToolsProjectEvent::Type::ProjectConfigChanged:
    {
      // we are lazy and just re-select everything here
      // that ensures that ui elements will rebuild their content

      plDeque<const plDocumentObject*> selection = GetSelectionManager()->GetSelection();
      GetSelectionManager()->SetSelection(selection);
    }
    break;

    default:
      break;
  }
}

void plSceneDocument::HandleGameModeMsg(const plGameModeMsgToEditor* pMsg)
{
  if (m_GameMode == GameMode::Simulate)
  {
    if (pMsg->m_bRunningPTG)
    {
      m_GameMode = GameMode::Off;
      plLog::Warning("Incorrect state change from 'simulate' to 'play-the-game'");
    }
    else
    {
      // probably the message just arrived late ?
      return;
    }
  }

  if (m_GameMode == GameMode::Off || m_GameMode == GameMode::Play)
  {
    SetGameMode(pMsg->m_bRunningPTG ? GameMode::Play : GameMode::Off);
    return;
  }

  PL_REPORT_FAILURE("Unreachable Code reached.");
}

void plSceneDocument::HandleObjectStateFromEngineMsg(const plPushObjectStateMsgToEditor* pMsg)
{
  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Pull Object State");

  for (const auto& state : pMsg->m_ObjectStates)
  {
    auto pObject = GetObjectManager()->GetObject(state.m_ObjectGuid);

    if (pObject)
    {
      SetGlobalTransform(pObject, plTransform(state.m_vPosition, state.m_qRotation), TransformationChanges::Translation | TransformationChanges::Rotation);
    }
  }

  pHistory->FinishTransaction();
}

void plSceneDocument::SendObjectMsg(const plDocumentObject* pObj, plObjectTagMsgToEngine* pMsg)
{
  // if plObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (pObj == nullptr || !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);
}

void plSceneDocument::SendObjectMsgRecursive(const plDocumentObject* pObj, plObjectTagMsgToEngine* pMsg)
{
  // if plObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (pObj == nullptr || !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);

  for (auto pChild : pObj->GetChildren())
  {
    SendObjectMsgRecursive(pChild, pMsg);
  }
}

void plSceneDocument::GatherObjectsOfType(plDocumentObject* pRoot, plGatherObjectsOfTypeMsgInterDoc* pMsg) const
{
  if (pRoot->GetType() == pMsg->m_pType)
  {
    plStringBuilder sFullPath;
    GenerateFullDisplayName(pRoot, sFullPath);

    auto& res = pMsg->m_Results.ExpandAndGetRef();
    res.m_ObjectGuid = pRoot->GetGuid();
    res.m_pDocument = this;
    res.m_sDisplayName = sFullPath;
  }

  for (auto pChild : pRoot->GetChildren())
  {
    GatherObjectsOfType(pChild, pMsg);
  }
}

void plSceneDocument::OnInterDocumentMessage(plReflectedClass* pMessage, plDocument* pSender)
{
  // #TODO needs to be overwritten by Scene2
  if (pMessage->GetDynamicRTTI()->IsDerivedFrom<plGatherObjectsOfTypeMsgInterDoc>())
  {
    GatherObjectsOfType(GetObjectManager()->GetRootObject(), static_cast<plGatherObjectsOfTypeMsgInterDoc*>(pMessage));
  }
}

plStatus plSceneDocument::RequestExportScene(const char* szTargetFile, const plAssetFileHeader& header)
{
  if (GetGameMode() != GameMode::Off)
    return plStatus("Cannot export while the scene is simulating");

  PL_SUCCEED_OR_RETURN(SaveDocument());

  const plStatus status = plAssetDocument::RemoteExport(header, szTargetFile);

  // make sure the world is reset
  SendGameWorldToEngine();

  return status;
}

void plSceneDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // scenes do not have exposed parameters
  if (!IsPrefab())
    return;

  plExposedParameters* pExposedParams = PL_DEFAULT_NEW(plExposedParameters);

  if (m_DocumentType == DocumentType::Prefab)
  {
    plSet<plString> alreadyExposed;

    auto pSettings = GetSettings<plPrefabDocumentSettings>();
    for (auto prop : pSettings->m_ExposedProperties)
    {
      auto pRootObject = GetObjectManager()->GetObject(prop.m_Object);
      if (!pRootObject)
      {
        plLog::Warning("The exposed scene property '{0}' does not point to a valid object and is skipped.", prop.m_sName);
        continue;
      }

      plObjectPropertyPathContext context = {pRootObject, GetObjectAccessor(), "Children"};

      plPropertyReference key;
      auto res = plObjectPropertyPath::ResolvePropertyPath(context, prop.m_sPropertyPath, key);
      if (res.Failed())
      {
        plLog::Warning("The exposed scene property '{0}' can no longer be resolved and is skipped.", prop.m_sName);
        continue;
      }
      plVariant value;

      const plExposedParameter* pSourceParameter = nullptr;
      auto pLeafObject = GetObjectManager()->GetObject(key.m_Object);
      if (const plExposedParametersAttribute* pAttrib = key.m_pProperty->GetAttributeByType<plExposedParametersAttribute>())
      {
        // If the target of the exposed parameter is yet another exposed parameter, we need to do the following:
        // A: Get the default value from via an plExposedParameterCommandAccessor. This ensures that in case the target does not actually exist (because it was not overwritten in this template instance) we get the default parameter of the exposed param instead.
        // B: Replace the property type of the exposed parameter (which will always be an plVariant inside the plVariantDictionary) with the property type the exposed parameter actually points to.
        const plAbstractProperty* pParameterSourceProp = pLeafObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
        PL_ASSERT_DEBUG(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), pLeafObject->GetType()->GetTypeName());

        plExposedParameterCommandAccessor proxy(context.m_pAccessor, key.m_pProperty, pParameterSourceProp);
        res = proxy.GetValue(pLeafObject, key.m_pProperty, value, key.m_Index);
        if (key.m_Index.IsA<plString>())
        {
          pSourceParameter = proxy.GetExposedParam(pLeafObject, key.m_Index.Get<plString>());
        }
      }
      else
      {
        res = context.m_pAccessor->GetValue(pLeafObject, key.m_pProperty, value, key.m_Index);
      }
      PL_ASSERT_DEBUG(res.Succeeded(), "ResolvePropertyPath succeeded so GetValue should too");

      // do not show the same parameter twice, even if they have different types, as the UI doesn't handle that case properly
      // TODO: we should prevent users from using the same name for differently typed parameters
      // don't do this earlier, we do want to validate each exposed property with the code above
      if (alreadyExposed.Contains(prop.m_sName))
        continue;

      alreadyExposed.Insert(prop.m_sName);

      plExposedParameter* param = PL_DEFAULT_NEW(plExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = prop.m_sName;
      param->m_DefaultValue = value;
      if (pSourceParameter)
      {
        // Copy properties of exposed parameter that we are exposing one level deeper.
        param->m_sType = pSourceParameter->m_sType;
        for (auto attrib : pSourceParameter->m_Attributes)
        {
          param->m_Attributes.PushBack(plReflectionSerializer::Clone(attrib));
        }
      }
      else
      {
        param->m_sType = key.m_pProperty->GetSpecificType()->GetTypeName();
        for (auto attrib : key.m_pProperty->GetAttributes())
        {
          param->m_Attributes.PushBack(plReflectionSerializer::Clone(attrib));
        }
      }
    }
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

plTransformStatus plSceneDocument::ExportScene(bool bCreateThumbnail)
{
  if (GetUnknownObjectTypeInstances() > 0)
  {
    return plTransformStatus("Can't export scene/prefab when it contains unknown object types.");
  }

  // #TODO export layers
  auto saveres = SaveDocument();

  if (saveres.m_Result.Failed())
    return saveres;

  plTransformStatus res;

  if (bCreateThumbnail)
  {
    // this is needed to generate a scene thumbnail, however that has a larger overhead (1 sec or so)
    res = plAssetCurator::GetSingleton()->TransformAsset(GetGuid(), plTransformFlags::ForceTransform | plTransformFlags::TriggeredManually);
  }
  else
    res = TransformAsset(plTransformFlags::ForceTransform | plTransformFlags::TriggeredManually);

  if (res.Failed())
    plLog::Error(res.m_sMessage);
  else
    plLog::Success(res.m_sMessage);

  ShowDocumentStatus(res.m_sMessage.GetData());

  return res;
}

void plSceneDocument::ExportSceneGeometry(const char* szFile, bool bOnlySelection, int iExtractionMode, const plMat3& mTransform)
{
  plExportSceneGeometryMsgToEngine msg;
  msg.m_sOutputFile = szFile;
  msg.m_bSelectionOnly = bOnlySelection;
  msg.m_iExtractionMode = iExtractionMode;
  msg.m_Transform = mTransform;

  SendMessageToEngine(&msg);

  plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(plFmt("Geometry exported to '{0}'", szFile), plTime::MakeFromSeconds(5.0f));
}

void plSceneDocument::HandleEngineMessage(const plEditorEngineDocumentMsg* pMsg)
{
  plGameObjectDocument::HandleEngineMessage(pMsg);

  if (const plGameModeMsgToEditor* msg = plDynamicCast<const plGameModeMsgToEditor*>(pMsg))
  {
    HandleGameModeMsg(msg);
    return;
  }

  if (const plDocumentOpenResponseMsgToEditor* msg = plDynamicCast<const plDocumentOpenResponseMsgToEditor*>(pMsg))
  {
    SyncObjectHiddenState();
  }

  if (const plPushObjectStateMsgToEditor* msg = plDynamicCast<const plPushObjectStateMsgToEditor*>(pMsg))
  {
    HandleObjectStateFromEngineMsg(msg);
  }
}

plTransformStatus plSceneDocument::InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  if (m_DocumentType == DocumentType::Prefab)
  {
    const plPrefabDocumentSettings* pSettings = GetSettings<plPrefabDocumentSettings>();

    if (GetEditorEngineConnection() != nullptr)
    {
      plExposedDocumentObjectPropertiesMsgToEngine msg;
      msg.m_Properties = pSettings->m_ExposedProperties;

      SendMessageToEngine(&msg);
    }
  }
  return RequestExportScene(szTargetFile, AssetHeader);
}


plTransformStatus plSceneDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  PL_ASSERT_NOT_IMPLEMENTED;

  /* this function is never called */
  return plStatus(PL_FAILURE);
}


plTransformStatus plSceneDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  plStatus status = plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo, {});

  // if we were to do this BEFORE making the screenshot, the scene would be killed, but not immediately restored
  // and the screenshot would end up empty
  // instead the engine side ensures simulation is stopped and makes a screenshot of whatever state is visible
  // but then the editor and engine state are out of sync, so AFTER the screenshot is done,
  // we ensure to also stop simulation on the editor side
  StopGameMode();

  return status;
}

void plSceneDocument::SyncObjectHiddenState()
{
  // #TODO Scene2 handling
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void plSceneDocument::SyncObjectHiddenState(plDocumentObject* pObject)
{
  const bool bHidden = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid())->m_bHidden;
  m_DocumentObjectMetaData->EndReadMetaData();

  plObjectTagMsgToEngine msg;
  msg.m_bSetTag = bHidden;
  msg.m_sTag = "EditorHidden";

  SendObjectMsg(pObject, &msg);

  for (auto pChild : pObject->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void plSceneDocument::UpdateObjectDebugTargets()
{
  plGatherObjectsForDebugVisMsgInterDoc msg;
  BroadcastInterDocumentMessage(&msg, this);

  {
    plObjectsForDebugVisMsgToEngine msgToEngine;
    msgToEngine.m_Objects.SetCountUninitialized(sizeof(plUuid) * msg.m_Objects.GetCount());

    plMemoryUtils::Copy<plUInt8>(msgToEngine.m_Objects.GetData(), reinterpret_cast<plUInt8*>(msg.m_Objects.GetData()), msgToEngine.m_Objects.GetCount());

    GetEditorEngineConnection()->SendMessage(&msgToEngine);
  }
}
