#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ActionManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plActionManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plActionManager::Shutdown();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plEvent<const plActionManager::Event&> plActionManager::s_Events;
plIdTable<plActionId, plActionDescriptor*> plActionManager::s_ActionTable;
plMap<plString, plActionManager::CategoryData> plActionManager::s_CategoryPathToActions;
plMap<plString, plString> plActionManager::s_ShortcutOverride;

////////////////////////////////////////////////////////////////////////
// plActionManager public functions
////////////////////////////////////////////////////////////////////////

plActionDescriptorHandle plActionManager::RegisterAction(const plActionDescriptor& desc)
{
  plActionDescriptorHandle hType = GetActionHandle(desc.m_sCategoryPath, desc.m_sActionName);
  PLASMA_ASSERT_DEV(hType.IsInvalidated(), "The action '{0}' in category '{1}' was already registered!", desc.m_sActionName, desc.m_sCategoryPath);

  plActionDescriptor* pDesc = CreateActionDesc(desc);

  // apply shortcut override
  {
    auto ovride = s_ShortcutOverride.Find(desc.m_sActionName);
    if (ovride.IsValid())
      pDesc->m_sShortcut = ovride.Value();
  }

  hType = plActionDescriptorHandle(s_ActionTable.Insert(pDesc));
  pDesc->m_Handle = hType;

  auto it = s_CategoryPathToActions.FindOrAdd(pDesc->m_sCategoryPath);
  it.Value().m_Actions.Insert(hType);
  it.Value().m_ActionNameToHandle[pDesc->m_sActionName.GetData()] = hType;

  {
    Event msg;
    msg.m_Type = Event::Type::ActionAdded;
    msg.m_pDesc = pDesc;
    msg.m_Handle = hType;
    s_Events.Broadcast(msg);
  }
  return hType;
}

bool plActionManager::UnregisterAction(plActionDescriptorHandle& ref_hAction)
{
  plActionDescriptor* pDesc = nullptr;
  if (!s_ActionTable.TryGetValue(ref_hAction, pDesc))
  {
    ref_hAction.Invalidate();
    return false;
  }

  auto it = s_CategoryPathToActions.Find(pDesc->m_sCategoryPath);
  PLASMA_ASSERT_DEV(it.IsValid(), "Action is present but not mapped in its category path!");
  PLASMA_VERIFY(it.Value().m_Actions.Remove(ref_hAction), "Action is present but not in its category data!");
  PLASMA_VERIFY(it.Value().m_ActionNameToHandle.Remove(pDesc->m_sActionName), "Action is present but its name is not in the map!");
  if (it.Value().m_Actions.IsEmpty())
  {
    s_CategoryPathToActions.Remove(it);
  }

  s_ActionTable.Remove(ref_hAction);
  DeleteActionDesc(pDesc);
  ref_hAction.Invalidate();
  return true;
}

const plActionDescriptor* plActionManager::GetActionDescriptor(plActionDescriptorHandle hAction)
{
  plActionDescriptor* pDesc = nullptr;
  if (s_ActionTable.TryGetValue(hAction, pDesc))
    return pDesc;

  return nullptr;
}

const plIdTable<plActionId, plActionDescriptor*>::ConstIterator plActionManager::GetActionIterator()
{
  return s_ActionTable.GetIterator();
}

plActionDescriptorHandle plActionManager::GetActionHandle(const char* szCategoryPath, const char* szActionName)
{
  plActionDescriptorHandle hAction;
  auto it = s_CategoryPathToActions.Find(szCategoryPath);
  if (!it.IsValid())
    return hAction;

  it.Value().m_ActionNameToHandle.TryGetValue(szActionName, hAction);

  return hAction;
}

plString plActionManager::FindActionCategory(const char* szActionName)
{
  for (auto itCat : s_CategoryPathToActions)
  {
    if (itCat.Value().m_ActionNameToHandle.Contains(szActionName))
      return itCat.Key();
  }

  return plString();
}

