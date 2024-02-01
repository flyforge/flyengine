#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/Scene2DocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Visualizers/BoxReflectionProbeVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Utils/CoreRenderProfile.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

void OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plScene2Document>())
      {
        new plQtScene2DocumentWindow(static_cast<plScene2Document*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
      else if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plSceneDocument>())
      {
        new plQtSceneDocumentWindow(static_cast<plSceneDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ToolsProjectEventHandler(const plEditorAppEvent& e)
{
  if (e.m_Type == plEditorAppEvent::Type::BeforeApplyDataDirectories)
  {
    // plQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">sdk/Data/Base", "base");
  }
}

void AssetCuratorEventHandler(const plAssetCuratorEvent& e)
{
  if (e.m_Type == plAssetCuratorEvent::Type::ActivePlatformChanged)
  {
    plSet<plString> allCamPipes;

    auto& dynEnum = plDynamicStringEnum::CreateDynamicEnum("CameraPipelines");

    for (plUInt32 profileIdx = 0; profileIdx < plAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++profileIdx)
    {
      const plPlatformProfile* pProfile = plAssetCurator::GetSingleton()->GetAssetProfile(profileIdx);

      const plRenderPipelineProfileConfig* pConfig = pProfile->GetTypeConfig<plRenderPipelineProfileConfig>();

      for (auto it = pConfig->m_CameraPipelines.GetIterator(); it.IsValid(); ++it)
      {
        dynEnum.AddValidValue(it.Key(), true);
      }
    }
  }
}

void plCameraComponent_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);
void plSkyLightComponent_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);
void plGreyBoxComponent_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);
void plLightComponent_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);
void plSceneDocument_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

QImage SliderImageGenerator_LightTemperature(plUInt32 uiWidth, plUInt32 uiHeight, double fMinValue, double fMaxValue)
{
  // can use a 1D image, height doesn't need to be all used
  QImage image = QImage(uiWidth, 1, QImage::Format::Format_RGB32);

  for (int x = 0; x < uiWidth; ++x)
  {
    const double pos = (double)x / (uiWidth - 1.0);
    plColor c = plColor::MakeFromKelvin(static_cast<plUInt32>((pos * (fMaxValue - fMinValue)) + fMinValue));

    plColorGammaUB cg = c;
    image.setPixel(x, 0, qRgb(cg.r, cg.g, cg.b));
  }

  return image;
}

