#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Dialogs/ExportAndRunDlg.moc.h>
#include <EditorPluginScene/Dialogs/ExtractGeometryDlg.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QProcess>
#include <SharedPluginScene/Common/Messages.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plSceneActions::s_hSceneCategory;
plActionDescriptorHandle plSceneActions::s_hSceneUtilsMenu;
plActionDescriptorHandle plSceneActions::s_hExportScene;
plActionDescriptorHandle plSceneActions::s_hGameModeSimulate;
plActionDescriptorHandle plSceneActions::s_hGameModePlay;
plActionDescriptorHandle plSceneActions::s_hGameModePlayFromHere;
plActionDescriptorHandle plSceneActions::s_hGameModeStop;
plActionDescriptorHandle plSceneActions::s_hUtilExportSceneToOBJ;
plActionDescriptorHandle plSceneActions::s_hKeepSimulationChanges;
plActionDescriptorHandle plSceneActions::s_hCreateThumbnail;
plActionDescriptorHandle plSceneActions::s_hFavoriteCamsMenu;
plActionDescriptorHandle plSceneActions::s_hStoreEditorCamera[10];
plActionDescriptorHandle plSceneActions::s_hRestoreEditorCamera[10];
plActionDescriptorHandle plSceneActions::s_hJumpToCamera[10];
plActionDescriptorHandle plSceneActions::s_hCreateLevelCamera[10];

