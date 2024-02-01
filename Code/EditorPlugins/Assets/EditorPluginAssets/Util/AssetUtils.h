#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

struct plMaterialResourceSlot
{
  plString m_sLabel;
  plString m_sResource;
  bool m_bHighlight = false;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plMaterialResourceSlot);