void OnLoadPlugin()
{
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plSceneDocument_PropertyMetaStateEventHandler);

  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(OnDocumentManagerEvent));

  plQtEditorApp::GetSingleton()->m_Events.AddEventHandler(ToolsProjectEventHandler);

  plAssetCurator::GetSingleton()->m_Events.AddEventHandler(AssetCuratorEventHandler);

  // Add built in tags
  {
    plToolsTagRegistry::AddTag(plToolsTag("Default", "Exclude From Export", true));
    plToolsTagRegistry::AddTag(plToolsTag("Default", "CastShadow", true));
    plToolsTagRegistry::AddTag(plToolsTag("Default", "SkyLight", true));
  }

  plSelectionActions::RegisterActions();
  plSceneGizmoActions::RegisterActions();
  plSceneActions::RegisterActions();
  plLayerActions::RegisterActions();

  // misc
  plQtImageSliderWidget::s_ImageGenerators["LightTemperature"] = SliderImageGenerator_LightTemperature;

  // Menu Bar
  const char* MenuBars[] = {"EditorPluginScene_DocumentMenuBar", "EditorPluginScene_Scene2MenuBar"};
  for (const char* szMenuBar : MenuBars)
  {
    plActionMapManager::RegisterActionMap(szMenuBar).AssertSuccess();
    plStandardMenus::MapActions(szMenuBar, plStandardMenuTypes::Default | plStandardMenuTypes::Edit | plStandardMenuTypes::Scene | plStandardMenuTypes::View);
    plProjectActions::MapActions(szMenuBar);
    plDocumentActions::MapMenuActions(szMenuBar);
    plAssetActions::MapMenuActions(szMenuBar);
    plDocumentActions::MapToolsActions(szMenuBar);
    plCommandHistoryActions::MapActions(szMenuBar);
    plTransformGizmoActions::MapMenuActions(szMenuBar);
    plSceneGizmoActions::MapMenuActions(szMenuBar);
    plGameObjectSelectionActions::MapActions(szMenuBar);
    plSelectionActions::MapActions(szMenuBar);
    plEditActions::MapActions(szMenuBar, true, true);
    plTranslateGizmoAction::MapActions(szMenuBar);
    plGameObjectDocumentActions::MapMenuActions(szMenuBar);
    plGameObjectDocumentActions::MapMenuSimulationSpeed(szMenuBar);
    plSceneActions::MapMenuActions(szMenuBar);
  }
  // Scene2 Menu bar adjustments
  {
    plActionMap* pMap = plActionMapManager::GetActionMap(MenuBars[1]);
    pMap->UnmapAction(plDocumentActions::s_hSave, "G.File.Common").AssertSuccess();
    pMap->MapAction(plLayerActions::s_hSaveActiveLayer, "G.File.Common", 6.5f);
  }


  // Tool Bar
  const char* ToolBars[] = {"EditorPluginScene_DocumentToolBar", "EditorPluginScene_Scene2ToolBar"};
  for (const char* szToolBar : ToolBars)
  {
    plActionMapManager::RegisterActionMap(szToolBar).AssertSuccess();
    plDocumentActions::MapToolbarActions(szToolBar);
    plCommandHistoryActions::MapActions(szToolBar, "");
    plTransformGizmoActions::MapToolbarActions(szToolBar);
    plSceneGizmoActions::MapToolbarActions(szToolBar);
    plGameObjectDocumentActions::MapToolbarActions(szToolBar);
    plSceneActions::MapToolbarActions(szToolBar);
  }
  // Scene2 Tool bar adjustments
  {
    plActionMap* pMap = plActionMapManager::GetActionMap(ToolBars[1]);
    pMap->UnmapAction(plDocumentActions::s_hSave, "SaveCategory").AssertSuccess();
    pMap->MapAction(plLayerActions::s_hSaveActiveLayer, "SaveCategory", 1.0f);
  }

  // View Tool Bar
  plActionMapManager::RegisterActionMap("EditorPluginScene_ViewToolBar").AssertSuccess();
  plViewActions::MapToolbarActions("EditorPluginScene_ViewToolBar", plViewActions::PerspectiveMode | plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
  plQuadViewActions::MapToolbarActions("EditorPluginScene_ViewToolBar");

  // Visualizers
  plVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plPointLightVisualizerAttribute>(), [](const plRTTI* pRtti) -> plVisualizerAdapter*
    { return PL_DEFAULT_NEW(plPointLightVisualizerAdapter); });
  plVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plSpotLightVisualizerAttribute>(), [](const plRTTI* pRtti) -> plVisualizerAdapter*
    { return PL_DEFAULT_NEW(plSpotLightVisualizerAdapter); });
  plVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plBoxReflectionProbeVisualizerAttribute>(), [](const plRTTI* pRtti) -> plVisualizerAdapter*
    { return PL_DEFAULT_NEW(plBoxReflectionProbeVisualizerAdapter); });

  // SceneGraph Context Menu
  plActionMapManager::RegisterActionMap("EditorPluginScene_ScenegraphContextMenu").AssertSuccess();
  plGameObjectSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");
  plSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");
  plEditActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");

  // Layer Context Menu
  plActionMapManager::RegisterActionMap("EditorPluginScene_LayerContextMenu").AssertSuccess();
  plLayerActions::MapContextMenuActions("EditorPluginScene_LayerContextMenu");

  // component property meta states
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plCameraComponent_PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plSkyLightComponent_PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plGreyBoxComponent_PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plLightComponent_PropertyMetaStateEventHandler);
}

void OnUnloadPlugin()
{
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plSceneDocument_PropertyMetaStateEventHandler);

  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(OnDocumentManagerEvent));
  plQtEditorApp::GetSingleton()->m_Events.RemoveEventHandler(ToolsProjectEventHandler);
  plAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(AssetCuratorEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plGreyBoxComponent_PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plSkyLightComponent_PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plCameraComponent_PropertyMetaStateEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plLightComponent_PropertyMetaStateEventHandler);


  plSelectionActions::UnregisterActions();
  plSceneGizmoActions::UnregisterActions();
  plLayerActions::UnregisterActions();
  plSceneActions::UnregisterActions();
}

PL_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PL_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void plCameraComponent_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  static const plRTTI* pRtti = plRTTI::FindTypeByName("plCameraComponent");
  PL_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const plInt64 usage = e.m_pObject->GetTypeAccessor().GetValue("UsageHint").ConvertTo<plInt64>();
  const bool isRenderTarget = (usage == 3); // plCameraUsageHint::RenderTarget

  auto& props = *e.m_pPropertyStates;

  props["RenderTarget"].m_Visibility = isRenderTarget ? plPropertyUiState::Default : plPropertyUiState::Disabled;
  props["RenderTargetOffset"].m_Visibility = isRenderTarget ? plPropertyUiState::Default : plPropertyUiState::Invisible;
  props["RenderTargetSize"].m_Visibility = isRenderTarget ? plPropertyUiState::Default : plPropertyUiState::Invisible;
}