void plSceneActions::RegisterActions()
{
  s_hSceneCategory = PLASMA_REGISTER_CATEGORY("SceneCategory");
  s_hSceneUtilsMenu = PLASMA_REGISTER_MENU_WITH_ICON("Scene.Utils.Menu", "");

  s_hExportScene = PLASMA_REGISTER_ACTION_1("Scene.ExportAndRun", plActionScope::Document, "Scene", "Ctrl+R", plSceneAction, plSceneAction::ActionType::ExportAndRunScene);
  s_hGameModeSimulate = PLASMA_REGISTER_ACTION_1("Scene.GameMode.Simulate", plActionScope::Document, "Scene", "F5", plSceneAction, plSceneAction::ActionType::StartGameModeSimulate);
  s_hGameModePlay = PLASMA_REGISTER_ACTION_1("Scene.GameMode.Play", plActionScope::Document, "Scene", "Ctrl+F5", plSceneAction, plSceneAction::ActionType::StartGameModePlay);

  s_hGameModePlayFromHere = PLASMA_REGISTER_ACTION_1("Scene.GameMode.PlayFromHere", plActionScope::Document, "Scene", "Ctrl+Shift+F5", plSceneAction,
    plSceneAction::ActionType::StartGameModePlayFromHere);

  s_hGameModeStop = PLASMA_REGISTER_ACTION_1("Scene.GameMode.Stop", plActionScope::Document, "Scene", "Shift+F5", plSceneAction, plSceneAction::ActionType::StopGameMode);

  s_hUtilExportSceneToOBJ = PLASMA_REGISTER_ACTION_1("Scene.ExportSceneToOBJ", plActionScope::Document, "Scene", "", plSceneAction, plSceneAction::ActionType::ExportSceneToOBJ);

  s_hKeepSimulationChanges = PLASMA_REGISTER_ACTION_1("Scene.KeepSimulationChanges", plActionScope::Document, "Scene", "K", plSceneAction, plSceneAction::ActionType::KeepSimulationChanges);

  s_hCreateThumbnail = PLASMA_REGISTER_ACTION_1("Scene.CreateThumbnail", plActionScope::Document, "Scene", "", plSceneAction, plSceneAction::ActionType::CreateThumbnail);
  // unfortunately the macros use lambdas thus using a loop to generate the strings does not work
  {
    s_hFavoriteCamsMenu = PLASMA_REGISTER_MENU_WITH_ICON("Scene.FavoriteCams.Menu", "");

    s_hStoreEditorCamera[0] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Store.0", plActionScope::Document, "Scene - Cameras", "Ctrl+0", plSceneAction, plSceneAction::ActionType::StoreEditorCamera0);
    s_hStoreEditorCamera[1] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Store.1", plActionScope::Document, "Scene - Cameras", "Ctrl+1", plSceneAction, plSceneAction::ActionType::StoreEditorCamera1);
    s_hStoreEditorCamera[2] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Store.2", plActionScope::Document, "Scene - Cameras", "Ctrl+2", plSceneAction, plSceneAction::ActionType::StoreEditorCamera2);
    s_hStoreEditorCamera[3] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Store.3", plActionScope::Document, "Scene - Cameras", "Ctrl+3", plSceneAction, plSceneAction::ActionType::StoreEditorCamera3);
    s_hStoreEditorCamera[4] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Store.4", plActionScope::Document, "Scene - Cameras", "Ctrl+4", plSceneAction, plSceneAction::ActionType::StoreEditorCamera4);
    s_hStoreEditorCamera[5] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Store.5", plActionScope::Document, "Scene - Cameras", "Ctrl+5", plSceneAction, plSceneAction::ActionType::StoreEditorCamera5);
    s_hStoreEditorCamera[6] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Store.6", plActionScope::Document, "Scene - Cameras", "Ctrl+6", plSceneAction, plSceneAction::ActionType::StoreEditorCamera6);
    s_hStoreEditorCamera[7] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Store.7", plActionScope::Document, "Scene - Cameras", "Ctrl+7", plSceneAction, plSceneAction::ActionType::StoreEditorCamera7);
    s_hStoreEditorCamera[8] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Store.8", plActionScope::Document, "Scene - Cameras", "Ctrl+8", plSceneAction, plSceneAction::ActionType::StoreEditorCamera8);
    s_hStoreEditorCamera[9] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Store.9", plActionScope::Document, "Scene - Cameras", "Ctrl+9", plSceneAction, plSceneAction::ActionType::StoreEditorCamera9);

    s_hRestoreEditorCamera[0] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Restore.0", plActionScope::Document, "Scene - Cameras", "0", plSceneAction, plSceneAction::ActionType::RestoreEditorCamera0);
    s_hRestoreEditorCamera[1] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Restore.1", plActionScope::Document, "Scene - Cameras", "1", plSceneAction, plSceneAction::ActionType::RestoreEditorCamera1);
    s_hRestoreEditorCamera[2] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Restore.2", plActionScope::Document, "Scene - Cameras", "2", plSceneAction, plSceneAction::ActionType::RestoreEditorCamera2);
    s_hRestoreEditorCamera[3] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Restore.3", plActionScope::Document, "Scene - Cameras", "3", plSceneAction, plSceneAction::ActionType::RestoreEditorCamera3);
    s_hRestoreEditorCamera[4] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Restore.4", plActionScope::Document, "Scene - Cameras", "4", plSceneAction, plSceneAction::ActionType::RestoreEditorCamera4);
    s_hRestoreEditorCamera[5] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Restore.5", plActionScope::Document, "Scene - Cameras", "5", plSceneAction, plSceneAction::ActionType::RestoreEditorCamera5);
    s_hRestoreEditorCamera[6] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Restore.6", plActionScope::Document, "Scene - Cameras", "6", plSceneAction, plSceneAction::ActionType::RestoreEditorCamera6);
    s_hRestoreEditorCamera[7] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Restore.7", plActionScope::Document, "Scene - Cameras", "7", plSceneAction, plSceneAction::ActionType::RestoreEditorCamera7);
    s_hRestoreEditorCamera[8] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Restore.8", plActionScope::Document, "Scene - Cameras", "8", plSceneAction, plSceneAction::ActionType::RestoreEditorCamera8);
    s_hRestoreEditorCamera[9] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Restore.9", plActionScope::Document, "Scene - Cameras", "9", plSceneAction, plSceneAction::ActionType::RestoreEditorCamera9);

    s_hJumpToCamera[0] = PLASMA_REGISTER_ACTION_1("Scene.Camera.JumpTo.0", plActionScope::Document, "Scene - Cameras", "Alt+0", plSceneAction, plSceneAction::ActionType::JumpToCamera0);
    s_hJumpToCamera[1] = PLASMA_REGISTER_ACTION_1("Scene.Camera.JumpTo.1", plActionScope::Document, "Scene - Cameras", "Alt+1", plSceneAction, plSceneAction::ActionType::JumpToCamera1);
    s_hJumpToCamera[2] = PLASMA_REGISTER_ACTION_1("Scene.Camera.JumpTo.2", plActionScope::Document, "Scene - Cameras", "Alt+2", plSceneAction, plSceneAction::ActionType::JumpToCamera2);
    s_hJumpToCamera[3] = PLASMA_REGISTER_ACTION_1("Scene.Camera.JumpTo.3", plActionScope::Document, "Scene - Cameras", "Alt+3", plSceneAction, plSceneAction::ActionType::JumpToCamera3);
    s_hJumpToCamera[4] = PLASMA_REGISTER_ACTION_1("Scene.Camera.JumpTo.4", plActionScope::Document, "Scene - Cameras", "Alt+4", plSceneAction, plSceneAction::ActionType::JumpToCamera4);
    s_hJumpToCamera[5] = PLASMA_REGISTER_ACTION_1("Scene.Camera.JumpTo.5", plActionScope::Document, "Scene - Cameras", "Alt+5", plSceneAction, plSceneAction::ActionType::JumpToCamera5);
    s_hJumpToCamera[6] = PLASMA_REGISTER_ACTION_1("Scene.Camera.JumpTo.6", plActionScope::Document, "Scene - Cameras", "Alt+6", plSceneAction, plSceneAction::ActionType::JumpToCamera6);
    s_hJumpToCamera[7] = PLASMA_REGISTER_ACTION_1("Scene.Camera.JumpTo.7", plActionScope::Document, "Scene - Cameras", "Alt+7", plSceneAction, plSceneAction::ActionType::JumpToCamera7);
    s_hJumpToCamera[8] = PLASMA_REGISTER_ACTION_1("Scene.Camera.JumpTo.8", plActionScope::Document, "Scene - Cameras", "Alt+8", plSceneAction, plSceneAction::ActionType::JumpToCamera8);
    s_hJumpToCamera[9] = PLASMA_REGISTER_ACTION_1("Scene.Camera.JumpTo.9", plActionScope::Document, "Scene - Cameras", "Alt+9", plSceneAction, plSceneAction::ActionType::JumpToCamera9);

    s_hCreateLevelCamera[0] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Create.0", plActionScope::Document, "Scene - Cameras", "Ctrl+Alt+0", plSceneAction, plSceneAction::ActionType::CreateLevelCamera0);
    s_hCreateLevelCamera[1] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Create.1", plActionScope::Document, "Scene - Cameras", "Ctrl+Alt+1", plSceneAction, plSceneAction::ActionType::CreateLevelCamera1);
    s_hCreateLevelCamera[2] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Create.2", plActionScope::Document, "Scene - Cameras", "Ctrl+Alt+2", plSceneAction, plSceneAction::ActionType::CreateLevelCamera2);
    s_hCreateLevelCamera[3] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Create.3", plActionScope::Document, "Scene - Cameras", "Ctrl+Alt+3", plSceneAction, plSceneAction::ActionType::CreateLevelCamera3);
    s_hCreateLevelCamera[4] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Create.4", plActionScope::Document, "Scene - Cameras", "Ctrl+Alt+4", plSceneAction, plSceneAction::ActionType::CreateLevelCamera4);
    s_hCreateLevelCamera[5] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Create.5", plActionScope::Document, "Scene - Cameras", "Ctrl+Alt+5", plSceneAction, plSceneAction::ActionType::CreateLevelCamera5);
    s_hCreateLevelCamera[6] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Create.6", plActionScope::Document, "Scene - Cameras", "Ctrl+Alt+6", plSceneAction, plSceneAction::ActionType::CreateLevelCamera6);
    s_hCreateLevelCamera[7] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Create.7", plActionScope::Document, "Scene - Cameras", "Ctrl+Alt+7", plSceneAction, plSceneAction::ActionType::CreateLevelCamera7);
    s_hCreateLevelCamera[8] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Create.8", plActionScope::Document, "Scene - Cameras", "Ctrl+Alt+8", plSceneAction, plSceneAction::ActionType::CreateLevelCamera8);
    s_hCreateLevelCamera[9] = PLASMA_REGISTER_ACTION_1("Scene.Camera.Create.9", plActionScope::Document, "Scene - Cameras", "Ctrl+Alt+9", plSceneAction, plSceneAction::ActionType::CreateLevelCamera9);
  }
}

void plSceneActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hSceneCategory);
  plActionManager::UnregisterAction(s_hSceneUtilsMenu);
  plActionManager::UnregisterAction(s_hExportScene);
  plActionManager::UnregisterAction(s_hGameModeSimulate);
  plActionManager::UnregisterAction(s_hGameModePlay);
  plActionManager::UnregisterAction(s_hGameModePlayFromHere);
  plActionManager::UnregisterAction(s_hGameModeStop);
  plActionManager::UnregisterAction(s_hUtilExportSceneToOBJ);
  plActionManager::UnregisterAction(s_hKeepSimulationChanges);
  plActionManager::UnregisterAction(s_hCreateThumbnail);
  plActionManager::UnregisterAction(s_hFavoriteCamsMenu);

  for (plUInt32 i = 0; i < 10; ++i)
  {
    plActionManager::UnregisterAction(s_hStoreEditorCamera[i]);
    plActionManager::UnregisterAction(s_hRestoreEditorCamera[i]);
    plActionManager::UnregisterAction(s_hJumpToCamera[i]);
    plActionManager::UnregisterAction(s_hCreateLevelCamera[i]);
  }
}

void plSceneActions::MapMenuActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "G.Scene/SceneCategory";
    const char* szUtilsSubPath = "G.Scene/Scene.Utils.Menu";

    pMap->MapAction(s_hSceneUtilsMenu, "G.Scene", 2.0f);
    // pMap->MapAction(s_hCreateThumbnail, szUtilsSubPath, 0.0f); // now available through the export scene dialog
    pMap->MapAction(s_hKeepSimulationChanges, szUtilsSubPath, 1.0f);
    pMap->MapAction(s_hUtilExportSceneToOBJ, szUtilsSubPath, 2.0f);

    pMap->MapAction(s_hFavoriteCamsMenu, "G.Scene", 3.0f);
    const char* szFavCamsSubPath = "G.Scene/Scene.FavoriteCams.Menu";

    for (plUInt32 i = 0; i < 10; ++i)
    {
      pMap->MapAction(s_hStoreEditorCamera[i], szFavCamsSubPath, 10.0f + i);
      pMap->MapAction(s_hRestoreEditorCamera[i], szFavCamsSubPath, 20.0f + i);
      pMap->MapAction(s_hJumpToCamera[i], szFavCamsSubPath, 30.0f + i);
      pMap->MapAction(s_hCreateLevelCamera[i], szFavCamsSubPath, 40.0f + i);
    }

    pMap->MapAction(s_hSceneCategory, "G.Scene", 4.0f);
    pMap->MapAction(s_hExportScene, szSubPath, 1.0f);
    pMap->MapAction(s_hGameModeStop, szSubPath, 4.0f);
    pMap->MapAction(s_hGameModeSimulate, szSubPath, 5.0f);
    pMap->MapAction(s_hGameModePlay, szSubPath, 6.0f);
    pMap->MapAction(s_hGameModePlayFromHere, szSubPath, 7.0f);
  }
}

