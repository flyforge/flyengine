#pragma once

#include <Foundation/Containers/Set.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

class plReflectedTypeStorageAccessor;
class plDocumentObject;

/// \brief Manages all plReflectedTypeStorageAccessor instances.
///
/// This class takes care of patching all plReflectedTypeStorageAccessor instances when their
/// plRTTI is modified. It also provides the mapping from property name to the data
/// storage index of the corresponding plVariant in the plReflectedTypeStorageAccessor.
class PLASMA_TOOLSFOUNDATION_DLL plReflectedTypeStorageManager
{
public:
  plReflectedTypeStorageManager();

private:
  struct ReflectedTypeStorageMapping
  {
    struct StorageInfo
    {
      StorageInfo()
        : m_uiIndex(0)
        , m_Type(plVariant::Type::Invalid)
      {
      }
      StorageInfo(plUInt16 uiIndex, plVariant::Type::Enum type, const plVariant& defaultValue)
        : m_uiIndex(uiIndex)
        , m_Type(type)
        , m_DefaultValue(defaultValue)
      {
      }

      plUInt16 m_uiIndex;
      plEnum<plVariant::Type> m_Type;
      plVariant m_DefaultValue;
    };

    /// \brief Flattens all POD type properties of the given plRTTI into m_PathToStorageInfoTable.
    ///
    /// The functions first adds all parent class properties and then adds its own properties.
    /// POD type properties are added under the current path.
    void AddProperties(const plRTTI* pType);
    void AddPropertiesRecursive(const plRTTI* pType, plSet<const plDocumentObject*>& requiresPatchingEmbeddedClass);

    void UpdateInstances(plUInt32 uiIndex, const plAbstractProperty* pProperty, plSet<const plDocumentObject*>& requiresPatchingEmbeddedClass);
    void AddPropertyToInstances(plUInt32 uiIndex, const plAbstractProperty* pProperty, plSet<const plDocumentObject*>& requiresPatchingEmbeddedClass);

    plVariantType::Enum GetStorageType(const plAbstractProperty* pProperty);

    plSet<plReflectedTypeStorageAccessor*> m_Instances;
    plHashTable<plString, StorageInfo> m_PathToStorageInfoTable;
  };

  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, ReflectedTypeStorageManager);
  friend class plReflectedTypeStorageAccessor;

  static void Startup();
  static void Shutdown();

  static const ReflectedTypeStorageMapping* AddStorageAccessor(plReflectedTypeStorageAccessor* pInstance);
  static void RemoveStorageAccessor(plReflectedTypeStorageAccessor* pInstance);

  static ReflectedTypeStorageMapping* GetTypeStorageMapping(const plRTTI* pType);
  static void TypeEventHandler(const plPhantomRttiManagerEvent& e);

private:
  static plMap<const plRTTI*, ReflectedTypeStorageMapping*> s_ReflectedTypeToStorageMapping;
};
