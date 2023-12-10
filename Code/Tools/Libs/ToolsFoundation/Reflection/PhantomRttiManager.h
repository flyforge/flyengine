#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/IdTable.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class plPhantomRTTI;

struct plPhantomRttiManagerEvent
{
  enum class Type
  {
    TypeAdded,
    TypeRemoved,
    TypeChanged,
  };

  plPhantomRttiManagerEvent()

    = default;

  Type m_Type = Type::TypeAdded;
  const plRTTI* m_pChangedType = nullptr;
};

/// \brief Manages all plPhantomRTTI types that have been added to him.
///
/// A plPhantomRTTI cannot be created directly but must be created via this managers
/// RegisterType function with a given plReflectedTypeDescriptor.
class PLASMA_TOOLSFOUNDATION_DLL plPhantomRttiManager
{
public:
  /// \brief Adds a reflected type to the list of accessible types.
  ///
  /// Types must be added in the correct order, any type must be added before
  /// it can be referenced in other types. Any base class must be added before
  /// any class deriving from it can be added.
  /// Call the function again if a type has changed during the run of the
  /// program. If the type actually differs the last known class layout the
  /// m_TypeChangedEvent event will be called with the old and new plRTTI.
  ///
  /// \sa plReflectionUtils::GetReflectedTypeDescriptorFromRtti
  static const plRTTI* RegisterType(plReflectedTypeDescriptor& ref_desc);

  /// \brief Removes a type from the list of accessible types.
  ///
  /// No instance of the given type or storage must still exist when this function is called.
  static bool UnregisterType(const plRTTI* pRtti);

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, ReflectedTypeManager);

  static void Startup();
  static void Shutdown();
  static void PluginEventHandler(const plPluginEvent& e);

public:
  static plCopyOnBroadcastEvent<const plPhantomRttiManagerEvent&> s_Events;

private:
  static plSet<const plRTTI*> s_RegisteredConcreteTypes;
  static plHashTable<plStringView, plPhantomRTTI*> s_NameToPhantom;
};