void plSceneActions::MapToolbarActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "SceneCategory";


    /// \todo This works incorrectly with value 6.0f -> it places the action inside the snap category
    pMap->MapAction(s_hSceneCategory, "", 11.0f);
    pMap->MapAction(s_hGameModeStop, szSubPath, 1.0f);
    pMap->MapAction(s_hGameModeSimulate, szSubPath, 2.0f);
    pMap->MapAction(s_hGameModePlay, szSubPath, 3.0f);
    pMap->MapAction(s_hExportScene, szSubPath, 4.0f);
  }
}

void plSceneActions::MapViewContextMenuActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hGameModePlayFromHere, "", 1.0f);
}

plSceneAction::plSceneAction(const plActionContext& context, const char* szName, plSceneAction::ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pSceneDocument = static_cast<plSceneDocument*>(context.m_pDocument);
  m_pSceneDocument->m_GameObjectEvents.AddEventHandler(plMakeDelegate(&plSceneAction::SceneEventHandler, this));

  switch (m_Type)
  {
    case ActionType::ExportAndRunScene:
      SetIconPath(":/EditorPluginScene/Icons/SceneExport.svg");
      break;

    case ActionType::StartGameModeSimulate:
      SetCheckable(true);
      SetIconPath(":/EditorPluginScene/Icons/ScenePlay.svg");
      SetChecked(m_pSceneDocument->GetGameMode() == GameMode::Simulate);
      SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Play);
      break;

    case ActionType::StartGameModePlay:
      SetIconPath(":/EditorPluginScene/Icons/ScenePlayTheGame.svg");
      break;

    case ActionType::StartGameModePlayFromHere:
      SetIconPath(":/EditorPluginScene/Icons/ScenePlayTheGame.svg"); // TODO: icon
      break;

    case ActionType::StopGameMode:
      SetIconPath(":/EditorPluginScene/Icons/SceneStop.svg");
      break;

    case ActionType::ExportSceneToOBJ:
      // SetIconPath(":/EditorPluginScene/Icons/SceneStop.svg"); // TODO: icon
      break;

    case ActionType::KeepSimulationChanges:
      SetIconPath(":/EditorPluginScene/Icons/PullObjectState.svg");
      break;

    case ActionType::CreateThumbnail:
      // SetIconPath(":/EditorPluginScene/Icons/PullObjectState.svg"); // TODO: icon
      break;

    case ActionType::JumpToCamera0:
    case ActionType::JumpToCamera1:
    case ActionType::JumpToCamera2:
    case ActionType::JumpToCamera3:
    case ActionType::JumpToCamera4:
    case ActionType::JumpToCamera5:
    case ActionType::JumpToCamera6:
    case ActionType::JumpToCamera7:
    case ActionType::JumpToCamera8:
    case ActionType::JumpToCamera9:
      SetIconPath(":/TypeIcons/plCameraComponent.svg");
      break;

    default:
      // no icon
      break;
  }

  UpdateState();
}