void plSkyLightComponent_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  static const plRTTI* pRtti = plRTTI::FindTypeByName("plSkyLightComponent");
  PL_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const plInt64 iReflectionProbeMode = e.m_pObject->GetTypeAccessor().GetValue("ReflectionProbeMode").ConvertTo<plInt64>();
  const bool bIsStatic = (iReflectionProbeMode == 0); // plReflectionProbeMode::Static

  auto& props = *e.m_pPropertyStates;

  props["CubeMap"].m_Visibility = bIsStatic ? plPropertyUiState::Default : plPropertyUiState::Invisible;
  // props["RenderTargetOffset"].m_Visibility = isRenderTarget ? plPropertyUiState::Default : plPropertyUiState::Invisible;
  // props["RenderTargetSize"].m_Visibility = isRenderTarget ? plPropertyUiState::Default : plPropertyUiState::Invisible;
}

void plGreyBoxComponent_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  static const plRTTI* pRtti = plRTTI::FindTypeByName("plGreyBoxComponent");
  PL_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  auto& props = *e.m_pPropertyStates;

  const plInt64 iShapeType = e.m_pObject->GetTypeAccessor().GetValue("Shape").ConvertTo<plInt64>();

  props["Detail"].m_Visibility = plPropertyUiState::Invisible;
  props["Detail"].m_sNewLabelText = "Detail";
  props["Curvature"].m_Visibility = plPropertyUiState::Invisible;
  props["Thickness"].m_Visibility = plPropertyUiState::Invisible;
  props["SlopedTop"].m_Visibility = plPropertyUiState::Invisible;
  props["SlopedBottom"].m_Visibility = plPropertyUiState::Invisible;

  switch (iShapeType)
  {
    case plGreyBoxShape::Box:
      break;
    case plGreyBoxShape::RampX:
    case plGreyBoxShape::RampY:
      break;
    case plGreyBoxShape::Column:
      props["Detail"].m_Visibility = plPropertyUiState::Default;
      break;
    case plGreyBoxShape::StairsX:
    case plGreyBoxShape::StairsY:
      props["Detail"].m_Visibility = plPropertyUiState::Default;
      props["Curvature"].m_Visibility = plPropertyUiState::Default;
      props["SlopedTop"].m_Visibility = plPropertyUiState::Default;
      props["Detail"].m_sNewLabelText = "Steps";
      break;
    case plGreyBoxShape::ArchX:
    case plGreyBoxShape::ArchY:
      props["Detail"].m_Visibility = plPropertyUiState::Default;
      props["Curvature"].m_Visibility = plPropertyUiState::Default;
      props["Thickness"].m_Visibility = plPropertyUiState::Default;
      break;
    case plGreyBoxShape::SpiralStairs:
      props["Detail"].m_Visibility = plPropertyUiState::Default;
      props["Curvature"].m_Visibility = plPropertyUiState::Default;
      props["Thickness"].m_Visibility = plPropertyUiState::Default;
      props["SlopedTop"].m_Visibility = plPropertyUiState::Default;
      props["SlopedBottom"].m_Visibility = plPropertyUiState::Default;
      props["Detail"].m_sNewLabelText = "Steps";
      break;
  }
}

void plLightComponent_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  static const plRTTI* pRtti = plRTTI::FindTypeByName("plLightComponent");
  PL_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (!e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom(pRtti))
    return;

  auto& props = *e.m_pPropertyStates;

  const bool bUseColorTemperature = e.m_pObject->GetTypeAccessor().GetValue("UseColorTemperature").ConvertTo<bool>();
  props["Temperature"].m_Visibility = bUseColorTemperature ? plPropertyUiState::Default : plPropertyUiState::Invisible;
  props["LightColor"].m_Visibility = bUseColorTemperature ? plPropertyUiState::Invisible : plPropertyUiState::Default;

  const bool bCastShadows = e.m_pObject->GetTypeAccessor().GetValue("CastShadows").ConvertTo<bool>();
  props["PenumbraSize"].m_Visibility = bCastShadows ? plPropertyUiState::Default : plPropertyUiState::Invisible;
  props["SlopeBias"].m_Visibility = bCastShadows ? plPropertyUiState::Default : plPropertyUiState::Invisible;
  props["ConstantBias"].m_Visibility = bCastShadows ? plPropertyUiState::Default : plPropertyUiState::Invisible;
}
