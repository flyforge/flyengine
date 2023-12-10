#pragma once

#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/Variant.h>

class plDocumentObject;

struct PLASMA_GUIFOUNDATION_DLL plPropertySelection
{
  const plDocumentObject* m_pObject;
  plVariant m_Index;

  bool operator==(const plPropertySelection& rhs) const { return m_pObject == rhs.m_pObject && m_Index == rhs.m_Index; }

  bool operator<(const plPropertySelection& rhs) const
  {
    // Qt6 requires the less than operator but never calls it, so we use this dummy for now.
    PLASMA_ASSERT_NOT_IMPLEMENTED;
    return false;
  }
};

struct PLASMA_GUIFOUNDATION_DLL plPropertyClipboard
{
  plString m_Type;
  plVariant m_Value;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GUIFOUNDATION_DLL, plPropertyClipboard)
