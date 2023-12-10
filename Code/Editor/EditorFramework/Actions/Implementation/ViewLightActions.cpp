#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewLightButtonAction, 1, plRTTINoAllocator);
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewLightSliderAction, 1, plRTTINoAllocator);
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plActionDescriptorHandle plViewLightActions::s_hLightMenu;
plActionDescriptorHandle plViewLightActions::s_hSkyBox;
plActionDescriptorHandle plViewLightActions::s_hSkyLight;
plActionDescriptorHandle plViewLightActions::s_hSkyLightCubeMap;
plActionDescriptorHandle plViewLightActions::s_hSkyLightIntensity;
plActionDescriptorHandle plViewLightActions::s_hDirLight;
plActionDescriptorHandle plViewLightActions::s_hDirLightAngle;
plActionDescriptorHandle plViewLightActions::s_hDirLightShadows;
plActionDescriptorHandle plViewLightActions::s_hDirLightIntensity;
plActionDescriptorHandle plViewLightActions::s_hFog;
plActionDescriptorHandle plViewLightActions::s_hSetAsDefault;

void plViewLightActions::RegisterActions()
{
  s_hLightMenu = PLASMA_REGISTER_MENU_WITH_ICON("View.LightMenu", ":/EditorFramework/Icons/ViewLightMenu.svg");
  s_hSkyBox = PLASMA_REGISTER_ACTION_1(
    "View.SkyBox", plActionScope::Document, "View", "", plViewLightButtonAction, plEngineViewLightSettingsEvent::Type::SkyBoxChanged);
  s_hSkyLight = PLASMA_REGISTER_ACTION_1(
    "View.SkyLight", plActionScope::Document, "View", "", plViewLightButtonAction, plEngineViewLightSettingsEvent::Type::SkyLightChanged);
  s_hSkyLightCubeMap = PLASMA_REGISTER_ACTION_1(
    "View.SkyLightCubeMap", plActionScope::Document, "View", "", plViewLightButtonAction, plEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged);
  s_hSkyLightIntensity = PLASMA_REGISTER_ACTION_1(
    "View.SkyLightIntensity", plActionScope::Document, "View", "", plViewLightSliderAction, plEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged);

  s_hDirLight = PLASMA_REGISTER_ACTION_1(
    "View.DirectionalLight", plActionScope::Document, "View", "", plViewLightButtonAction, plEngineViewLightSettingsEvent::Type::DirectionalLightChanged);
  s_hDirLightAngle = PLASMA_REGISTER_ACTION_1(
    "View.DirLightAngle", plActionScope::Document, "View", "", plViewLightSliderAction, plEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged);
  s_hDirLightShadows = PLASMA_REGISTER_ACTION_1(
    "View.DirectionalLightShadows", plActionScope::Document, "View", "", plViewLightButtonAction, plEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged);
  s_hDirLightIntensity = PLASMA_REGISTER_ACTION_1(
    "View.DirLightIntensity", plActionScope::Document, "View", "", plViewLightSliderAction, plEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged);
  s_hFog = PLASMA_REGISTER_ACTION_1(
    "View.Fog", plActionScope::Document, "View", "", plViewLightButtonAction, plEngineViewLightSettingsEvent::Type::FogChanged);
  s_hSetAsDefault = PLASMA_REGISTER_ACTION_1(
    "View.SetAsDefault", plActionScope::Document, "View", "", plViewLightButtonAction, plEngineViewLightSettingsEvent::Type::DefaultValuesChanged);
}

void plViewLightActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hLightMenu);
  plActionManager::UnregisterAction(s_hSkyBox);
  plActionManager::UnregisterAction(s_hSkyLight);
  plActionManager::UnregisterAction(s_hSkyLightCubeMap);
  plActionManager::UnregisterAction(s_hSkyLightIntensity);
  plActionManager::UnregisterAction(s_hDirLight);
  plActionManager::UnregisterAction(s_hDirLightAngle);
  plActionManager::UnregisterAction(s_hDirLightShadows);
  plActionManager::UnregisterAction(s_hDirLightIntensity);
  plActionManager::UnregisterAction(s_hFog);
  plActionManager::UnregisterAction(s_hSetAsDefault);
}

