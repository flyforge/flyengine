#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

struct plMaterialResourceSlot
{
  plString m_sLabel;
  plString m_sResource;
  bool m_bHighlight = false;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plMaterialResourceSlot);