plSceneAction::~plSceneAction()
{
  m_pSceneDocument->m_GameObjectEvents.RemoveEventHandler(plMakeDelegate(&plSceneAction::SceneEventHandler, this));
}

void plSceneAction::Execute(const plVariant& value)
{
  switch (m_Type)
  {
    case ActionType::ExportAndRunScene:
    {
      plStringBuilder sCmd;
      GetPlayerCommandLine(sCmd);

      plQtExportAndRunDlg dlg(nullptr);
      dlg.m_sCmdLine = sCmd;
      dlg.s_bUpdateThumbnail = false;
      dlg.m_bShowThumbnailCheckbox = !m_pSceneDocument->IsPrefab();

      if (dlg.exec() != QDialog::Accepted)
        return;

      plProgressRange range("Export and Run", 4, true);

      range.BeginNextStep("Build C++");
      if (dlg.s_bCompileCpp)
      {
        if (plCppProject::EnsureCppPluginReady().Failed())
          return;
      }

      bool bDidTransformAll = false;

      range.BeginNextStep("Transform Assets");
      if (dlg.s_bTransformAll)
      {
        if (plAssetCurator::GetSingleton()->TransformAllAssets(plTransformFlags::None).Succeeded())
        {
          // once all assets have been transformed, disable it for the next export
          dlg.s_bTransformAll = false;
          bDidTransformAll = true;
        }
      }

      bool bCreateThumbnail = dlg.s_bUpdateThumbnail;

      range.BeginNextStep("Create Thumbnail");
      if (!m_pSceneDocument->IsPrefab() && !bCreateThumbnail)
      {
        // if the thumbnail doesn't exist, or is very old, update it anyway

        plStringBuilder sThumbnailPath = m_pSceneDocument->GetAssetDocumentManager()->GenerateResourceThumbnailPath(m_pSceneDocument->GetDocumentPath());

        plFileStats stat;
        if (plOSFile::GetFileStats(sThumbnailPath, stat).Failed())
        {
          bCreateThumbnail = true;
        }
        else
        {
          auto tNow = plTimestamp::CurrentTimestamp();
          auto tComp = stat.m_LastModificationTime + plTime::MakeFromHours(24) * 7;

          if (tComp.GetInt64(plSIUnitOfTime::Second) < tNow.GetInt64(plSIUnitOfTime::Second))
          {
            bCreateThumbnail = true;
          }
        }
      }


      // Convert collections
      if (!bDidTransformAll)
      {
        plAssetCurator* pCurator = plAssetCurator::GetSingleton();
        pCurator->TransformAssetsForSceneExport(pCurator->GetActiveAssetProfile());
      }

      dlg.s_bUpdateThumbnail = false;

      range.BeginNextStep("Export Scene");
      if (m_pSceneDocument->ExportScene(bCreateThumbnail).Failed())
      {
        plQtUiServices::GetSingleton()->MessageBoxWarning("Scene export failed.");
        return;
      }

      // send event, so that 3rd party code can hook into this
      {
        plGameObjectDocumentEvent e;
        e.m_Type = plGameObjectDocumentEvent::Type::GameMode_StartingExternal;
        e.m_pDocument = m_pSceneDocument;
        m_pSceneDocument->s_GameObjectDocumentEvents.Broadcast(e);
      }

      if (dlg.m_bRunAfterExport)
      {
        LaunchPlayer(dlg.m_sApplication);
      }

      return;
    }

    case ActionType::StartGameModePlay:
      m_pSceneDocument->TriggerGameModePlay(false);
      return;

    case ActionType::StartGameModePlayFromHere:
      m_pSceneDocument->TriggerGameModePlay(true);
      return;

    case ActionType::StartGameModeSimulate:
      m_pSceneDocument->StartSimulateWorld();
      return;

    case ActionType::StopGameMode:
      m_pSceneDocument->StopGameMode();
      return;

    case ActionType::ExportSceneToOBJ:
    {
      plQtExtractGeometryDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        m_pSceneDocument->ExportSceneGeometry(
          dlg.s_sDestinationFile.toUtf8().data(), dlg.s_bOnlySelection, dlg.s_iExtractionMode, dlg.GetCoordinateSystemTransform());
      }
      return;
    }

    case ActionType::KeepSimulationChanges:
    {
      plPullObjectStateMsgToEngine msg;
      m_pSceneDocument->SendMessageToEngine(&msg);
      return;
    }

    case ActionType::CreateThumbnail:
      m_pSceneDocument->ExportScene(true);
      return;

    case ActionType::StoreEditorCamera0:
    case ActionType::StoreEditorCamera1:
    case ActionType::StoreEditorCamera2:
    case ActionType::StoreEditorCamera3:
    case ActionType::StoreEditorCamera4:
    case ActionType::StoreEditorCamera5:
    case ActionType::StoreEditorCamera6:
    case ActionType::StoreEditorCamera7:
    case ActionType::StoreEditorCamera8:
    case ActionType::StoreEditorCamera9:
    {
      const plInt32 iCamIdx = (int)m_Type - (int)ActionType::StoreEditorCamera0;

      m_pSceneDocument->StoreFavoriteCamera(iCamIdx);
      m_pSceneDocument->ShowDocumentStatus(plFmt("Stored favorite camera position {0}", iCamIdx));

      return;
    }

    case ActionType::RestoreEditorCamera0:
    case ActionType::RestoreEditorCamera1:
    case ActionType::RestoreEditorCamera2:
    case ActionType::RestoreEditorCamera3:
    case ActionType::RestoreEditorCamera4:
    case ActionType::RestoreEditorCamera5:
    case ActionType::RestoreEditorCamera6:
    case ActionType::RestoreEditorCamera7:
    case ActionType::RestoreEditorCamera8:
    case ActionType::RestoreEditorCamera9:
    {
      const plInt32 iCamIdx = (int)m_Type - (int)ActionType::RestoreEditorCamera0;

      m_pSceneDocument->RestoreFavoriteCamera(iCamIdx);
      m_pSceneDocument->ShowDocumentStatus(plFmt("Restored favorite camera position {0}", iCamIdx));

      return;
    }

    case ActionType::JumpToCamera0:
    case ActionType::JumpToCamera1:
    case ActionType::JumpToCamera2:
    case ActionType::JumpToCamera3:
    case ActionType::JumpToCamera4:
    case ActionType::JumpToCamera5:
    case ActionType::JumpToCamera6:
    case ActionType::JumpToCamera7:
    case ActionType::JumpToCamera8:
    case ActionType::JumpToCamera9:
    {
      const plInt32 iCamIdx = (int)m_Type - (int)ActionType::JumpToCamera0;

      const bool bImmediate = value.IsA<bool>() ? value.Get<bool>() : false;
      if (m_pSceneDocument->JumpToLevelCamera(iCamIdx, bImmediate).Failed())
      {
        m_pSceneDocument->ShowDocumentStatus(plFmt("No Camera Component found with shortcut set to '{0}'", iCamIdx));
      }
      return;
    }

    case ActionType::CreateLevelCamera0:
    case ActionType::CreateLevelCamera1:
    case ActionType::CreateLevelCamera2:
    case ActionType::CreateLevelCamera3:
    case ActionType::CreateLevelCamera4:
    case ActionType::CreateLevelCamera5:
    case ActionType::CreateLevelCamera6:
    case ActionType::CreateLevelCamera7:
    case ActionType::CreateLevelCamera8:
    case ActionType::CreateLevelCamera9:
    {
      const plInt32 iCamIdx = (int)m_Type - (int)ActionType::CreateLevelCamera0;

      if (auto pView = plQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;
          pView == nullptr || pView->m_pViewConfig->m_Perspective != plSceneViewPerspective::Perspective)
      {
        m_pSceneDocument->ShowDocumentStatus("Note: Level cameras cannot be created in orthographic views.");
        return;
      }

      if (m_pSceneDocument->CreateLevelCamera(iCamIdx).Succeeded())
      {
        m_pSceneDocument->ShowDocumentStatus(plFmt("Create level camera with shortcut set to '{0}'", iCamIdx));
      }
      else
      {
        m_pSceneDocument->ShowDocumentStatus(plFmt("Could not create level camera '{}'.", iCamIdx));
      }
      return;
    }
  }
}

