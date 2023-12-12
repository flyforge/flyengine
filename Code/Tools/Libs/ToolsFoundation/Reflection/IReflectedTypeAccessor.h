#pragma once

#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

class plDocumentObject;
struct plStatus;

/// \brief Provides access to the properties of an plRTTI compatible data storage.
class PLASMA_TOOLSFOUNDATION_DLL plIReflectedTypeAccessor
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
  virtual const plVariant GetValue(const char* szProperty, plVariant index = plVariant(), plStatus* res = nullptr) const = 0;

  /// \brief Sets a property defined by its path to the given value. Returns whether the operation was successful.
  virtual bool SetValue(const char* szProperty, const plVariant& value, plVariant index = plVariant()) = 0;

  virtual plInt32 GetCount(const char* szProperty) const = 0;
  virtual bool GetKeys(const char* szProperty, plDynamicArray<plVariant>& out_keys) const = 0;

  virtual bool InsertValue(const char* szProperty, plVariant index, const plVariant& value) = 0;
  virtual bool RemoveValue(const char* szProperty, plVariant index) = 0;
  virtual bool MoveValue(const char* szProperty, plVariant oldIndex, plVariant newIndex) = 0;

  virtual plVariant GetPropertyChildIndex(const char* szProperty, const plVariant& value) const = 0;

  const plDocumentObject* GetOwner() const { return m_pOwner; }

  bool GetValues(const char* szProperty, plDynamicArray<plVariant>& out_values) const;


private:
  friend class plDocumentObjectManager;
  friend class plDocumentObject;

  const plRTTI* m_pRtti;
  plDocumentObject* m_pOwner;
};
