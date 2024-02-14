#include <AmplitudeAudioPlugin/AmplitudeAudioPluginPCH.h>

#include <AmplitudeAudioPlugin/AmplitudeAudioPluginDLL.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>

PL_STATICLINK_LIBRARY(AmplitudeAudioPlugin)
{
  if (bReturn)
    return;

  PL_STATICLINK_REFERENCE(AmplitudeAudioPlugin_AmplitudeAudioSingleton);
  PL_STATICLINK_REFERENCE(AmplitudeAudioPlugin_AmplitudeAudioPluginStartup);
  PL_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Core_AudioMiddlewareControlsManager);

  PL_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Components_AudioControlsComponent);
  PL_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Components_AmplitudeComponent);
  PL_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Components_AmplitudeListenerComponent);
  PL_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Components_AmplitudeTriggerComponent);

  PL_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Resources_AudioControlCollectionResource);
}
