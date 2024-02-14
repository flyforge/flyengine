#include <AmplitudeAudioPlugin/AmplitudeAudioPluginPCH.h>

#include <AmplitudeAudioPlugin/Components/AmplitudeComponent.h>

// clang-format off
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plAmplitudeComponent, 1)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Sound/Amplitude"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

PL_STATICLINK_FILE(AmplitudeAudioPlugin, AmplitudeAudioPlugin_Components_AmplitudeComponent);