plResult plActionManager::ExecuteAction(const char* szCategory, const char* szActionName, const plActionContext& context, const plVariant& value /*= plVariant()*/)
{
  plString sCategory = szCategory;

  if (szCategory == nullptr)
  {
    sCategory = FindActionCategory(szActionName);
  }

  auto hAction = plActionManager::GetActionHandle(sCategory, szActionName);

  if (hAction.IsInvalidated())
    return PLASMA_FAILURE;

  const plActionDescriptor* pDesc = plActionManager::GetActionDescriptor(hAction);

  if (pDesc == nullptr)
    return PLASMA_FAILURE;

  plAction* pAction = pDesc->CreateAction(context);

  if (pAction == nullptr)
    return PLASMA_FAILURE;

  pAction->Execute(value);
  pDesc->DeleteAction(pAction);

  return PLASMA_SUCCESS;
}

void plActionManager::SaveShortcutAssignment()
{
  plStringBuilder sFile = plApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
  sFile.AppendPath("Settings/Shortcuts.ddl");

  PLASMA_LOG_BLOCK("LoadShortcutAssignment", sFile.GetData());

  plDeferredFileWriter file;
  file.SetOutput(sFile);

  plOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(plOpenDdlWriter::TypeStringMode::Compliant);

  plStringBuilder sKey;

  for (auto it = GetActionIterator(); it.IsValid(); ++it)
  {
    auto pAction = it.Value();

    if (pAction->m_Type != plActionType::Action)
      continue;

    if (pAction->m_sShortcut == pAction->m_sDefaultShortcut)
      sKey.Set("default: ", pAction->m_sShortcut);
    else
      sKey = pAction->m_sShortcut;

    writer.BeginPrimitiveList(plOpenDdlPrimitiveType::String, pAction->m_sActionName);
    writer.WriteString(sKey);
    writer.EndPrimitiveList();
  }

  if (file.Close().Failed())
  {
    plLog::Error("Failed to write shortcuts config file '{0}'", sFile);
  }
}

void plActionManager::LoadShortcutAssignment()
{
  plStringBuilder sFile = plApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
  sFile.AppendPath("Settings/Shortcuts.ddl");

  PLASMA_LOG_BLOCK("LoadShortcutAssignment", sFile.GetData());

  plFileReader file;
  if (file.Open(sFile).Failed())
  {
    plLog::Dev("No shortcuts file '{0}' was found", sFile);
    return;
  }

  plOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, plLog::GetThreadLocalLogSystem()).Failed())
    return;

  const auto obj = reader.GetRootElement();

  plStringBuilder sKey, sValue;

  for (auto pElement = obj->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (!pElement->HasName() || !pElement->HasPrimitives(plOpenDdlPrimitiveType::String))
      continue;

    sKey = pElement->GetName();
    sValue = pElement->GetPrimitivesString()[0];

    if (sValue.FindSubString_NoCase("default") != nullptr)
      continue;

    s_ShortcutOverride[sKey] = sValue;
  }

  // apply overrides
  for (auto it = GetActionIterator(); it.IsValid(); ++it)
  {
    auto pAction = it.Value();

    if (pAction->m_Type != plActionType::Action)
      continue;

    auto ovride = s_ShortcutOverride.Find(pAction->m_sActionName);
    if (ovride.IsValid())
      pAction->m_sShortcut = ovride.Value();
  }
}

////////////////////////////////////////////////////////////////////////
// plActionManager private functions
////////////////////////////////////////////////////////////////////////

void plActionManager::Startup()
{
  plDocumentActions::RegisterActions();
  plStandardMenus::RegisterActions();
  plCommandHistoryActions::RegisterActions();
  plEditActions::RegisterActions();
}

void plActionManager::Shutdown()
{
  plDocumentActions::UnregisterActions();
  plStandardMenus::UnregisterActions();
  plCommandHistoryActions::UnregisterActions();
  plEditActions::UnregisterActions();

  PLASMA_ASSERT_DEV(s_ActionTable.IsEmpty(), "Some actions were registered but not unregistred.");
  PLASMA_ASSERT_DEV(s_CategoryPathToActions.IsEmpty(), "Some actions were registered but not unregistred.");

  s_ActionTable.Clear();
  s_CategoryPathToActions.Clear();
  s_ShortcutOverride.Clear();
}

plActionDescriptor* plActionManager::CreateActionDesc(const plActionDescriptor& desc)
{
  plActionDescriptor* pDesc = PLASMA_DEFAULT_NEW(plActionDescriptor);
  *pDesc = desc;
  return pDesc;
}

void plActionManager::DeleteActionDesc(plActionDescriptor* pDesc)
{
  PLASMA_DEFAULT_DELETE(pDesc);
}
