#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectContextAction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plGameObjectContextActions::s_hCategory;
plActionDescriptorHandle plGameObjectContextActions::s_hPickContextScene;
plActionDescriptorHandle plGameObjectContextActions::s_hPickContextObject;
plActionDescriptorHandle plGameObjectContextActions::s_hClearContextObject;

void plGameObjectContextActions::RegisterActions()
{
  s_hCategory = PL_REGISTER_CATEGORY("GameObjectContextCategory");
  s_hPickContextScene = PL_REGISTER_ACTION_1("GameObjectContext.PickContextScene", plActionScope::Window, "Game Object Context", "",
    plGameObjectContextAction, plGameObjectContextAction::ActionType::PickContextScene);
  s_hPickContextObject = PL_REGISTER_ACTION_1("GameObjectContext.PickContextObject", plActionScope::Window, "Game Object Context", "",
    plGameObjectContextAction, plGameObjectContextAction::ActionType::PickContextObject);
  s_hClearContextObject = PL_REGISTER_ACTION_1("GameObjectContext.ClearContextObject", plActionScope::Window, "Game Object Context", "",
    plGameObjectContextAction, plGameObjectContextAction::ActionType::ClearContextObject);
}


void plGameObjectContextActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategory);
  plActionManager::UnregisterAction(s_hPickContextScene);
  plActionManager::UnregisterAction(s_hPickContextObject);
  plActionManager::UnregisterAction(s_hClearContextObject);
}

void plGameObjectContextActions::MapToolbarActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 10.0f);

  const plStringView szSubPath = "GameObjectContextCategory";
  pMap->MapAction(s_hPickContextScene, szSubPath, 1.0f);
}


void plGameObjectContextActions::MapContextMenuActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 10.0f);

  const plStringView szSubPath = "GameObjectContextCategory";
  pMap->MapAction(s_hPickContextObject, szSubPath, 1.0f);
  pMap->MapAction(s_hClearContextObject, szSubPath, 2.0f);
}

plGameObjectContextAction::plGameObjectContextAction(const plActionContext& context, const char* szName, plGameObjectContextAction::ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::PickContextScene:
      SetIconPath(":/EditorPluginAssets/PickTarget.svg");
      break;
    case ActionType::PickContextObject:
      SetIconPath(":/EditorPluginAssets/PickTarget.svg");
      break;
    case ActionType::ClearContextObject:
      SetIconPath(":/EditorPluginAssets/PickTarget.svg");
      break;
    default:
      break;
  }

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plGameObjectContextAction::SelectionEventHandler, this));
  Update();
}

plGameObjectContextAction::~plGameObjectContextAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plGameObjectContextAction::SelectionEventHandler, this));
}

void plGameObjectContextAction::Execute(const plVariant& value)
{
  plGameObjectContextDocument* pDocument = static_cast<plGameObjectContextDocument*>(GetContext().m_pDocument);
  plUuid document = pDocument->GetContextDocumentGuid();
  switch (m_Type)
  {
    case ActionType::PickContextScene:
    {
      plQtAssetBrowserDlg dlg(GetContext().m_pWindow, document, "Scene;Prefab");
      if (dlg.exec() == 0)
        return;

      document = dlg.GetSelectedAssetGuid();
      pDocument->SetContext(document, plUuid()).LogFailure();
      return;
    }
    case ActionType::PickContextObject:
    {
      const auto& selection = pDocument->GetSelectionManager()->GetSelection();
      if (selection.GetCount() == 1)
      {
        if (selection[0]->GetType() == plGetStaticRTTI<plGameObject>())
        {
          pDocument->SetContext(document, selection[0]->GetGuid()).LogFailure();
        }
      }
    }
      return;
    case ActionType::ClearContextObject:
    {
      pDocument->SetContext(document, plUuid()).LogFailure();
    }
      return;
  }
}

void plGameObjectContextAction::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  Update();
}

void plGameObjectContextAction::Update()
{
  plGameObjectContextDocument* pDocument = static_cast<plGameObjectContextDocument*>(GetContext().m_pDocument);

  switch (m_Type)
  {
    case ActionType::PickContextObject:
    {
      const auto& selection = pDocument->GetSelectionManager()->GetSelection();
      bool bIsSingleGameObject = selection.GetCount() == 1 && selection[0]->GetType() == plGetStaticRTTI<plGameObject>();
      SetEnabled(bIsSingleGameObject);
    }

    default:
      break;
  }
}
