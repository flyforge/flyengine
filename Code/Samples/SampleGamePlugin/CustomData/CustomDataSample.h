#pragma once

#include <Core/CustomData/CustomData.h>

class CustomDataSample : public plCustomData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(CustomDataSample, plCustomData);

public:

  plString m_sText;
  plInt32 m_iSize = 42;
  plColor m_Color;
};

PLASMA_DECLARE_CUSTOM_DATA_RESOURCE(CustomDataSample);