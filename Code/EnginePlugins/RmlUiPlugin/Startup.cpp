#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

static plRmlUiResourceLoader s_RmlUiResourceLoader;

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(RmlUi, RmlUiPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {    
  }

  ON_CORESYSTEMS_SHUTDOWN
  {    
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    plResourceManager::SetResourceTypeLoader<plRmlUiResource>(&s_RmlUiResourceLoader);

    plResourceManager::RegisterResourceForAssetType("RmlUi", plGetStaticRTTI<plRmlUiResource>());

    {
      plRmlUiResourceDescriptor desc;
      plRmlUiResourceHandle hResource = plResourceManager::CreateResource<plRmlUiResource>("RmlUiMissing", std::move(desc), "Fallback for missing rml ui resource");
      plResourceManager::SetResourceTypeMissingFallback<plRmlUiResource>(hResource);
    }

    if (plRmlUi::GetSingleton() == nullptr)
    {
      PL_DEFAULT_NEW(plRmlUi);
    }
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (plRmlUi* pRmlUi = plRmlUi::GetSingleton())
    {
      PL_DEFAULT_DELETE(pRmlUi);
    }

    plResourceManager::SetResourceTypeLoader<plRmlUiResource>(nullptr);

    plRmlUiResource::CleanupDynamicPluginReferences();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on
