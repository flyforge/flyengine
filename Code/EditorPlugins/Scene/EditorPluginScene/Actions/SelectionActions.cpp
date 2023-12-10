#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QFileDialog>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSelectionAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plActionDescriptorHandle plSelectionActions::s_hGroupSelectedItems;
plActionDescriptorHandle plSelectionActions::s_hCreateEmptyChildObject;
plActionDescriptorHandle plSelectionActions::s_hCreateEmptyObjectAtPosition;
plActionDescriptorHandle plSelectionActions::s_hHideSelectedObjects;
plActionDescriptorHandle plSelectionActions::s_hHideUnselectedObjects;
plActionDescriptorHandle plSelectionActions::s_hShowHiddenObjects;
plActionDescriptorHandle plSelectionActions::s_hPrefabMenu;
plActionDescriptorHandle plSelectionActions::s_hCreatePrefab;
plActionDescriptorHandle plSelectionActions::s_hRevertPrefab;
plActionDescriptorHandle plSelectionActions::s_hUnlinkFromPrefab;
plActionDescriptorHandle plSelectionActions::s_hOpenPrefabDocument;
plActionDescriptorHandle plSelectionActions::s_hDuplicateSpecial;
plActionDescriptorHandle plSelectionActions::s_hDeltaTransform;
plActionDescriptorHandle plSelectionActions::s_hSnapObjectToCamera;
plActionDescriptorHandle plSelectionActions::s_hAttachToObject;
plActionDescriptorHandle plSelectionActions::s_hDetachFromParent;
plActionDescriptorHandle plSelectionActions::s_hConvertToEnginePrefab;
plActionDescriptorHandle plSelectionActions::s_hConvertToEditorPrefab;
plActionDescriptorHandle plSelectionActions::s_hCopyReference;



void plSelectionActions::RegisterActions()
{
  s_hGroupSelectedItems = PLASMA_REGISTER_ACTION_1("Selection.GroupItems", plActionScope::Document, "Scene - Selection", "Ctrl+G", plSelectionAction,
    plSelectionAction::ActionType::GroupSelectedItems);
  s_hCreateEmptyChildObject = PLASMA_REGISTER_ACTION_1("Selection.CreateEmptyChildObject", plActionScope::Document, "Scene - Selection", "",
    plSelectionAction, plSelectionAction::ActionType::CreateEmptyChildObject);
  s_hCreateEmptyObjectAtPosition = PLASMA_REGISTER_ACTION_1("Selection.CreateEmptyObjectAtPosition", plActionScope::Document, "Scene - Selection",
    "Ctrl+Shift+X", plSelectionAction, plSelectionAction::ActionType::CreateEmptyObjectAtPosition);
  s_hHideSelectedObjects = PLASMA_REGISTER_ACTION_1(
    "Selection.HideItems", plActionScope::Document, "Scene - Selection", "H", plSelectionAction, plSelectionAction::ActionType::HideSelectedObjects);
  s_hHideUnselectedObjects = PLASMA_REGISTER_ACTION_1("Selection.HideUnselectedItems", plActionScope::Document, "Scene - Selection", "Shift+H",
    plSelectionAction, plSelectionAction::ActionType::HideUnselectedObjects);
  s_hShowHiddenObjects = PLASMA_REGISTER_ACTION_1("Selection.ShowHidden", plActionScope::Document, "Scene - Selection", "Ctrl+H", plSelectionAction,
    plSelectionAction::ActionType::ShowHiddenObjects);
  s_hAttachToObject = PLASMA_REGISTER_ACTION_1(
    "Selection.Attach", plActionScope::Document, "Scene - Selection", "", plSelectionAction, plSelectionAction::ActionType::AttachToObject);
  s_hDetachFromParent = PLASMA_REGISTER_ACTION_1(
    "Selection.Detach", plActionScope::Document, "Scene - Selection", "", plSelectionAction, plSelectionAction::ActionType::DetachFromParent);

  s_hPrefabMenu = PLASMA_REGISTER_MENU_WITH_ICON("Prefabs.Menu", ":/AssetIcons/Prefab.svg");
  s_hCreatePrefab =
    PLASMA_REGISTER_ACTION_1("Prefabs.Create", plActionScope::Document, "Prefabs", "", plSelectionAction, plSelectionAction::ActionType::CreatePrefab);
  s_hRevertPrefab =
    PLASMA_REGISTER_ACTION_1("Prefabs.Revert", plActionScope::Document, "Prefabs", "", plSelectionAction, plSelectionAction::ActionType::RevertPrefab);
  s_hUnlinkFromPrefab = PLASMA_REGISTER_ACTION_1(
    "Prefabs.Unlink", plActionScope::Document, "Prefabs", "", plSelectionAction, plSelectionAction::ActionType::UnlinkFromPrefab);
  s_hOpenPrefabDocument = PLASMA_REGISTER_ACTION_1(
    "Prefabs.OpenDocument", plActionScope::Document, "Prefabs", "", plSelectionAction, plSelectionAction::ActionType::OpenPrefabDocument);
  s_hConvertToEnginePrefab = PLASMA_REGISTER_ACTION_1(
    "Prefabs.ConvertToEngine", plActionScope::Document, "Prefabs", "", plSelectionAction, plSelectionAction::ActionType::ConvertToEnginePrefab);
  s_hConvertToEditorPrefab = PLASMA_REGISTER_ACTION_1(
    "Prefabs.ConvertToEditor", plActionScope::Document, "Prefabs", "", plSelectionAction, plSelectionAction::ActionType::ConvertToEditorPrefab);

  s_hDuplicateSpecial = PLASMA_REGISTER_ACTION_1("Selection.DuplicateSpecial", plActionScope::Document, "Scene - Selection", "Ctrl+D", plSelectionAction,
    plSelectionAction::ActionType::DuplicateSpecial);
  s_hDeltaTransform = PLASMA_REGISTER_ACTION_1("Selection.DeltaTransform", plActionScope::Document, "Scene - Selection", "Ctrl+M", plSelectionAction,
    plSelectionAction::ActionType::DeltaTransform);
  s_hSnapObjectToCamera = PLASMA_REGISTER_ACTION_1(
    "Scene.Camera.SnapObjectToCamera", plActionScope::Document, "Camera", "", plSelectionAction, plSelectionAction::ActionType::SnapObjectToCamera);
  s_hCopyReference = PLASMA_REGISTER_ACTION_1(
    "Selection.CopyReference", plActionScope::Document, "Scene - Selection", "", plSelectionAction, plSelectionAction::ActionType::CopyReference);
}

void plSelectionActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hGroupSelectedItems);
  plActionManager::UnregisterAction(s_hCreateEmptyChildObject);
  plActionManager::UnregisterAction(s_hCreateEmptyObjectAtPosition);
  plActionManager::UnregisterAction(s_hHideSelectedObjects);
  plActionManager::UnregisterAction(s_hHideUnselectedObjects);
  plActionManager::UnregisterAction(s_hShowHiddenObjects);
  plActionManager::UnregisterAction(s_hPrefabMenu);
  plActionManager::UnregisterAction(s_hCreatePrefab);
  plActionManager::UnregisterAction(s_hRevertPrefab);
  plActionManager::UnregisterAction(s_hUnlinkFromPrefab);
  plActionManager::UnregisterAction(s_hOpenPrefabDocument);
  plActionManager::UnregisterAction(s_hDuplicateSpecial);
  plActionManager::UnregisterAction(s_hDeltaTransform);
  plActionManager::UnregisterAction(s_hSnapObjectToCamera);
  plActionManager::UnregisterAction(s_hAttachToObject);
  plActionManager::UnregisterAction(s_hDetachFromParent);
  plActionManager::UnregisterAction(s_hConvertToEditorPrefab);
  plActionManager::UnregisterAction(s_hConvertToEnginePrefab);
  plActionManager::UnregisterAction(s_hCopyReference);
}

void plSelectionActions::MapActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCreateEmptyChildObject, "G.Selection", 1.0f);
  pMap->MapAction(s_hCreateEmptyObjectAtPosition, "G.Selection", 1.1f);
  pMap->MapAction(s_hGroupSelectedItems, "G.Selection", 3.7f);
  pMap->MapAction(s_hHideSelectedObjects, "G.Selection", 4.0f);
  pMap->MapAction(s_hHideUnselectedObjects, "G.Selection", 5.0f);
  pMap->MapAction(s_hShowHiddenObjects, "G.Selection", 6.0f);
  pMap->MapAction(s_hDuplicateSpecial, "G.Selection", 7.0f);
  pMap->MapAction(s_hDeltaTransform, "G.Selection", 7.1f);
  pMap->MapAction(s_hAttachToObject, "G.Selection", 7.2f);
  pMap->MapAction(s_hDetachFromParent, "G.Selection", 7.3f);
  pMap->MapAction(s_hSnapObjectToCamera, "G.Selection", 9.0f);
  pMap->MapAction(s_hCopyReference, "G.Selection", 10.0f);

  MapPrefabActions(sMapping, 0.0f);
}

