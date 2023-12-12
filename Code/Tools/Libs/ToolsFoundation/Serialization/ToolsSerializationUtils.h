#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class plDocumentObjectManager;
class plDocumentObject;
class plRTTI;

/// \brief Helper functions for serializing data
///
/// Also check out plToolsReflectionUtils for related functionality.
class PLASMA_TOOLSFOUNDATION_DLL plToolsSerializationUtils
{
public:
  using FilterFunction = plDelegate<bool(const plAbstractProperty*)>;

  static void SerializeTypes(const plSet<const plRTTI*>& types, plAbstractObjectGraph& typesGraph);

  static void CopyProperties(const plDocumentObject* pSource, const plDocumentObjectManager* pSourceManager, void* pTarget, const plRTTI* pTargetType, FilterFunction PropertFilter = nullptr);
};
