#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioPCH.h>

PL_STATICLINK_LIBRARY(EditorPluginAmplitudeAudio)
{
  if (bReturn)
    return;

  
  PL_STATICLINK_REFERENCE(EditorPluginAmplitudeAudio_Assets_AmplitudeAudioControlCollectionAsset);
  PL_STATICLINK_REFERENCE(EditorPluginAmplitudeAudio_Assets_AmplitudeAudioControlCollectionAssetManager);
  PL_STATICLINK_REFERENCE(EditorPluginAmplitudeAudio_Assets_AmplitudeAudioControlCollectionAssetWindow);

  PL_STATICLINK_REFERENCE(EditorPluginAmplitudeAudio_AmplitudeAudioControlsManager);
}
