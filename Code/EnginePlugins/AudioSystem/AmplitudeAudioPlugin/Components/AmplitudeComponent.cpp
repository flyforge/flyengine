#include <AmplitudeAudioPlugin/AmplitudeAudioPluginPCH.h>

#include <AmplitudeAudioPlugin/Components/AmplitudeComponent.h>

// clang-format off
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plAmplitudeComponent, 1)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Sound/Amplitude"),
    new plColorAttribute(plColorScheme::Sound),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

PLASMA_STATICLINK_FILE(AmplitudeAudioPlugin, AmplitudeAudioPlugin_Components_AmplitudeComponent);