void plViewLightActions::MapToolbarActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hLightMenu, "", 2.5f);
  const plStringView sSubPath = "View.LightMenu";
  pMap->MapAction(s_hSkyBox, sSubPath, 1.0f);
  pMap->MapAction(s_hSkyLight, sSubPath, 1.0f);
  pMap->MapAction(s_hSkyLightCubeMap, sSubPath, 2.0f);
  pMap->MapAction(s_hSkyLightIntensity, sSubPath, 3.0f);
  pMap->MapAction(s_hDirLight, sSubPath, 4.0f);
  pMap->MapAction(s_hDirLightAngle, sSubPath, 5.0f);
  pMap->MapAction(s_hDirLightShadows, sSubPath, 6.0f);
  pMap->MapAction(s_hDirLightIntensity, sSubPath, 7.0f);
  pMap->MapAction(s_hFog, sSubPath, 8.0f);
  pMap->MapAction(s_hSetAsDefault, sSubPath, 9.0f);
}

//////////////////////////////////////////////////////////////////////////

plViewLightButtonAction::plViewLightButtonAction(const plActionContext& context, const char* szName, plEngineViewLightSettingsEvent::Type button)
  : plButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(m_Context.m_pWindow);
  m_pSettings = static_cast<plEngineViewLightSettings*>(pView->GetDocumentWindow()->GetDocument()->FindSyncObject(plEngineViewLightSettings::GetStaticRTTI()));
  PLASMA_ASSERT_DEV(m_pSettings != nullptr, "The asset document does not have a plEngineViewLightSettings sync object.");
  m_SettingsID = m_pSettings->m_EngineViewLightSettingsEvents.AddEventHandler(plMakeDelegate(&plViewLightButtonAction::LightSettingsEventHandler, this));

  switch (m_ButtonType)
  {
    case plEngineViewLightSettingsEvent::Type::SkyBoxChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/plSkyBoxComponent.svg");
      break;
    case plEngineViewLightSettingsEvent::Type::SkyLightChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/plSkyLightComponent.svg");
      break;
    case plEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
      SetIconPath(":/TypeIcons/plSkyLightComponent.svg");
      break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/plDirectionalLightComponent.svg");
      break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/plDirectionalLightComponent.svg");
      break;
    case plEngineViewLightSettingsEvent::Type::FogChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/plFogComponent.svg");
      break;
    case plEngineViewLightSettingsEvent::Type::DefaultValuesChanged:
      SetCheckable(false);
      SetIconPath(":/EditorFramework/Icons/ViewLightMenu.svg");
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  UpdateAction();
}

plViewLightButtonAction::~plViewLightButtonAction()
{
  m_pSettings->m_EngineViewLightSettingsEvents.RemoveEventHandler(m_SettingsID);
}

void plViewLightButtonAction::Execute(const plVariant& value)
{
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case plEngineViewLightSettingsEvent::Type::SkyBoxChanged:
    {
      m_pSettings->SetSkyBox(value.ConvertTo<bool>());
    }
    break;
    case plEngineViewLightSettingsEvent::Type::SkyLightChanged:
    {
      m_pSettings->SetSkyLight(value.ConvertTo<bool>());
    }
    break;
    case plEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
    {
      plStringBuilder sFile = m_pSettings->GetSkyLightCubeMap();
      plUuid assetGuid = plConversionUtils::ConvertStringToUuid(sFile);

      plQtAssetBrowserDlg dlg(pView, assetGuid, "CompatibleAsset_Texture_Cube");
      if (dlg.exec() == 0)
        return;

      assetGuid = dlg.GetSelectedAssetGuid();
      if (assetGuid.IsValid())
        plConversionUtils::ToString(assetGuid, sFile);

      if (sFile.IsEmpty())
      {
        sFile = dlg.GetSelectedAssetPathRelative();

        if (sFile.IsEmpty())
        {
          sFile = dlg.GetSelectedAssetPathAbsolute();

          plQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile);
        }
      }

      if (sFile.IsEmpty())
        return;

      m_pSettings->SetSkyLightCubeMap(sFile);
    }
    break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
    {
      m_pSettings->SetDirectionalLight(value.ConvertTo<bool>());
    }
    break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
    {
      m_pSettings->SetDirectionalLightShadows(value.ConvertTo<bool>());
    }
    break;
    case plEngineViewLightSettingsEvent::Type::FogChanged:
    {
      m_pSettings->SetFog(value.ConvertTo<bool>());
    }
    break;
    case plEngineViewLightSettingsEvent::Type::DefaultValuesChanged:
    {
      if (plQtUiServices::MessageBoxQuestion("Do you want to make the current light settings the global default?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
      {
        plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();
        pPreferences->SetAsDefaultValues(*m_pSettings);
      }
    }
    break;
    default:
      break;
  }
}

