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
    plActionMapManager::RegisterActionMap("AnimationGraphAssetMenuBar").AssertSuccess();

    plStandardMenus::MapActions("AnimationGraphAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("AnimationGraphAssetMenuBar");
    plDocumentActions::MapMenuActions("AnimationGraphAssetMenuBar");
    plAssetActions::MapMenuActions("AnimationGraphAssetMenuBar");
    plCommandHistoryActions::MapActions("AnimationGraphAssetMenuBar");
    plEditActions::MapActions("AnimationGraphAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("AnimationGraphAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("AnimationGraphAssetToolBar");
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
    plActionMapManager::RegisterActionMap("TextureAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("TextureAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("TextureAssetMenuBar");
    plDocumentActions::MapMenuActions("TextureAssetMenuBar");
    plAssetActions::MapMenuActions("TextureAssetMenuBar");
    plCommandHistoryActions::MapActions("TextureAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("TextureAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("TextureAssetToolBar");
    plCommandHistoryActions::MapActions("TextureAssetToolBar", "");
    plAssetActions::MapToolBarActions("TextureAssetToolBar", true);
    plTextureAssetActions::MapToolbarActions("TextureAssetToolBar");
  }
}

static void ConfigureTextureCubeAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plTextureCubeAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("TextureCubeAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("TextureCubeAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("TextureCubeAssetMenuBar");
    plDocumentActions::MapMenuActions("TextureCubeAssetMenuBar");
    plAssetActions::MapMenuActions("TextureCubeAssetMenuBar");
    plCommandHistoryActions::MapActions("TextureCubeAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("TextureCubeAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("TextureCubeAssetToolBar");
    plCommandHistoryActions::MapActions("TextureCubeAssetToolBar", "");
    plAssetActions::MapToolBarActions("TextureCubeAssetToolBar", true);
    plTextureAssetActions::MapToolbarActions("TextureCubeAssetToolBar");
  }
}

static void ConfigureLUTAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plLUTAssetProperties::PropertyMetaStateEventHandler);

  plLUTAssetActions::RegisterActions();

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("LUTAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("LUTAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("LUTAssetMenuBar");
    plDocumentActions::MapMenuActions("LUTAssetMenuBar");
    plAssetActions::MapMenuActions("LUTAssetMenuBar");
    plCommandHistoryActions::MapActions("LUTAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("LUTAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("LUTAssetToolBar");
    plCommandHistoryActions::MapActions("LUTAssetToolBar", "");
    plAssetActions::MapToolBarActions("LUTAssetToolBar", true);
  }
}

static void ConfigureMaterialAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plMaterialAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("MaterialAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("MaterialAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("MaterialAssetMenuBar");
    plDocumentActions::MapMenuActions("MaterialAssetMenuBar");
    plDocumentActions::MapToolsActions("MaterialAssetMenuBar");
    plAssetActions::MapMenuActions("MaterialAssetMenuBar");
    plCommandHistoryActions::MapActions("MaterialAssetMenuBar");
    plEditActions::MapActions("MaterialAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("MaterialAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("MaterialAssetToolBar");
    plCommandHistoryActions::MapActions("MaterialAssetToolBar", "");
    plAssetActions::MapToolBarActions("MaterialAssetToolBar", true);

    plMaterialAssetActions::RegisterActions();
    plMaterialAssetActions::MapToolbarActions("MaterialAssetToolBar");

    plVisualShaderActions::RegisterActions();
    plVisualShaderActions::MapActions("MaterialAssetToolBar");
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("MaterialAssetViewToolBar").AssertSuccess();
    plViewActions::MapToolbarActions("MaterialAssetViewToolBar", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapToolbarActions("MaterialAssetViewToolBar");
  }
}

static void ConfigureRenderPipelineAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("RenderPipelineAssetMenuBar").AssertSuccess();

    plStandardMenus::MapActions("RenderPipelineAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("RenderPipelineAssetMenuBar");
    plDocumentActions::MapMenuActions("RenderPipelineAssetMenuBar");
    plAssetActions::MapMenuActions("RenderPipelineAssetMenuBar");
    plCommandHistoryActions::MapActions("RenderPipelineAssetMenuBar");
    plEditActions::MapActions("RenderPipelineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("RenderPipelineAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("RenderPipelineAssetToolBar");
    plCommandHistoryActions::MapActions("RenderPipelineAssetToolBar", "");
    plAssetActions::MapToolBarActions("RenderPipelineAssetToolBar", true);
  }
}

static void ConfigureMeshAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("MeshAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("MeshAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("MeshAssetMenuBar");
    plDocumentActions::MapMenuActions("MeshAssetMenuBar");
    plAssetActions::MapMenuActions("MeshAssetMenuBar");
    plCommandHistoryActions::MapActions("MeshAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("MeshAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("MeshAssetToolBar");
    plCommandHistoryActions::MapActions("MeshAssetToolBar", "");
    plAssetActions::MapToolBarActions("MeshAssetToolBar", true);
    plCommonAssetActions::MapToolbarActions("MeshAssetToolBar", plCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("MeshAssetViewToolBar").AssertSuccess();
    plViewActions::MapToolbarActions("MeshAssetViewToolBar", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapToolbarActions("MeshAssetViewToolBar");
  }
}

static void ConfigureSurfaceAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("SurfaceAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("SurfaceAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("SurfaceAssetMenuBar");
    plDocumentActions::MapMenuActions("SurfaceAssetMenuBar");
    plAssetActions::MapMenuActions("SurfaceAssetMenuBar");
    plDocumentActions::MapToolsActions("SurfaceAssetMenuBar");
    plCommandHistoryActions::MapActions("SurfaceAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("SurfaceAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("SurfaceAssetToolBar");
    plCommandHistoryActions::MapActions("SurfaceAssetToolBar", "");
    plAssetActions::MapToolBarActions("SurfaceAssetToolBar", true);
  }
}

static void ConfigureCollectionAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("CollectionAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("CollectionAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("CollectionAssetMenuBar");
    plDocumentActions::MapMenuActions("CollectionAssetMenuBar");
    plAssetActions::MapMenuActions("CollectionAssetMenuBar");
    plDocumentActions::MapToolsActions("CollectionAssetMenuBar");
    plCommandHistoryActions::MapActions("CollectionAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("CollectionAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("CollectionAssetToolBar");
    plCommandHistoryActions::MapActions("CollectionAssetToolBar", "");
    plAssetActions::MapToolBarActions("CollectionAssetToolBar", true);
  }
}

static void ConfigureColorGradientAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("ColorGradientAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("ColorGradientAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("ColorGradientAssetMenuBar");
    plDocumentActions::MapMenuActions("ColorGradientAssetMenuBar");
    plAssetActions::MapMenuActions("ColorGradientAssetMenuBar");
    plDocumentActions::MapToolsActions("ColorGradientAssetMenuBar");
    plCommandHistoryActions::MapActions("ColorGradientAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("ColorGradientAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("ColorGradientAssetToolBar");
    plCommandHistoryActions::MapActions("ColorGradientAssetToolBar", "");
    plAssetActions::MapToolBarActions("ColorGradientAssetToolBar", true);
  }
}

static void ConfigureCurve1DAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("Curve1DAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("Curve1DAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("Curve1DAssetMenuBar");
    plDocumentActions::MapMenuActions("Curve1DAssetMenuBar");
    plAssetActions::MapMenuActions("Curve1DAssetMenuBar");
    plDocumentActions::MapToolsActions("Curve1DAssetMenuBar");
    plCommandHistoryActions::MapActions("Curve1DAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("Curve1DAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("Curve1DAssetToolBar");
    plCommandHistoryActions::MapActions("Curve1DAssetToolBar", "");
    plAssetActions::MapToolBarActions("Curve1DAssetToolBar", true);
  }
}

static void ConfigurePropertyAnimAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("PropertyAnimAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("PropertyAnimAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit | plStandardMenuTypes::Scene | plStandardMenuTypes::View);
    plProjectActions::MapActions("PropertyAnimAssetMenuBar");
    plDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar");
    plAssetActions::MapMenuActions("PropertyAnimAssetMenuBar");
    plDocumentActions::MapToolsActions("PropertyAnimAssetMenuBar");
    plCommandHistoryActions::MapActions("PropertyAnimAssetMenuBar");
    plGameObjectSelectionActions::MapActions("PropertyAnimAssetMenuBar");
    plGameObjectDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar");
    plGameObjectDocumentActions::MapMenuSimulationSpeed("PropertyAnimAssetMenuBar");
    plTransformGizmoActions::MapMenuActions("PropertyAnimAssetMenuBar");
    plTranslateGizmoAction::MapActions("PropertyAnimAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("PropertyAnimAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar");
    plCommandHistoryActions::MapActions("PropertyAnimAssetToolBar", "");
    plAssetActions::MapToolBarActions("PropertyAnimAssetToolBar", true);
    plGameObjectContextActions::MapToolbarActions("PropertyAnimAssetToolBar");
    plGameObjectDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar");
    plTransformGizmoActions::MapToolbarActions("PropertyAnimAssetToolBar");
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("PropertyAnimAssetViewToolBar").AssertSuccess();
    plViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar", plViewActions::PerspectiveMode | plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plQuadViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar");
  }

  // SceneGraph Context Menu
  {
    plActionMapManager::RegisterActionMap("PropertyAnimAsset_ScenegraphContextMenu").AssertSuccess();
    plGameObjectSelectionActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
    plGameObjectContextActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
  }
}

static void ConfigureDecalAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plDecalAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("DecalAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("DecalAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("DecalAssetMenuBar");
    plDocumentActions::MapMenuActions("DecalAssetMenuBar");
    plAssetActions::MapMenuActions("DecalAssetMenuBar");
    plCommandHistoryActions::MapActions("DecalAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("DecalAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("DecalAssetToolBar");
    plCommandHistoryActions::MapActions("DecalAssetToolBar", "");
    plAssetActions::MapToolBarActions("DecalAssetToolBar", true);
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("DecalAssetViewToolBar").AssertSuccess();
    plViewActions::MapToolbarActions("DecalAssetViewToolBar", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapToolbarActions("DecalAssetViewToolBar");
  }
}

static void ConfigureAnimationClipAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plAnimationClipAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("AnimationClipAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("AnimationClipAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("AnimationClipAssetMenuBar");
    plDocumentActions::MapMenuActions("AnimationClipAssetMenuBar");
    plAssetActions::MapMenuActions("AnimationClipAssetMenuBar");
    plCommandHistoryActions::MapActions("AnimationClipAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("AnimationClipAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("AnimationClipAssetToolBar");
    plCommandHistoryActions::MapActions("AnimationClipAssetToolBar", "");
    plAssetActions::MapToolBarActions("AnimationClipAssetToolBar", true);
    plCommonAssetActions::MapToolbarActions("AnimationClipAssetToolBar", plCommonAssetUiState::Loop | plCommonAssetUiState::Pause | plCommonAssetUiState::Restart | plCommonAssetUiState::SimulationSpeed | plCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("AnimationClipAssetViewToolBar").AssertSuccess();
    plViewActions::MapToolbarActions("AnimationClipAssetViewToolBar", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapToolbarActions("AnimationClipAssetViewToolBar");
  }
}

static void ConfigureSkeletonAsset()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plSkeletonAssetDocument::PropertyMetaStateEventHandler);

  plSkeletonActions::RegisterActions();

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("SkeletonAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("SkeletonAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("SkeletonAssetMenuBar");
    plDocumentActions::MapMenuActions("SkeletonAssetMenuBar");
    plAssetActions::MapMenuActions("SkeletonAssetMenuBar");
    plCommandHistoryActions::MapActions("SkeletonAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("SkeletonAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("SkeletonAssetToolBar");
    plCommandHistoryActions::MapActions("SkeletonAssetToolBar", "");
    plAssetActions::MapToolBarActions("SkeletonAssetToolBar", true);
    plCommonAssetActions::MapToolbarActions("SkeletonAssetToolBar", plCommonAssetUiState::Grid);
    plSkeletonActions::MapActions("SkeletonAssetToolBar");
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("SkeletonAssetViewToolBar").AssertSuccess();
    plViewActions::MapToolbarActions("SkeletonAssetViewToolBar", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapToolbarActions("SkeletonAssetViewToolBar");
  }
}

static void ConfigureAnimatedMeshAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("AnimatedMeshAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("AnimatedMeshAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("AnimatedMeshAssetMenuBar");
    plDocumentActions::MapMenuActions("AnimatedMeshAssetMenuBar");
    plAssetActions::MapMenuActions("AnimatedMeshAssetMenuBar");
    plCommandHistoryActions::MapActions("AnimatedMeshAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("AnimatedMeshAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("AnimatedMeshAssetToolBar");
    plCommandHistoryActions::MapActions("AnimatedMeshAssetToolBar", "");
    plAssetActions::MapToolBarActions("AnimatedMeshAssetToolBar", true);
    plCommonAssetActions::MapToolbarActions("AnimatedMeshAssetToolBar", plCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("AnimatedMeshAssetViewToolBar").AssertSuccess();
    plViewActions::MapToolbarActions("AnimatedMeshAssetViewToolBar", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapToolbarActions("AnimatedMeshAssetViewToolBar");
  }
}

static void ConfigureImageDataAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("ImageDataAssetMenuBar").AssertSuccess();
    plStandardMenus::MapActions("ImageDataAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("ImageDataAssetMenuBar");
    plDocumentActions::MapMenuActions("ImageDataAssetMenuBar");
    plAssetActions::MapMenuActions("ImageDataAssetMenuBar");
    plCommandHistoryActions::MapActions("ImageDataAssetMenuBar");
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("ImageDataAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("ImageDataAssetToolBar");
    plCommandHistoryActions::MapActions("ImageDataAssetToolBar", "");
    plAssetActions::MapToolBarActions("ImageDataAssetToolBar", true);
  }

  // View Tool Bar
  {
    plActionMapManager::RegisterActionMap("ImageDataAssetViewToolBar").AssertSuccess();
    plViewActions::MapToolbarActions("ImageDataAssetViewToolBar", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
    plViewLightActions::MapToolbarActions("ImageDataAssetViewToolBar");
  }
}

static void ConfigureStateMachineAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("StateMachineAssetMenuBar").AssertSuccess();

    plStandardMenus::MapActions("StateMachineAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("StateMachineAssetMenuBar");
    plDocumentActions::MapMenuActions("StateMachineAssetMenuBar");
    plAssetActions::MapMenuActions("StateMachineAssetMenuBar");
    plCommandHistoryActions::MapActions("StateMachineAssetMenuBar");
    plEditActions::MapActions("StateMachineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("StateMachineAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("StateMachineAssetToolBar");
    plCommandHistoryActions::MapActions("StateMachineAssetToolBar", "");
    plAssetActions::MapToolBarActions("StateMachineAssetToolBar", true);
  }
}
static void ConfigureBlackboardTemplateAsset()
{
  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("BlackboardTemplateAssetMenuBar").AssertSuccess();

    plStandardMenus::MapActions("BlackboardTemplateAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("BlackboardTemplateAssetMenuBar");
    plDocumentActions::MapMenuActions("BlackboardTemplateAssetMenuBar");
    plAssetActions::MapMenuActions("BlackboardTemplateAssetMenuBar");
    plCommandHistoryActions::MapActions("BlackboardTemplateAssetMenuBar");
    plEditActions::MapActions("BlackboardTemplateAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("BlackboardTemplateAssetToolBar").AssertSuccess();
    plDocumentActions::MapToolbarActions("BlackboardTemplateAssetToolBar");
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
