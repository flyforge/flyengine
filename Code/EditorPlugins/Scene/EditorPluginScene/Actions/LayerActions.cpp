#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QInputDialog>


// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLayerAction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plActionDescriptorHandle plLayerActions::s_hLayerCategory;
plActionDescriptorHandle plLayerActions::s_hCreateLayer;
plActionDescriptorHandle plLayerActions::s_hDeleteLayer;
plActionDescriptorHandle plLayerActions::s_hSaveLayer;
plActionDescriptorHandle plLayerActions::s_hSaveActiveLayer;
plActionDescriptorHandle plLayerActions::s_hLayerLoaded;
plActionDescriptorHandle plLayerActions::s_hLayerVisible;

void plLayerActions::RegisterActions()
{
  s_hLayerCategory = PL_REGISTER_CATEGORY("LayerCategory");

  s_hCreateLayer = PL_REGISTER_ACTION_1("Layer.CreateLayer", plActionScope::Document, "Scene - Layer", "",
    plLayerAction, plLayerAction::ActionType::CreateLayer);
  s_hDeleteLayer = PL_REGISTER_ACTION_1("Layer.DeleteLayer", plActionScope::Document, "Scene - Layer", "",
    plLayerAction, plLayerAction::ActionType::DeleteLayer);
  s_hSaveLayer = PL_REGISTER_ACTION_1("Layer.SaveLayer", plActionScope::Document, "Scene - Layer", "",
    plLayerAction, plLayerAction::ActionType::SaveLayer);
  s_hSaveActiveLayer = PL_REGISTER_ACTION_1("Layer.SaveActiveLayer", plActionScope::Document, "Scene - Layer", "Ctrl+S",
    plLayerAction, plLayerAction::ActionType::SaveActiveLayer);
  s_hLayerLoaded = PL_REGISTER_ACTION_1("Layer.LayerLoaded", plActionScope::Document, "Scene - Layer", "",
    plLayerAction, plLayerAction::ActionType::LayerLoaded);
  s_hLayerVisible = PL_REGISTER_ACTION_1("Layer.LayerVisible", plActionScope::Document, "Scene - Layer", "",
    plLayerAction, plLayerAction::ActionType::LayerVisible);
}

void plLayerActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hLayerCategory);
  plActionManager::UnregisterAction(s_hCreateLayer);
  plActionManager::UnregisterAction(s_hDeleteLayer);
  plActionManager::UnregisterAction(s_hSaveLayer);
  plActionManager::UnregisterAction(s_hSaveActiveLayer);
  plActionManager::UnregisterAction(s_hLayerLoaded);
  plActionManager::UnregisterAction(s_hLayerVisible);
}

void plLayerActions::MapContextMenuActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);


  pMap->MapAction(s_hLayerCategory, "", 0.0f);

  const plStringView sSubPath = "LayerCategory";
  pMap->MapAction(s_hCreateLayer, sSubPath, 1.0f);
  pMap->MapAction(s_hDeleteLayer, sSubPath, 2.0f);
  pMap->MapAction(s_hSaveLayer, sSubPath, 3.0f);
  pMap->MapAction(s_hLayerLoaded, sSubPath, 4.0f);
  pMap->MapAction(s_hLayerVisible, sSubPath, 5.0f);
}

plLayerAction::plLayerAction(const plActionContext& context, const char* szName, plLayerAction::ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<plScene2Document*>(static_cast<const plScene2Document*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::CreateLayer:
      SetIconPath(":/GuiFoundation/Icons/Add.svg");
      break;
    case ActionType::DeleteLayer:
      SetIconPath(":/GuiFoundation/Icons/Delete.svg");
      break;
    case ActionType::SaveLayer:
    case ActionType::SaveActiveLayer:
      SetIconPath(":/GuiFoundation/Icons/Save.svg");
      break;
    case ActionType::LayerLoaded:
      SetCheckable(true);
      break;
    case ActionType::LayerVisible:
      SetCheckable(true);
      break;
  }

  UpdateEnableState();
  m_pSceneDocument->m_LayerEvents.AddEventHandler(plMakeDelegate(&plLayerAction::LayerEventHandler, this));
  if (m_Type == ActionType::SaveActiveLayer)
  {
    m_pSceneDocument->s_EventsAny.AddEventHandler(plMakeDelegate(&plLayerAction::DocumentEventHandler, this));
  }
}


plLayerAction::~plLayerAction()
{
  m_pSceneDocument->m_LayerEvents.RemoveEventHandler(plMakeDelegate(&plLayerAction::LayerEventHandler, this));
  if (m_Type == ActionType::SaveActiveLayer)
  {
    m_pSceneDocument->s_EventsAny.RemoveEventHandler(plMakeDelegate(&plLayerAction::DocumentEventHandler, this));
  }
}