void plSelectionActions::MapPrefabActions(plStringView sMapping, float fPriority)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hPrefabMenu, "G.Selection", fPriority);

  pMap->MapAction(s_hOpenPrefabDocument, "G.Selection", "Prefabs.Menu", 1.0f);
  pMap->MapAction(s_hRevertPrefab, "G.Selection", "Prefabs.Menu", 2.0f);
  pMap->MapAction(s_hCreatePrefab, "G.Selection", "Prefabs.Menu", 3.0f);
  pMap->MapAction(s_hUnlinkFromPrefab, "G.Selection", "Prefabs.Menu", 4.0f);
  pMap->MapAction(s_hConvertToEditorPrefab, "G.Selection", "Prefabs.Menu", 5.0f);
  pMap->MapAction(s_hConvertToEnginePrefab, "G.Selection", "Prefabs.Menu", 6.0f);
}

void plSelectionActions::MapContextMenuActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCreateEmptyChildObject, "G.Selection", 0.5f);
  pMap->MapAction(s_hGroupSelectedItems, "G.Selection", 2.0f);
  pMap->MapAction(s_hHideSelectedObjects, "G.Selection", 3.0f);
  pMap->MapAction(s_hDetachFromParent, "G.Selection", 3.2f);
  pMap->MapAction(s_hCopyReference, "G.Selection", 4.0f);

  MapPrefabActions(sMapping, 4.0f);
}


void plSelectionActions::MapViewContextMenuActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hGroupSelectedItems, "G.Selection", 2.0f);
  pMap->MapAction(s_hHideSelectedObjects, "G.Selection", 3.0f);
  pMap->MapAction(s_hAttachToObject, "G.Selection", 3.1f);
  pMap->MapAction(s_hDetachFromParent, "G.Selection", 3.2f);
  pMap->MapAction(s_hSnapObjectToCamera, "G.Selection", 5.0f);
  pMap->MapAction(s_hCopyReference, "G.Selection", 6.0f);

  MapPrefabActions(sMapping, 7.0f);
}

plSelectionAction::plSelectionAction(const plActionContext& context, const char* szName, plSelectionAction::ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<plSceneDocument*>(static_cast<const plSceneDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::GroupSelectedItems:
      SetIconPath(":/EditorPluginScene/Icons/GroupSelection.svg");
      break;
    case ActionType::CreateEmptyChildObject:
      SetIconPath(":/EditorPluginScene/Icons/CreateNode.svg");
      break;
    case ActionType::CreateEmptyObjectAtPosition:
      SetIconPath(":/EditorPluginScene/Icons/CreateNode.svg");
      break;
    case ActionType::HideSelectedObjects:
      SetIconPath(":/EditorPluginScene/Icons/HideSelected.svg");
      break;
    case ActionType::HideUnselectedObjects:
      SetIconPath(":/EditorPluginScene/Icons/HideUnselected.svg");
      break;
    case ActionType::ShowHiddenObjects:
      SetIconPath(":/EditorPluginScene/Icons/ShowHidden.svg");
      break;
    case ActionType::CreatePrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabCreate.svg");
      break;
    case ActionType::RevertPrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabRevert.svg");
      break;
    case ActionType::UnlinkFromPrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabUnlink.svg");
      break;
    case ActionType::OpenPrefabDocument:
      SetIconPath(":/EditorPluginScene/Icons/PrefabOpenDocument.svg");
      break;
    case ActionType::DuplicateSpecial:
      SetIconPath(":/EditorPluginScene/Icons/Duplicate.svg");
      break;
    case ActionType::DeltaTransform:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::SnapObjectToCamera:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::AttachToObject:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::DetachFromParent:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::ConvertToEditorPrefab:
      // SetIconPath(":/EditorPluginScene/PrefabRevert.png"); // TODO Icon
      break;
    case ActionType::ConvertToEnginePrefab:
      // SetIconPath(":/EditorPluginScene/PrefabRevert.png"); // TODO Icon
      break;
    case ActionType::CopyReference:
      // SetIconPath(":/EditorPluginScene/PrefabRevert.png"); // TODO Icon
      break;
  }

  UpdateEnableState();

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plSelectionAction::SelectionEventHandler, this));
}


