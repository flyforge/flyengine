#pragma once

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>

/// \brief An plIReflectedTypeAccessor implementation that also stores the actual data that is defined in the passed plRTTI.
///
/// This class is used to store data on the tool side for classes that are not known to the tool but exist outside of it
/// like engine components. As this is basically a complex value map the used type can be hot-reloaded. For this, the
/// plRTTI just needs to be updated with its new definition in the plPhantomRttiManager and all plReflectedTypeStorageAccessor
/// will be automatically rearranged to match the new class layout.
class PLASMA_TOOLSFOUNDATION_DLL plReflectedTypeStorageAccessor : public plIReflectedTypeAccessor
{
  friend class plReflectedTypeStorageManager;

public:
  plReflectedTypeStorageAccessor(const plRTTI* hReflectedType, plDocumentObject* pOwner); // [tested]
  ~plReflectedTypeStorageAccessor();

  virtual const plVariant GetValue(const char* szProperty, plVariant index = plVariant(), plStatus* res = nullptr) const override; // [tested]
  virtual bool SetValue(const char* szProperty, const plVariant& value, plVariant index = plVariant()) override;                   // [tested]

  virtual plInt32 GetCount(const char* szProperty) const override;
  virtual bool GetKeys(const char* szProperty, plDynamicArray<plVariant>& out_keys) const override;

  virtual bool InsertValue(const char* szProperty, plVariant index, const plVariant& value) override;
  virtual bool RemoveValue(const char* szProperty, plVariant index) override;
  virtual bool MoveValue(const char* szProperty, plVariant oldIndex, plVariant newIndex) override;

  virtual plVariant GetPropertyChildIndex(const char* szProperty, const plVariant& value) const override;

private:
  plDynamicArray<plVariant> m_Data;
  const plReflectedTypeStorageManager::ReflectedTypeStorageMapping* m_pMapping;
};
