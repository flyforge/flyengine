#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/Dialogs/ShaderTemplateDlg.moc.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonActions.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ConfigureAnimationGraphAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("AnimationGraphAssetMenuBar").IgnoreResult();

    plStandardMenus::MapActions("AnimationGraphAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("AnimationGraphAssetMenuBar");
    plDocumentActions::MapActions("AnimationGraphAssetMenuBar", "Menu.File", false);
    plAssetActions::MapMenuActions("AnimationGraphAssetMenuBar", "Menu.File");
    plCommandHistoryActions::MapActions("AnimationGraphAssetMenuBar", "Menu.Edit");
    plEditActions::MapActions("AnimationGraphAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("AnimationGraphAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("AnimationGraphAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("AnimationGraphAssetToolBar", "");
    plAssetActions::MapToolBarActions("AnimationGraphAssetToolBar", true);
  }
}

static void ConfigureTexture2DAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plTextureAssetProperties::PropertyMetaStateEventHandler);

  plTextureAssetActions::RegisterActions();

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("TextureAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("TextureAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("TextureAssetMenuBar");
    plDocumentActions::MapActions("TextureAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("TextureAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("TextureAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("TextureAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("TextureAssetToolBar", "");
    plAssetActions::MapToolBarActions("TextureAssetToolBar", true);
    plTextureAssetActions::MapActions("TextureAssetToolBar", "");
  }
}

static void ConfigureTextureCubeAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plTextureCubeAssetProperties::PropertyMetaStateEventHandler);

  plTextureCubeAssetActions::RegisterActions();

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("TextureCubeAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("TextureCubeAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("TextureCubeAssetMenuBar");
    plDocumentActions::MapActions("TextureCubeAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("TextureCubeAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("TextureCubeAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("TextureCubeAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("TextureCubeAssetToolBar", "");
    plAssetActions::MapToolBarActions("TextureCubeAssetToolBar", true);
    plTextureAssetActions::MapActions("TextureCubeAssetToolBar", "");
  }
}

static void ConfigureLUTAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plLUTAssetProperties::PropertyMetaStateEventHandler);

  plLUTAssetActions::RegisterActions();

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("LUTAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("LUTAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("LUTAssetMenuBar");
    plDocumentActions::MapActions("LUTAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("LUTAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("LUTAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("LUTAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("LUTAssetToolBar", "");
    plAssetActions::MapToolBarActions("LUTAssetToolBar", true);
  }
}

static void ConfigureMaterialAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plMaterialAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("MaterialAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("MaterialAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("MaterialAssetMenuBar");
    plDocumentActions::MapActions("MaterialAssetMenuBar", "Menu.File", false);
    plDocumentActions::MapToolsActions("MaterialAssetMenuBar", "Menu.Tools");
    plCommandHistoryActions::MapActions("MaterialAssetMenuBar", "Menu.Edit");
    plEditActions::MapActions("MaterialAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("MaterialAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("MaterialAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("MaterialAssetToolBar", "");
    plAssetActions::MapToolBarActions("MaterialAssetToolBar", true);

    plMaterialAssetActions::RegisterActions();
    plMaterialAssetActions::MapActions("MaterialAssetToolBar", "");

    plVisualShaderActions::RegisterActions();
    plVisualShaderActions::MapActions("MaterialAssetToolBar");
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("MaterialAssetViewToolBar").IgnoreResult();
    plViewActions::MapActions("MaterialAssetViewToolBar", "", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapActions("MaterialAssetViewToolBar", "");
  }
}

static void ConfigureRenderPipelineAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("RenderPipelineAssetMenuBar").IgnoreResult();

    plStandardMenus::MapActions("RenderPipelineAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("RenderPipelineAssetMenuBar");
    plDocumentActions::MapActions("RenderPipelineAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("RenderPipelineAssetMenuBar", "Menu.Edit");
    plEditActions::MapActions("RenderPipelineAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("RenderPipelineAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("RenderPipelineAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("RenderPipelineAssetToolBar", "");
    plAssetActions::MapToolBarActions("RenderPipelineAssetToolBar", true);
  }
}

static void ConfigureMeshAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("MeshAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("MeshAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("MeshAssetMenuBar");
    plDocumentActions::MapActions("MeshAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("MeshAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("MeshAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("MeshAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("MeshAssetToolBar", "");
    plAssetActions::MapToolBarActions("MeshAssetToolBar", true);
    plCommonAssetActions::MapActions("MeshAssetToolBar", "", plCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("MeshAssetViewToolBar").IgnoreResult();
    plViewActions::MapActions("MeshAssetViewToolBar", "", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapActions("MeshAssetViewToolBar", "");
  }
}

static void ConfigureSurfaceAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("SurfaceAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("SurfaceAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("SurfaceAssetMenuBar");
    plDocumentActions::MapActions("SurfaceAssetMenuBar", "Menu.File", false);
    plDocumentActions::MapToolsActions("SurfaceAssetMenuBar", "Menu.Tools");
    plCommandHistoryActions::MapActions("SurfaceAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("SurfaceAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("SurfaceAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("SurfaceAssetToolBar", "");
    plAssetActions::MapToolBarActions("SurfaceAssetToolBar", true);
  }
}

static void ConfigureCollectionAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("CollectionAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("CollectionAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("CollectionAssetMenuBar");
    plDocumentActions::MapActions("CollectionAssetMenuBar", "Menu.File", false);
    plDocumentActions::MapToolsActions("CollectionAssetMenuBar", "Menu.Tools");
    plCommandHistoryActions::MapActions("CollectionAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("CollectionAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("CollectionAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("CollectionAssetToolBar", "");
    plAssetActions::MapToolBarActions("CollectionAssetToolBar", true);
  }
}

static void ConfigureColorGradientAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("ColorGradientAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("ColorGradientAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("ColorGradientAssetMenuBar");
    plDocumentActions::MapActions("ColorGradientAssetMenuBar", "Menu.File", false);
    plDocumentActions::MapToolsActions("ColorGradientAssetMenuBar", "Menu.Tools");
    plCommandHistoryActions::MapActions("ColorGradientAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("ColorGradientAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("ColorGradientAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("ColorGradientAssetToolBar", "");
    plAssetActions::MapToolBarActions("ColorGradientAssetToolBar", true);
  }
}

static void ConfigureCurve1DAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("Curve1DAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("Curve1DAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("Curve1DAssetMenuBar");
    plDocumentActions::MapActions("Curve1DAssetMenuBar", "Menu.File", false);
    plDocumentActions::MapToolsActions("Curve1DAssetMenuBar", "Menu.Tools");
    plCommandHistoryActions::MapActions("Curve1DAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("Curve1DAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("Curve1DAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("Curve1DAssetToolBar", "");
    plAssetActions::MapToolBarActions("Curve1DAssetToolBar", true);
  }
}

static void ConfigurePropertyAnimAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("PropertyAnimAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("PropertyAnimAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Scene | plStandardMenuTypes::View | plStandardMenuTypes::Help);
    plProjectActions::MapActions("PropertyAnimAssetMenuBar");
    plDocumentActions::MapActions("PropertyAnimAssetMenuBar", "Menu.File", false);
    plDocumentActions::MapToolsActions("PropertyAnimAssetMenuBar", "Menu.Tools");
    plCommandHistoryActions::MapActions("PropertyAnimAssetMenuBar", "Menu.Edit");
    plGameObjectSelectionActions::MapActions("PropertyAnimAssetMenuBar", "Menu.Edit");
    plGameObjectDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar", "Menu.View");
    plGameObjectDocumentActions::MapMenuSimulationSpeed("PropertyAnimAssetMenuBar", "Menu.Scene");
    plTransformGizmoActions::MapMenuActions("PropertyAnimAssetMenuBar", "Menu.Edit");
    plTranslateGizmoAction::MapActions("PropertyAnimAssetMenuBar", "Menu.Edit/Gizmo.Menu");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("PropertyAnimAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("PropertyAnimAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("PropertyAnimAssetToolBar", "");
    plAssetActions::MapToolBarActions("PropertyAnimAssetToolBar", true);
    plGameObjectContextActions::MapToolbarActions("PropertyAnimAssetToolBar", "");
    plGameObjectDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar", "");
    plTransformGizmoActions::MapToolbarActions("PropertyAnimAssetToolBar", "");
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("PropertyAnimAssetViewToolBar").IgnoreResult();
    plViewActions::MapActions("PropertyAnimAssetViewToolBar", "", plViewActions::PerspectiveMode | plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plQuadViewActions::MapActions("PropertyAnimAssetViewToolBar", "");
  }

  // SceneGraph Context Menu
  {
    plActionMapManager::RegisterActionMap("PropertyAnimAsset_ScenegraphContextMenu").IgnoreResult();
    plGameObjectSelectionActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu", "");
    plGameObjectContextActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu", "");
  }
}

static void ConfigureDecalAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plDecalAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("DecalAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("DecalAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("DecalAssetMenuBar");
    plDocumentActions::MapActions("DecalAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("DecalAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("DecalAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("DecalAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("DecalAssetToolBar", "");
    plAssetActions::MapToolBarActions("DecalAssetToolBar", true);
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("DecalAssetViewToolBar").IgnoreResult();
    plViewActions::MapActions("DecalAssetViewToolBar", "", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapActions("DecalAssetViewToolBar", "");
  }
}

static void ConfigureAnimationClipAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plAnimationClipAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("AnimationClipAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("AnimationClipAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("AnimationClipAssetMenuBar");
    plDocumentActions::MapActions("AnimationClipAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("AnimationClipAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("AnimationClipAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("AnimationClipAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("AnimationClipAssetToolBar", "");
    plAssetActions::MapToolBarActions("AnimationClipAssetToolBar", true);
    plCommonAssetActions::MapActions("AnimationClipAssetToolBar", "", plCommonAssetUiState::Loop | plCommonAssetUiState::Pause | plCommonAssetUiState::Restart | plCommonAssetUiState::SimulationSpeed | plCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("AnimationClipAssetViewToolBar").IgnoreResult();
    plViewActions::MapActions("AnimationClipAssetViewToolBar", "", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapActions("AnimationClipAssetViewToolBar", "");
  }
}

static void ConfigureSkeletonAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plSkeletonAssetDocument::PropertyMetaStateEventHandler);

  plSkeletonActions::RegisterActions();

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("SkeletonAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("SkeletonAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("SkeletonAssetMenuBar");
    plDocumentActions::MapActions("SkeletonAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("SkeletonAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("SkeletonAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("SkeletonAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("SkeletonAssetToolBar", "");
    plAssetActions::MapToolBarActions("SkeletonAssetToolBar", true);
    plCommonAssetActions::MapActions("SkeletonAssetToolBar", "", plCommonAssetUiState::Grid);
    plSkeletonActions::MapActions("SkeletonAssetToolBar", "");
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("SkeletonAssetViewToolBar").IgnoreResult();
    plViewActions::MapActions("SkeletonAssetViewToolBar", "", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapActions("SkeletonAssetViewToolBar", "");
  }
}

static void ConfigureAnimatedMeshAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("AnimatedMeshAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("AnimatedMeshAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("AnimatedMeshAssetMenuBar");
    plDocumentActions::MapActions("AnimatedMeshAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("AnimatedMeshAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("AnimatedMeshAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("AnimatedMeshAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("AnimatedMeshAssetToolBar", "");
    plAssetActions::MapToolBarActions("AnimatedMeshAssetToolBar", true);
    plCommonAssetActions::MapActions("AnimatedMeshAssetToolBar", "", plCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("AnimatedMeshAssetViewToolBar").IgnoreResult();
    plViewActions::MapActions("AnimatedMeshAssetViewToolBar", "", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapActions("AnimatedMeshAssetViewToolBar", "");
  }
}

static void ConfigureImageDataAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("ImageDataAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("ImageDataAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("ImageDataAssetMenuBar");
    plDocumentActions::MapActions("ImageDataAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("ImageDataAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("ImageDataAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("ImageDataAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("ImageDataAssetToolBar", "");
    plAssetActions::MapToolBarActions("ImageDataAssetToolBar", true);
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("ImageDataAssetViewToolBar").IgnoreResult();
    plViewActions::MapActions("ImageDataAssetViewToolBar", "", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapActions("ImageDataAssetViewToolBar", "");
  }
}

static void ConfigureStateMachineAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("StateMachineAssetMenuBar").IgnoreResult();

    plStandardMenus::MapActions("StateMachineAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("StateMachineAssetMenuBar");
    plDocumentActions::MapActions("StateMachineAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("StateMachineAssetMenuBar", "Menu.Edit");
    plEditActions::MapActions("StateMachineAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("StateMachineAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("StateMachineAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("StateMachineAssetToolBar", "");
    plAssetActions::MapToolBarActions("StateMachineAssetToolBar", true);
  }
}

static void ConfigureBlackboardTemplateAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("BlackboardTemplateAssetMenuBar").IgnoreResult();

    plStandardMenus::MapActions("BlackboardTemplateAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("BlackboardTemplateAssetMenuBar");
    plDocumentActions::MapActions("BlackboardTemplateAssetMenuBar", "Menu.File", false);
    plAssetActions::MapMenuActions("BlackboardTemplateAssetMenuBar", "Menu.File");
    plCommandHistoryActions::MapActions("BlackboardTemplateAssetMenuBar", "Menu.Edit");
    plEditActions::MapActions("BlackboardTemplateAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("BlackboardTemplateAssetToolBar").IgnoreResult();
    plDocumentActions::MapActions("BlackboardTemplateAssetToolBar", "", true);
    plCommandHistoryActions::MapActions("BlackboardTemplateAssetToolBar", "");
    plAssetActions::MapToolBarActions("BlackboardTemplateAssetToolBar", true);
  }
}

plVariant CustomAction_CreateShaderFromTemplate(const plDocument* pDoc)
{
  plQtShaderTemplateDlg dlg(nullptr, pDoc);

  if (dlg.exec() == QDialog::Accepted)
  {
    plStringBuilder abs;
    if (plFileSystem::ResolvePath(dlg.m_sResult, &abs, nullptr).Succeeded())
    {
      if (!plQtUiServices::GetSingleton()->OpenFileInDefaultProgram(abs))
      {
        plQtUiServices::GetSingleton()->MessageBoxInformation(plFmt("There is no default program set to open shader files:\n\n{}", abs));
      }
    }

    return dlg.m_sResult;
  }

  return {};
}

void OnLoadPlugin()
{
  ConfigureAnimationGraphAsset();
  ConfigureTexture2DAsset();
  ConfigureTextureCubeAsset();
  ConfigureLUTAsset();
  ConfigureMaterialAsset();
  ConfigureRenderPipelineAsset();
  ConfigureMeshAsset();
  ConfigureSurfaceAsset();
  ConfigureCollectionAsset();
  ConfigureColorGradientAsset();
  ConfigureCurve1DAsset();
  ConfigurePropertyAnimAsset();
  ConfigureDecalAsset();
  ConfigureAnimationClipAsset();
  ConfigureSkeletonAsset();
  ConfigureAnimatedMeshAsset();
  ConfigureImageDataAsset();
  ConfigureStateMachineAsset();
  ConfigureBlackboardTemplateAsset();

  plDocumentManager::s_CustomActions["CustomAction_CreateShaderFromTemplate"] = CustomAction_CreateShaderFromTemplate;
}

void OnUnloadPlugin()
{
  plTextureAssetActions::UnregisterActions();
  plTextureCubeAssetActions::UnregisterActions();
  plLUTAssetActions::UnregisterActions();
  plVisualShaderActions::UnregisterActions();
  plMaterialAssetActions::UnregisterActions();
  plSkeletonActions::UnregisterActions();

  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plMeshAssetProperties::PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plTextureAssetProperties::PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plDecalAssetProperties::PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plTextureCubeAssetProperties::PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plMaterialAssetProperties::PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plSkeletonAssetDocument::PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plAnimationClipAssetProperties::PropertyMetaStateEventHandler);
}

PLASMA_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PLASMA_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
