#pragma once

#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class plIReflectedTypeAccessor;
class plDocumentObject;
class plAbstractObjectGraph;

/// \brief Helper functions for handling reflection related operations.
///
/// Also check out plToolsSerializationUtils for related functionality.
class PLASMA_TOOLSFOUNDATION_DLL plToolsReflectionUtils
{
public:
  /// \brief Returns the default value for the entire property as it is stored on the editor side.
  static plVariant GetStorageDefault(const plAbstractProperty* pProperty);

  static bool GetFloatFromVariant(const plVariant& val, double& out_fValue);
  static bool GetVariantFromFloat(double fValue, plVariantType::Enum type, plVariant& out_val);

  /// \brief Creates a ReflectedTypeDescriptor from an plRTTI instance that can be serialized and registered at the plPhantomRttiManager.
  static void GetReflectedTypeDescriptorFromRtti(const plRTTI* pRtti, plReflectedTypeDescriptor& out_desc); // [tested]
  static void GetMinimalReflectedTypeDescriptorFromRtti(const plRTTI* pRtti, plReflectedTypeDescriptor& out_desc);

  static void GatherObjectTypes(const plDocumentObject* pObject, plSet<const plRTTI*>& inout_types);

  static bool DependencySortTypeDescriptorArray(plDynamicArray<plReflectedTypeDescriptor*>& ref_descriptors);
};
