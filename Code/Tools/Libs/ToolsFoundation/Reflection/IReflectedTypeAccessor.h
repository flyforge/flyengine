#pragma once

#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

class plDocumentObject;
struct plStatus;

/// \brief Provides access to the properties of an plRTTI compatible data storage.
class PL_TOOLSFOUNDATION_DLL plIReflectedTypeAccessor
{
public:
  /// \brief Constructor for the plIReflectedTypeAccessor.
  ///
  /// It is a valid implementation to pass an invalid handle. Note that in this case there is no way to determine
  /// what is actually stored inside. However, it can be useful to use e.g. the plReflectedTypeDirectAccessor
  /// to set properties on the engine runtime side without having the plPhantomRttiManager initialized.
  plIReflectedTypeAccessor(const plRTTI* pRtti, plDocumentObject* pOwner)
    : m_pRtti(pRtti)
    , m_pOwner(pOwner)
  {
  } // [tested]

  /// \brief Returns the plRTTI* of the wrapped instance type.
  const plRTTI* GetType() const { return m_pRtti; } // [tested]

  /// \brief Returns the value of the property defined by its path. Return value is invalid iff the path was invalid.
  virtual const plVariant GetValue(plStringView sProperty, plVariant index = plVariant(), plStatus* pRes = nullptr) const = 0;

  /// \brief Sets a property defined by its path to the given value. Returns whether the operation was successful.
  virtual bool SetValue(plStringView sProperty, const plVariant& value, plVariant index = plVariant()) = 0;

  virtual plInt32 GetCount(plStringView sProperty) const = 0;
  virtual bool GetKeys(plStringView sProperty, plDynamicArray<plVariant>& out_keys) const = 0;

  virtual bool InsertValue(plStringView sProperty, plVariant index, const plVariant& value) = 0;
  virtual bool RemoveValue(plStringView sProperty, plVariant index) = 0;
  virtual bool MoveValue(plStringView sProperty, plVariant oldIndex, plVariant newIndex) = 0;

  virtual plVariant GetPropertyChildIndex(plStringView sProperty, const plVariant& value) const = 0;

  const plDocumentObject* GetOwner() const { return m_pOwner; }

  bool GetValues(plStringView sProperty, plDynamicArray<plVariant>& out_values) const;


private:
  friend class plDocumentObjectManager;
  friend class plDocumentObject;

  const plRTTI* m_pRtti;
  plDocumentObject* m_pOwner;
};
