#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Utilities/EnumerableClass.h>

class plStreamWriter;
class plStreamReader;
class plVariantTypeInfo;

/// \brief Variant type registry allows for custom variant type infos to be accessed.
///
/// Custom variant types are defined via the PLASMA_DECLARE_CUSTOM_VARIANT_TYPE and PLASMA_DEFINE_CUSTOM_VARIANT_TYPE macros.
/// \sa PLASMA_DECLARE_CUSTOM_VARIANT_TYPE, PLASMA_DEFINE_CUSTOM_VARIANT_TYPE
class PLASMA_FOUNDATION_DLL plVariantTypeRegistry
{
  PLASMA_DECLARE_SINGLETON(plVariantTypeRegistry);

public:
  /// \brief Find the variant type info for the given plRTTI type.
  /// \return plVariantTypeInfo if one exits for the given type, otherwise nullptr.
  const plVariantTypeInfo* FindVariantTypeInfo(const plRTTI* pType) const;
  ~plVariantTypeRegistry();

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, VariantTypeRegistry);
  plVariantTypeRegistry();

  void PluginEventHandler(const plPluginEvent& EventData);
  void UpdateTypes();

  plHashTable<const plRTTI*, const plVariantTypeInfo*> m_TypeInfos;
};

/// \brief Defines functions to allow the full feature set of plVariant to be used.
/// \sa PLASMA_DEFINE_CUSTOM_VARIANT_TYPE, plVariantTypeRegistry
class PLASMA_FOUNDATION_DLL plVariantTypeInfo : public plEnumerable<plVariantTypeInfo>
{
public:
  plVariantTypeInfo();
  virtual const plRTTI* GetType() const = 0;
  virtual plUInt32 Hash(const void* pObject) const = 0;
  virtual bool Equal(const void* pObjectA, const void* pObjectB) const = 0;
  virtual void Serialize(plStreamWriter& ref_writer, const void* pObject) const = 0;
  virtual void Deserialize(plStreamReader& ref_reader, void* pObject) const = 0;

  PLASMA_DECLARE_ENUMERABLE_CLASS(plVariantTypeInfo);
};

/// \brief Helper template used by PLASMA_DEFINE_CUSTOM_VARIANT_TYPE.
/// \sa PLASMA_DEFINE_CUSTOM_VARIANT_TYPE
template <typename T>
class plVariantTypeInfoT : public plVariantTypeInfo
{
  const plRTTI* GetType() const override
  {
    return plGetStaticRTTI<T>();
  }
  plUInt32 Hash(const void* pObject) const override
  {
    return plHashHelper<T>::Hash(*static_cast<const T*>(pObject));
  }
  bool Equal(const void* pObjectA, const void* pObjectB) const override
  {
    return plHashHelper<T>::Equal(*static_cast<const T*>(pObjectA), *static_cast<const T*>(pObjectB));
  }
  void Serialize(plStreamWriter& writer, const void* pObject) const override
  {
    writer << *static_cast<const T*>(pObject);
  }
  void Deserialize(plStreamReader& reader, void* pObject) const override
  {
    reader >> *static_cast<T*>(pObject);
  }
};

/// \brief Defines a custom variant type, allowing it to be serialized and compared. The type needs to be declared first before using this macro.
///
/// The given type must implement plHashHelper and plStreamWriter / plStreamReader operators.
/// Macros should be placed in any cpp. Note that once a custom type is defined, it is considered a value type and will be passed by value. It must be linked into every editor and engine dll to allow serialization. Thus it should only be used for common types in base libraries.
/// Limitations: Currently only member variables are supported on custom types, no arrays, set, maps etc. For best performance, any custom type smaller than 16 bytes should be POD so it can be inlined into the plVariant.
/// \sa PLASMA_DECLARE_CUSTOM_VARIANT_TYPE, plVariantTypeRegistry, plVariant
#define PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(TYPE)                                                                                                                                       \
  PLASMA_CHECK_AT_COMPILETIME_MSG(plVariantTypeDeduction<TYPE>::value == plVariantType::TypedObject, "PLASMA_DECLARE_CUSTOM_VARIANT_TYPE needs to be added to the header defining TYPE"); \
  plVariantTypeInfoT<TYPE> g_plVariantTypeInfoT_##TYPE;
