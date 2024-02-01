#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginTypeScript/Actions/TypeScriptActions.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

void OnLoadPlugin()
{
  plTypeScriptActions::RegisterActions();

  // TypeScript
  {
    // Menu Bar
    {
      plActionMapManager::RegisterActionMap("TypeScriptAssetMenuBar").IgnoreResult();
      plStandardMenus::MapActions("TypeScriptAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
      plProjectActions::MapActions("TypeScriptAssetMenuBar");
      plDocumentActions::MapMenuActions("TypeScriptAssetMenuBar");
      plAssetActions::MapMenuActions("TypeScriptAssetMenuBar");
      plCommandHistoryActions::MapActions("TypeScriptAssetMenuBar");
    }

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("TypeScriptAssetToolBar").IgnoreResult();
      plDocumentActions::MapToolbarActions("TypeScriptAssetToolBar");
      plCommandHistoryActions::MapActions("TypeScriptAssetToolBar", "");
      plAssetActions::MapToolBarActions("TypeScriptAssetToolBar", true);
      plTypeScriptActions::MapActions("TypeScriptAssetToolBar");
    }
  }
}

void OnUnloadPlugin()
{
  plTypeScriptActions::UnregisterActions();
}

PL_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PL_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