void plViewLightButtonAction::LightSettingsEventHandler(const plEngineViewLightSettingsEvent& e)
{
  if (m_ButtonType == e.m_Type)
  {
    UpdateAction();
  }
}

void plViewLightButtonAction::UpdateAction()
{
  switch (m_ButtonType)
  {
    case plEngineViewLightSettingsEvent::Type::SkyBoxChanged:
    {
      SetChecked(m_pSettings->GetSkyBox());
    }
    break;
    case plEngineViewLightSettingsEvent::Type::SkyLightChanged:
    {
      SetChecked(m_pSettings->GetSkyLight());
    }
    break;
    case plEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
    {
    }
    break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
    {
      SetChecked(m_pSettings->GetDirectionalLight());
    }
    break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
    {
      SetChecked(m_pSettings->GetDirectionalLightShadows());
    }
    break;
    case plEngineViewLightSettingsEvent::Type::FogChanged:
    {
      SetChecked(m_pSettings->GetFog());
    }
    break;
    default:
      break;
  }
}
//////////////////////////////////////////////////////////////////////////

plViewLightSliderAction::plViewLightSliderAction(const plActionContext& context, const char* szName, plEngineViewLightSettingsEvent::Type button)
  : plSliderAction(context, szName)
{
  m_ButtonType = button;
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(m_Context.m_pWindow);
  m_pSettings = static_cast<plEngineViewLightSettings*>(pView->GetDocumentWindow()->GetDocument()->FindSyncObject(plEngineViewLightSettings::GetStaticRTTI()));
  PLASMA_ASSERT_DEV(m_pSettings != nullptr, "The asset document does not have a plEngineViewLightSettings sync object.");
  m_SettingsID = m_pSettings->m_EngineViewLightSettingsEvents.AddEventHandler(plMakeDelegate(&plViewLightSliderAction::LightSettingsEventHandler, this));

  switch (m_ButtonType)
  {
    case plEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
      SetIconPath(":/TypeIcons/plSkyLightComponent.svg");
      SetRange(0, 20);
      break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
      SetIconPath(":/TypeIcons/plDirectionalLightComponent.svg");
      SetRange(-90, 90);
      break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
      SetIconPath(":/TypeIcons/plDirectionalLightComponent.svg");
      SetRange(0, 200);
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  UpdateAction();
}

plViewLightSliderAction::~plViewLightSliderAction()
{
  m_pSettings->m_EngineViewLightSettingsEvents.RemoveEventHandler(m_SettingsID);
}

void plViewLightSliderAction::Execute(const plVariant& value)
{
  switch (m_ButtonType)
  {
    case plEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
    {
      m_pSettings->SetSkyLightIntensity(value.ConvertTo<float>() / 10.0f);
    }
    break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
    {
      m_pSettings->SetDirectionalLightAngle(plAngle::MakeFromDegree(value.ConvertTo<float>()));
    }
    break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
    {
      m_pSettings->SetDirectionalLightIntensity(value.ConvertTo<float>() / 10.0f);
    }
    break;
    default:
      break;
  }
}

void plViewLightSliderAction::LightSettingsEventHandler(const plEngineViewLightSettingsEvent& e)
{
  if (m_ButtonType == e.m_Type)
  {
    UpdateAction();
  }
}

void plViewLightSliderAction::UpdateAction()
{
  switch (m_ButtonType)
  {
    case plEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
    {
      SetValue(plMath::Clamp((plInt32)(m_pSettings->GetSkyLightIntensity() * 10.0f), 0, 20));
    }
    break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
    {
      SetValue(plMath::Clamp((plInt32)(m_pSettings->GetDirectionalLightAngle().GetDegree()), -90, 90));
    }
    break;
    case plEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
    {
      SetValue(plMath::Clamp((plInt32)(m_pSettings->GetDirectionalLightIntensity() * 10.0f), 1, 200));
    }
    break;
    default:
      break;
  }
}
