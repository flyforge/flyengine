#pragma once

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>

/// \brief An plIReflectedTypeAccessor implementation that also stores the actual data that is defined in the passed plRTTI.
///
/// This class is used to store data on the tool side for classes that are not known to the tool but exist outside of it
/// like engine components. As this is basically a complex value map the used type can be hot-reloaded. For this, the
/// plRTTI just needs to be updated with its new definition in the plPhantomRttiManager and all plReflectedTypeStorageAccessor
/// will be automatically rearranged to match the new class layout.
class PL_TOOLSFOUNDATION_DLL plReflectedTypeStorageAccessor : public plIReflectedTypeAccessor
{
  friend class plReflectedTypeStorageManager;

public:
  plReflectedTypeStorageAccessor(const plRTTI* pReflectedType, plDocumentObject* pOwner); // [tested]
  ~plReflectedTypeStorageAccessor();

  virtual const plVariant GetValue(plStringView sProperty, plVariant index = plVariant(), plStatus* pRes = nullptr) const override; // [tested]
  virtual bool SetValue(plStringView sProperty, const plVariant& value, plVariant index = plVariant()) override;                    // [tested]

  virtual plInt32 GetCount(plStringView sProperty) const override;
  virtual bool GetKeys(plStringView sProperty, plDynamicArray<plVariant>& out_keys) const override;

  virtual bool InsertValue(plStringView sProperty, plVariant index, const plVariant& value) override;
  virtual bool RemoveValue(plStringView sProperty, plVariant index) override;
  virtual bool MoveValue(plStringView sProperty, plVariant oldIndex, plVariant newIndex) override;

  virtual plVariant GetPropertyChildIndex(plStringView sProperty, const plVariant& value) const override;

private:
  plDynamicArray<plVariant> m_Data;
  const plReflectedTypeStorageManager::ReflectedTypeStorageMapping* m_pMapping;
};
