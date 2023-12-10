#pragma once

#include <Foundation/Types/TypeTraits.h>

class plRTTI;

/// \brief A typed raw pointer.
///
/// Common use case is the storage of object pointers inside an plVariant.
/// Has the same lifetime concerns that any other raw pointer.
/// \sa plVariant
struct plTypedPointer
{
  PLASMA_DECLARE_POD_TYPE();
  void* m_pObject = nullptr;
  const plRTTI* m_pType = nullptr;

  plTypedPointer() = default;
  plTypedPointer(void* pObject, const plRTTI* pType)
    : m_pObject(pObject)
    , m_pType(pType)
  {
  }

  bool operator==(const plTypedPointer& rhs) const
  {
    return m_pObject == rhs.m_pObject;
  }
  bool operator!=(const plTypedPointer& rhs) const
  {
    return m_pObject != rhs.m_pObject;
  }
};
