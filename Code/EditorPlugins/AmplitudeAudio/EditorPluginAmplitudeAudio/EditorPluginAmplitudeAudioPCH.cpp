#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioPCH.h>

PLASMA_STATICLINK_LIBRARY(EditorPluginAmplitudeAudio)
{
  if (bReturn)
    return;

  
  PLASMA_STATICLINK_REFERENCE(EditorPluginAmplitudeAudio_Assets_AmplitudeAudioControlCollectionAsset);
  PLASMA_STATICLINK_REFERENCE(EditorPluginAmplitudeAudio_Assets_AmplitudeAudioControlCollectionAssetManager);
  PLASMA_STATICLINK_REFERENCE(EditorPluginAmplitudeAudio_Assets_AmplitudeAudioControlCollectionAssetWindow);

  PLASMA_STATICLINK_REFERENCE(EditorPluginAmplitudeAudio_AmplitudeAudioControlsManager);
}