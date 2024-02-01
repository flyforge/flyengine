#pragma once

#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/Variant.h>

class plDocumentObject;

struct PL_GUIFOUNDATION_DLL plPropertySelection
{
  const plDocumentObject* m_pObject;
  plVariant m_Index;

  bool operator==(const plPropertySelection& rhs) const { return m_pObject == rhs.m_pObject && m_Index == rhs.m_Index; }

  bool operator<(const plPropertySelection& rhs) const
  {
    // Qt6 requires the less than operator but never calls it, so we use this dummy for now.
    PL_ASSERT_NOT_IMPLEMENTED;
    return false;
  }
};

struct PL_GUIFOUNDATION_DLL plPropertyClipboard
{
  plString m_Type;
  plVariant m_Value;
};
PL_DECLARE_REFLECTABLE_TYPE(PL_GUIFOUNDATION_DLL, plPropertyClipboard)
