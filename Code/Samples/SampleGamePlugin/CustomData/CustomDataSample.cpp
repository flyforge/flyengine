#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/CustomData/CustomDataSample.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(CustomDataSample, 1, plRTTIDefaultAllocator<CustomDataSample>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Text", m_sText),
    PLASMA_MEMBER_PROPERTY("Size", m_iSize)->AddAttributes(new plDefaultValueAttribute(42), new plClampValueAttribute(16, 64)),
    PLASMA_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new plDefaultValueAttribute(plColor::CornflowerBlue)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PLASMA_DEFINE_CUSTOM_DATA_RESOURCE(CustomDataSample);