void plSceneAction::LaunchPlayer(const char* szPlayerApp)
{
  plStringBuilder sCmd;
  QStringList arguments = GetPlayerCommandLine(sCmd);

  plLog::Info("Running: {} {}", szPlayerApp, sCmd);
  m_pSceneDocument->ShowDocumentStatus(plFmt("Running: {} {}", szPlayerApp, sCmd));

  QProcess proc;
  proc.startDetached(QString::fromUtf8(szPlayerApp), arguments);
}

QStringList plSceneAction::GetPlayerCommandLine(plStringBuilder& out_sSingleLine) const
{
  QStringList arguments;
  arguments << "-project";
  arguments << plToolsProject::GetSingleton()->GetProjectDirectory().GetData();

  {
    arguments << "-scene";

    plStringBuilder sAssetDataDir = plAssetCurator::GetSingleton()->FindDataDirectoryForAsset(m_pSceneDocument->GetDocumentPath());

    plStringBuilder sRelativePath = m_pSceneDocument->GetAssetDocumentManager()->GetAbsoluteOutputFileName(m_pSceneDocument->GetAssetDocumentTypeDescriptor(), m_pSceneDocument->GetDocumentPath(), "");

    sRelativePath.MakeRelativeTo(sAssetDataDir).AssertSuccess();
    sRelativePath.MakeCleanPath();

    arguments << sRelativePath.GetData();
  }

  plStringBuilder sWndCfgPath = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sWndCfgPath.AppendPath("RuntimeConfigs/Window.ddl");

#if PLASMA_ENABLED(PLASMA_MIGRATE_RUNTIMECONFIGS)
  plStringBuilder sWndCfgPathOld = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sWndCfgPathOld.AppendPath("Window.ddl");
  sWndCfgPath = plFileSystem::MigrateFileLocation(sWndCfgPathOld, sWndCfgPath);
#endif

  if (plOSFile::ExistsFile(sWndCfgPath))
  {
    arguments << "-wnd";
    arguments << QString::fromUtf8(sWndCfgPath, sWndCfgPath.GetElementCount());
  }

  arguments << "-profile";
  arguments << plString(plAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName()).GetData();

  for (QString s : arguments)
  {
    if (s.contains(" "))
      out_sSingleLine.AppendFormat(" \"{}\"", s.toUtf8().data());
    else
      out_sSingleLine.AppendFormat(" {}", s.toUtf8().data());
  }

  return arguments;
}

void plSceneAction::SceneEventHandler(const plGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case plGameObjectEvent::Type::GameModeChanged:
      UpdateState();
      break;

    default:
      break;
  }
}

void plSceneAction::UpdateState()
{
  if (m_Type == ActionType::StartGameModeSimulate || m_Type == ActionType::StartGameModePlay || m_Type == ActionType::ExportAndRunScene)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() == GameMode::Off);
  }

  if (m_Type == ActionType::StopGameMode)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off);
  }

  if (m_Type == ActionType::KeepSimulationChanges)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off && !m_pSceneDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}