plSelectionAction::~plSelectionAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plSelectionAction::SelectionEventHandler, this));
}

void plSelectionAction::Execute(const plVariant& value)
{
  switch (m_Type)
  {
    case ActionType::GroupSelectedItems:
      m_pSceneDocument->GroupSelection();
      return;
    case ActionType::CreateEmptyChildObject:
    {
      auto res = m_pSceneDocument->CreateEmptyObject(true, false);
      plQtUiServices::MessageBoxStatus(res, "Object creation failed.");
      return;
    }
    case ActionType::CreateEmptyObjectAtPosition:
    {
      auto res = m_pSceneDocument->CreateEmptyObject(false, true);
      plQtUiServices::MessageBoxStatus(res, "Object creation failed.");
      return;
    }
    case ActionType::HideSelectedObjects:
      m_pSceneDocument->ShowOrHideSelectedObjects(plSceneDocument::ShowOrHide::Hide);
      m_pSceneDocument->ShowDocumentStatus("Hiding selected objects");
      break;
    case ActionType::HideUnselectedObjects:
      m_pSceneDocument->HideUnselectedObjects();
      m_pSceneDocument->ShowDocumentStatus("Hiding unselected objects");
      break;
    case ActionType::ShowHiddenObjects:
      m_pSceneDocument->ShowOrHideAllObjects(plSceneDocument::ShowOrHide::Show);
      m_pSceneDocument->ShowDocumentStatus("Showing hidden objects");
      break;
    case ActionType::CreatePrefab:
      CreatePrefab();
      break;

    case ActionType::RevertPrefab:
    {
      if (plQtUiServices::MessageBoxQuestion("Discard all modifications to the selected prefabs and revert to the prefab template state?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        const plDeque<const plDocumentObject*> sel = m_pSceneDocument->GetSelectionManager()->GetTopLevelSelection(plGetStaticRTTI<plGameObject>());
        m_pSceneDocument->RevertPrefabs(sel);
      }
    }
    break;

    case ActionType::UnlinkFromPrefab:
    {
      if (plQtUiServices::MessageBoxQuestion("Unlink the selected prefab instances from their templates?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        const plDeque<const plDocumentObject*> sel = m_pSceneDocument->GetSelectionManager()->GetTopLevelSelection(plGetStaticRTTI<plGameObject>());
        m_pSceneDocument->UnlinkPrefabs(sel);
      }
    }
    break;

    case ActionType::OpenPrefabDocument:
      OpenPrefabDocument();
      break;

    case ActionType::DuplicateSpecial:
      m_pSceneDocument->DuplicateSpecial();
      break;

    case ActionType::DeltaTransform:
      m_pSceneDocument->DeltaTransform();
      break;

    case ActionType::SnapObjectToCamera:
      m_pSceneDocument->SnapObjectToCamera();
      break;

    case ActionType::AttachToObject:
      m_pSceneDocument->AttachToObject();
      break;
    case ActionType::DetachFromParent:
      m_pSceneDocument->DetachFromParent();
      break;

    case ActionType::CopyReference:
      m_pSceneDocument->CopyReference();
      break;

    case ActionType::ConvertToEditorPrefab:
    {
      const plDeque<const plDocumentObject*> sel = m_pSceneDocument->GetSelectionManager()->GetTopLevelSelection(plGetStaticRTTI<plGameObject>());
      m_pSceneDocument->ConvertToEditorPrefab(sel);
    }
    break;

    case ActionType::ConvertToEnginePrefab:
    {
      if (plQtUiServices::MessageBoxQuestion("Discard all modifications to the selected prefabs and convert them to engine prefabs?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        const plDeque<const plDocumentObject*> sel = m_pSceneDocument->GetSelectionManager()->GetTopLevelSelection(plGetStaticRTTI<plGameObject>());
        m_pSceneDocument->ConvertToEnginePrefab(sel);
      }
    }
    break;
  }
}


void plSelectionAction::OpenPrefabDocument()
{
  const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

  if (sel.GetCount() != 1)
    return;

  const plSceneDocument* pScene = static_cast<const plSceneDocument*>(m_Context.m_pDocument);


  plUuid PrefabAsset;
  if (pScene->IsObjectEnginePrefab(sel[0]->GetGuid(), &PrefabAsset))
  {
    // PrefabAsset is all we need
  }
  else
  {
    auto pMeta = pScene->m_DocumentObjectMetaData->BeginReadMetaData(sel[0]->GetGuid());
    PrefabAsset = pMeta->m_CreateFromPrefab;
    pScene->m_DocumentObjectMetaData->EndReadMetaData();
  }

  auto pAsset = plAssetCurator::GetSingleton()->GetSubAsset(PrefabAsset);
  if (pAsset)
  {
    plQtEditorApp::GetSingleton()->OpenDocumentQueued(pAsset->m_pAssetInfo->m_Path.GetAbsolutePath());
  }
  else
  {
    plQtUiServices::MessageBoxWarning("The prefab asset of this instance is currently unknown. It may have been deleted. Try updating the "
                                      "asset library ('Check FileSystem'), if it should be there.");
  }
}

void plSelectionAction::CreatePrefab()
{
  static plString sSearchDir = plToolsProject::GetSingleton()->GetProjectFile();

  plStringBuilder sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Prefab"),
    QString::fromUtf8(sSearchDir.GetData()), QString::fromUtf8("*.plPrefab"), nullptr, QFileDialog::Option::DontResolveSymlinks)
                            .toUtf8()
                            .data();

  if (!sFile.IsEmpty())
  {
    sFile.ChangeFileExtension("plPrefab");

    sSearchDir = sFile.GetFileDirectory();

    if (plOSFile::ExistsFile(sFile))
    {
      plQtUiServices::MessageBoxInformation("You currently cannot replace an existing prefab this way. Please choose a new prefab file.");
      return;
    }

    auto res = m_pSceneDocument->CreatePrefabDocumentFromSelection(sFile, plGetStaticRTTI<plGameObject>());
    m_pSceneDocument->ScheduleSendObjectSelection(); // fix selection of prefab object
    plQtUiServices::MessageBoxStatus(res, "Failed to create Prefab", "Successfully created Prefab");
  }
}

void plSelectionAction::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  UpdateEnableState();
}

void plSelectionAction::UpdateEnableState()
{
  if (m_Type == ActionType::HideSelectedObjects || m_Type == ActionType::DuplicateSpecial || m_Type == ActionType::DeltaTransform ||
      m_Type == ActionType::SnapObjectToCamera || m_Type == ActionType::DetachFromParent || m_Type == ActionType::HideUnselectedObjects ||
      m_Type == ActionType::AttachToObject)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }

  if (m_Type == ActionType::GroupSelectedItems)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() > 1);
  }

  if (m_Type == ActionType::CreateEmptyChildObject)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() <= 1);
  }

  if (m_Type == ActionType::CopyReference)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() == 1);
  }

  if (m_Type == ActionType::OpenPrefabDocument)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.GetCount() != 1)
    {
      SetEnabled(false);
      return;
    }

    const plSceneDocument* pScene = static_cast<const plSceneDocument*>(m_Context.m_pDocument);
    const bool bIsPrefab = pScene->IsObjectEditorPrefab(sel[0]->GetGuid()) || pScene->IsObjectEnginePrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab);
    return;
  }

  if (m_Type == ActionType::RevertPrefab || m_Type == ActionType::UnlinkFromPrefab || m_Type == ActionType::ConvertToEnginePrefab ||
      m_Type == ActionType::CreatePrefab)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.IsEmpty())
    {
      SetEnabled(false);
      return;
    }

    if (m_Type == ActionType::CreatePrefab)
    {
      SetEnabled(true);
      return;
    }

    const bool bShouldBePrefab =
      (m_Type == ActionType::RevertPrefab) || (m_Type == ActionType::ConvertToEnginePrefab) || (m_Type == ActionType::UnlinkFromPrefab);

    const plSceneDocument* pScene = static_cast<const plSceneDocument*>(m_Context.m_pDocument);
    const bool bIsPrefab = pScene->IsObjectEditorPrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab == bShouldBePrefab);
  }

  if (m_Type == ActionType::ConvertToEditorPrefab)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.IsEmpty())
    {
      SetEnabled(false);
      return;
    }

    const plSceneDocument* pScene = static_cast<const plSceneDocument*>(m_Context.m_pDocument);
    const bool bIsPrefab = pScene->IsObjectEnginePrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab);
  }
}
