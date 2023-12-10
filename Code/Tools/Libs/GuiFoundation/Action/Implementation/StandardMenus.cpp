#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

plActionDescriptorHandle plStandardMenus::s_hMenuProject;
plActionDescriptorHandle plStandardMenus::s_hMenuFile;
plActionDescriptorHandle plStandardMenus::s_hMenuEdit;
plActionDescriptorHandle plStandardMenus::s_hMenuPanels;
plActionDescriptorHandle plStandardMenus::s_hMenuScene;
plActionDescriptorHandle plStandardMenus::s_hMenuView;
plActionDescriptorHandle plStandardMenus::s_hMenuTools;
plActionDescriptorHandle plStandardMenus::s_hMenuHelp;
plActionDescriptorHandle plStandardMenus::s_hCheckForUpdates;
plActionDescriptorHandle plStandardMenus::s_hReportProblem;

void plStandardMenus::RegisterActions()
{
  s_hMenuProject = PLASMA_REGISTER_MENU("G.Project");
  s_hMenuFile = PLASMA_REGISTER_MENU("G.File");
  s_hMenuEdit = PLASMA_REGISTER_MENU("G.Edit");
  s_hMenuPanels = PLASMA_REGISTER_DYNAMIC_MENU("G.Panels", plApplicationPanelsMenuAction, "");
  s_hMenuScene = PLASMA_REGISTER_MENU("G.Scene");
  s_hMenuView = PLASMA_REGISTER_MENU("G.View");
  s_hMenuTools = PLASMA_REGISTER_MENU("G.Tools");
  s_hMenuHelp = PLASMA_REGISTER_MENU("G.Help");
  s_hCheckForUpdates = PLASMA_REGISTER_ACTION_1("Help.CheckForUpdates", plActionScope::Global, "Help", "", plHelpActions, plHelpActions::ButtonType::CheckForUpdates);
  s_hReportProblem = PLASMA_REGISTER_ACTION_1("Help.ReportProblem", plActionScope::Global, "Help", "", plHelpActions, plHelpActions::ButtonType::ReportProblem);
}

void plStandardMenus::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hMenuProject);
  plActionManager::UnregisterAction(s_hMenuFile);
  plActionManager::UnregisterAction(s_hMenuEdit);
  plActionManager::UnregisterAction(s_hMenuPanels);
  plActionManager::UnregisterAction(s_hMenuScene);
  plActionManager::UnregisterAction(s_hMenuView);
  plActionManager::UnregisterAction(s_hMenuTools);
  plActionManager::UnregisterAction(s_hMenuHelp);
  plActionManager::UnregisterAction(s_hCheckForUpdates);
  plActionManager::UnregisterAction(s_hReportProblem);
}

void plStandardMenus::MapActions(plStringView sMapping, const plBitflags<plStandardMenuTypes>& menus)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "'{0}' does not exist", sMapping);

  plActionMapDescriptor md;

  if (menus.IsAnySet(plStandardMenuTypes::Project))
    pMap->MapAction(s_hMenuProject, "", -10000.0f);

  if (menus.IsAnySet(plStandardMenuTypes::File))
    pMap->MapAction(s_hMenuFile, "", 1.0f);

  if (menus.IsAnySet(plStandardMenuTypes::Edit))
    pMap->MapAction(s_hMenuEdit, "", 2.0f);

  if (menus.IsAnySet(plStandardMenuTypes::Scene))
    pMap->MapAction(s_hMenuScene, "", 3.0f);

  if (menus.IsAnySet(plStandardMenuTypes::View))
    pMap->MapAction(s_hMenuView, "", 4.0f);

  if (menus.IsAnySet(plStandardMenuTypes::Tools))
    pMap->MapAction(s_hMenuTools, "", 5.0f);

  if (menus.IsAnySet(plStandardMenuTypes::Panels))
    pMap->MapAction(s_hMenuPanels, "", 6.0f);

  if (menus.IsAnySet(plStandardMenuTypes::Help))
  {
    pMap->MapAction(s_hMenuHelp, "", 7.0f);
    pMap->MapAction(s_hReportProblem, "G.Help", 3.0f);
    pMap->MapAction(s_hCheckForUpdates, "G.Help", 10.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// plApplicationPanelsMenuAction
////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plApplicationPanelsMenuAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

struct plComparePanels
{
  /// \brief Returns true if a is less than b
  PLASMA_ALWAYS_INLINE bool Less(const plDynamicMenuAction::Item& p1, const plDynamicMenuAction::Item& p2) const { return p1.m_sDisplay < p2.m_sDisplay; }

  /// \brief Returns true if a is equal to b
  PLASMA_ALWAYS_INLINE bool Equal(const plDynamicMenuAction::Item& p1, const plDynamicMenuAction::Item& p2) const
  {
    return p1.m_sDisplay == p2.m_sDisplay;
  }
};


void plApplicationPanelsMenuAction::GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_entries)
{
  out_entries.Clear();

  for (auto* pPanel : plQtApplicationPanel::GetAllApplicationPanels())
  {
    plDynamicMenuAction::Item item;
    item.m_sDisplay = pPanel->windowTitle().toUtf8().data();
    item.m_UserValue = pPanel;
    item.m_Icon = pPanel->icon();
    item.m_CheckState = pPanel->isClosed() ? plDynamicMenuAction::Item::CheckMark::Unchecked : plDynamicMenuAction::Item::CheckMark::Checked;

    out_entries.PushBack(item);
  }

  // make sure the panels appear in alphabetical order in the menu
  plComparePanels cp;
  out_entries.Sort<plComparePanels>(cp);
}

void plApplicationPanelsMenuAction::Execute(const plVariant& value)
{
  plQtApplicationPanel* pPanel = static_cast<plQtApplicationPanel*>(value.ConvertTo<void*>());
  if (pPanel->isClosed())
  {
    pPanel->toggleView(true);
    pPanel->EnsureVisible();
  }
  else
  {
    pPanel->toggleView(false);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plHelpActions, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plHelpActions::plHelpActions(const plActionContext& context, const char* szName, ButtonType button)
  : plButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  if (button == ButtonType::ReportProblem)
  {
    SetIconPath(":/EditorFramework/Icons/GitHub-Light.svg");
  }
}

plHelpActions::~plHelpActions() = default;

void plHelpActions::Execute(const plVariant& value)
{
  if (m_ButtonType == ButtonType::ReportProblem)
  {
    QDesktopServices::openUrl(QUrl("https://github.com/plEngine/plEngine/issues"));
  }
  if (m_ButtonType == ButtonType::CheckForUpdates)
  {
    plQtUiServices::CheckForUpdates();
  }
}
