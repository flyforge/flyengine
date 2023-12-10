#include <AmplitudeAudioPlugin/AmplitudeAudioPluginPCH.h>

#include <AmplitudeAudioPlugin/AmplitudeAudioPluginDLL.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>

PLASMA_STATICLINK_LIBRARY(AmplitudeAudioPlugin)
{
  if (bReturn)
    return;

  PLASMA_STATICLINK_REFERENCE(AmplitudeAudioPlugin_AmplitudeAudioSingleton);
  PLASMA_STATICLINK_REFERENCE(AmplitudeAudioPlugin_AmplitudeAudioPluginStartup);
  PLASMA_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Core_AudioMiddlewareControlsManager);

  PLASMA_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Components_AudioControlsComponent);
  PLASMA_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Components_AmplitudeComponent);
  PLASMA_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Components_AmplitudeListenerComponent);
  PLASMA_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Components_AmplitudeTriggerComponent);

  PLASMA_STATICLINK_REFERENCE(AmplitudeAudioPlugin_Resources_AudioControlCollectionResource);
}