void plLayerAction::ToggleLayerLoaded(plScene2Document* pSceneDocument, plUuid layerGuid)
{
  bool bLoad = !pSceneDocument->IsLayerLoaded(layerGuid);
  if (!bLoad)
  {
    plSceneDocument* pLayer = pSceneDocument->GetLayerDocument(layerGuid);
    if (pLayer && pLayer->IsModified())
    {
      plStringBuilder sMsg;
      plStringBuilder sLayerName = "<Unknown>";
      {
        const plAssetCurator::plLockedSubAsset subAsset = plAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
        if (subAsset.isValid())
        {
          sLayerName = subAsset->GetName();
        }
      }
      sMsg.SetFormat("The layer '{}' has been modified.\nSave before unloading?", sLayerName);
      QMessageBox::StandardButton res = plQtUiServices::MessageBoxQuestion(sMsg, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::No);
      switch (res)
      {
        case QMessageBox::Yes:
        {
          plStatus saveRes = pLayer->SaveDocument();
          if (saveRes.Failed())
          {
            saveRes.LogFailure();
            return;
          }
        }
        break;
        case QMessageBox::Cancel:
          return;
        case QMessageBox::Default:
          break;
        default:
          break;
      }
    }
  }

  pSceneDocument->SetLayerLoaded(layerGuid, bLoad).LogFailure();
  pSceneDocument->SetActiveLayer(layerGuid).LogFailure();
}

void plLayerAction::Execute(const plVariant& value)
{
  switch (m_Type)
  {
    case ActionType::CreateLayer:
    {
      plUuid layerGuid;
      QString name = QInputDialog::getText(GetContext().m_pWindow, "Add Layer", "Layer Name:");
      name = name.trimmed();
      if (name.isEmpty())
        return;
      plStatus res = m_pSceneDocument->CreateLayer(name.toUtf8().data(), layerGuid);
      res.LogFailure();
      return;
    }
    case ActionType::DeleteLayer:
    {
      plUuid layerGuid = GetCurrentSelectedLayer();
      m_pSceneDocument->DeleteLayer(layerGuid).LogFailure();
      return;
    }
    case ActionType::SaveLayer:
    {
      plUuid layerGuid = GetCurrentSelectedLayer();
      if (plSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
      {
        pLayer->SaveDocument().LogFailure();
      }
      return;
    }
    case ActionType::SaveActiveLayer:
    {
      plUuid layerGuid = m_pSceneDocument->GetActiveLayer();
      if (plSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
      {
        pLayer->SaveDocument().LogFailure();
      }
      return;
    }
    case ActionType::LayerLoaded:
    {
      plUuid layerGuid = GetCurrentSelectedLayer();
      ToggleLayerLoaded(m_pSceneDocument, layerGuid);
      return;
    }
    case ActionType::LayerVisible:
    {
      plUuid layerGuid = GetCurrentSelectedLayer();
      bool bVisible = !m_pSceneDocument->IsLayerVisible(layerGuid);
      m_pSceneDocument->SetLayerVisible(layerGuid, bVisible).LogFailure();
      return;
    }
  }
}

void plLayerAction::LayerEventHandler(const plScene2LayerEvent& e)
{
  UpdateEnableState();
}

void plLayerAction::DocumentEventHandler(const plDocumentEvent& e)
{
  UpdateEnableState();
}

void plLayerAction::UpdateEnableState()
{
  plUuid layerGuid = GetCurrentSelectedLayer();

  switch (m_Type)
  {
    case ActionType::CreateLayer:
      return;
    case ActionType::DeleteLayer:
    {
      SetEnabled(layerGuid.IsValid() && layerGuid != m_pSceneDocument->GetGuid());
      return;
    }
    case ActionType::SaveLayer:
    {
      plSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid);
      SetEnabled(pLayer && pLayer->IsModified());
      return;
    }
    case ActionType::SaveActiveLayer:
    {
      plSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(m_pSceneDocument->GetActiveLayer());
      SetEnabled(pLayer && pLayer->IsModified());
      return;
    }
    case ActionType::LayerLoaded:
    {
      SetEnabled(layerGuid.IsValid() && layerGuid != m_pSceneDocument->GetGuid());
      SetChecked(m_pSceneDocument->IsLayerLoaded(layerGuid));
      return;
    }
    case ActionType::LayerVisible:
    {
      SetEnabled(layerGuid.IsValid());
      SetChecked(m_pSceneDocument->IsLayerVisible(layerGuid));
      return;
    }
  }
}

plUuid plLayerAction::GetCurrentSelectedLayer() const
{
  plSelectionManager* pSelection = m_pSceneDocument->GetLayerSelectionManager();
  plUuid layerGuid;
  if (const plDocumentObject* pObject = pSelection->GetCurrentObject())
  {
    plObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
    if (pObject->GetType()->IsDerivedFrom(plGetStaticRTTI<plSceneLayer>()))
    {
      layerGuid = pAccessor->Get<plUuid>(pObject, "Layer");
    }
  }
  return layerGuid;
}
