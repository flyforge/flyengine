#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

// *****************************************
// ***** Runtime Type Information Data *****

struct plRTTIAllocator;
class plAbstractProperty;
class plAbstractFunctionProperty;
class plAbstractMessageHandler;
struct plMessageSenderInfo;
class plPropertyAttribute;
class plMessage;
using plMessageId = plUInt16;

/// \brief This class holds information about reflected types. Each instance represents one type that is known to the reflection
/// system.
///
/// Instances of this class are typically created through the macros from the StaticRTTI.h header.
/// Each instance represents one type. This class holds information about derivation hierarchies and exposed properties. You can thus find
/// out whether a type is derived from some base class and what properties of which types are available. Properties can then be read and
/// modified on instances of this type.
class PLASMA_FOUNDATION_DLL plRTTI
{
public:
  /// \brief The constructor requires all the information about the type that this object represents.
  plRTTI(plStringView sName, const plRTTI* pParentType, plUInt32 uiTypeSize, plUInt32 uiTypeVersion, plUInt8 uiVariantType,
    plBitflags<plTypeFlags> flags, plRTTIAllocator* pAllocator, plArrayPtr<const plAbstractProperty*> properties, plArrayPtr<const plAbstractFunctionProperty*> functions,
    plArrayPtr<const plPropertyAttribute*> attributes, plArrayPtr<plAbstractMessageHandler*> messageHandlers,
    plArrayPtr<plMessageSenderInfo> messageSenders, const plRTTI* (*fnVerifyParent)());


  ~plRTTI();

  /// \brief Can be called in debug builds to check that all reflected objects are correctly set up.
  void VerifyCorrectness() const;

  /// \brief Calls VerifyCorrectness() on all plRTTI objects.
  static void VerifyCorrectnessForAllTypes();

  /// \brief Returns the name of this type.
  PLASMA_ALWAYS_INLINE plStringView GetTypeName() const { return m_sTypeName; } // [tested]

  /// \brief Returns the hash of the name of this type.
  PLASMA_ALWAYS_INLINE plUInt64 GetTypeNameHash() const { return m_uiTypeNameHash; } // [tested]

  /// \brief Returns the type that is the base class of this type. May be nullptr if this type has no base class.
  PLASMA_ALWAYS_INLINE const plRTTI* GetParentType() const { return m_pParentType; } // [tested]

  /// \brief Returns the corresponding variant type for this type or Invalid if there is none.
  PLASMA_ALWAYS_INLINE plVariantType::Enum GetVariantType() const { return static_cast<plVariantType::Enum>(m_uiVariantType); }

  /// \brief Returns true if this type is derived from the given type (or of the same type).
  PLASMA_ALWAYS_INLINE bool IsDerivedFrom(const plRTTI* pBaseType) const // [tested]
  {
    const plUInt32 thisGeneration = m_ParentHierarchy.GetCount();
    const plUInt32 baseGeneration = pBaseType->m_ParentHierarchy.GetCount();
    PLASMA_ASSERT_DEBUG(thisGeneration > 0 && baseGeneration > 0, "SetupParentHierarchy() has not been called");
    return thisGeneration >= baseGeneration && m_ParentHierarchy.GetData()[thisGeneration - baseGeneration] == pBaseType;
  }

  /// \brief Returns true if this type is derived from or identical to the given type.
  template <typename BASE>
  PLASMA_ALWAYS_INLINE bool IsDerivedFrom() const // [tested]
  {
    return IsDerivedFrom(plGetStaticRTTI<BASE>());
  }

  /// \brief Returns the object through which instances of this type can be allocated.
  PLASMA_ALWAYS_INLINE plRTTIAllocator* GetAllocator() const { return m_pAllocator; } // [tested]

  /// \brief Returns the array of properties that this type has. Does NOT include properties from base classes.
  PLASMA_ALWAYS_INLINE plArrayPtr<const plAbstractProperty* const> GetProperties() const { return m_Properties; } // [tested]

  PLASMA_ALWAYS_INLINE plArrayPtr<const plAbstractFunctionProperty* const> GetFunctions() const { return m_Functions; }

  PLASMA_ALWAYS_INLINE plArrayPtr<const plPropertyAttribute* const> GetAttributes() const { return m_Attributes; }

  /// \brief Returns the first attribute that derives from the given type, or nullptr if nothing is found.
  template <typename Type>
  const Type* GetAttributeByType() const;

  /// \brief Returns the list of properties that this type has, including derived properties from all base classes.
  void GetAllProperties(plDynamicArray<const plAbstractProperty*>& out_properties) const; // [tested]

  /// \brief Returns the size (in bytes) of an instance of this type.
  PLASMA_ALWAYS_INLINE plUInt32 GetTypeSize() const { return m_uiTypeSize; } // [tested]

  /// \brief Returns the version number of this type.
  PLASMA_ALWAYS_INLINE plUInt32 GetTypeVersion() const { return m_uiTypeVersion; }

  /// \brief Returns the type flags.
  PLASMA_ALWAYS_INLINE const plBitflags<plTypeFlags>& GetTypeFlags() const { return m_TypeFlags; } // [tested]

  /// \brief Searches all plRTTI instances for the one with the given name, or nullptr if no such type exists.
  static const plRTTI* FindTypeByName(plStringView sName); // [tested]

  /// \brief Searches all plRTTI instances for the one with the given hashed name, or nullptr if no such type exists.
  static const plRTTI* FindTypeByNameHash(plUInt64 uiNameHash); // [tested]
  static const plRTTI* FindTypeByNameHash32(plUInt32 uiNameHash);

  using PredicateFunc = plDelegate<bool(const plRTTI*), 48>;
  /// \brief Searches all plRTTI instances for one where the given predicate function returns true
  static const plRTTI* FindTypeIf(PredicateFunc func);

  /// \brief Will iterate over all properties of this type and (optionally) the base types to search for a property with the given name.
  const plAbstractProperty* FindPropertyByName(plStringView sName, bool bSearchBaseTypes = true) const; // [tested]

  /// \brief Returns the name of the plugin which this type is declared in.
  PLASMA_ALWAYS_INLINE plStringView GetPluginName() const { return m_sPluginName; } // [tested]

  /// \brief Returns the array of message handlers that this type has.
  PLASMA_ALWAYS_INLINE const plArrayPtr<plAbstractMessageHandler*>& GetMessageHandlers() const { return m_MessageHandlers; }

  /// \brief Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message
  /// handler for this type exists.
  bool DispatchMessage(void* pInstance, plMessage& ref_msg) const;

  /// \brief Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message
  /// handler for this type exists.
  bool DispatchMessage(const void* pInstance, plMessage& ref_msg) const;

  /// \brief Returns whether this type can handle the given message type.
  template <typename MessageType>
  PLASMA_ALWAYS_INLINE bool CanHandleMessage() const
  {
    return CanHandleMessage(MessageType::GetTypeMsgId());
  }

  /// \brief Returns whether this type can handle the message type with the given id.
  inline bool CanHandleMessage(plMessageId id) const
  {
    PLASMA_ASSERT_DEBUG(m_uiMsgIdOffset != plSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                            "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                            "you may have forgotten to instantiate an plPlugin object inside your plugin DLL.");

    const plUInt32 uiIndex = id - m_uiMsgIdOffset;
    return uiIndex < m_DynamicMessageHandlers.GetCount() && m_DynamicMessageHandlers.GetData()[uiIndex] != nullptr;
  }

  PLASMA_ALWAYS_INLINE const plArrayPtr<plMessageSenderInfo>& GetMessageSender() const { return m_MessageSenders; }

  struct ForEachOptions
  {
    using StorageType = plUInt8;

    enum Enum
    {
      None = 0,
      ExcludeNonAllocatable = PLASMA_BIT(0),
      ExcludeAbstract = PLASMA_BIT(1),

      Default = None
    };

    struct Bits
    {
      plUInt8 ExcludeNonAllocatable : 1;
    };
  };

  using VisitorFunc = plDelegate<void(const plRTTI*), 48>;
  static void ForEachType(VisitorFunc func, plBitflags<ForEachOptions> options = ForEachOptions::Default); // [tested]

  static void ForEachDerivedType(const plRTTI* pBaseType, VisitorFunc func, plBitflags<ForEachOptions> options = ForEachOptions::Default);

  template <typename T>
  static PLASMA_ALWAYS_INLINE void ForEachDerivedType(VisitorFunc func, plBitflags<ForEachOptions> options = ForEachOptions::Default)
  {
    ForEachDerivedType(plGetStaticRTTI<T>(), func, options);
  }

protected:
  plStringView m_sPluginName;
  plStringView m_sTypeName;
  plArrayPtr<const plAbstractProperty* const> m_Properties;
  plArrayPtr<const plAbstractFunctionProperty* const> m_Functions;
  plArrayPtr<const plPropertyAttribute* const> m_Attributes;
  void UpdateType(const plRTTI* pParentType, plUInt32 uiTypeSize, plUInt32 uiTypeVersion, plUInt8 uiVariantType, plBitflags<plTypeFlags> flags);
  void RegisterType();
  void UnregisterType();

  void GatherDynamicMessageHandlers();
  void SetupParentHierarchy();

  const plRTTI* m_pParentType = nullptr;
  plRTTIAllocator* m_pAllocator = nullptr;

  plUInt32 m_uiTypeSize = 0;
  plUInt32 m_uiTypeVersion = 0;
  plUInt64 m_uiTypeNameHash = 0;
  plUInt32 m_uiTypeIndex = 0;
  plBitflags<plTypeFlags> m_TypeFlags;
  plUInt8 m_uiVariantType = 0;
  plUInt16 m_uiMsgIdOffset = plSmallInvalidIndex;

  const plRTTI* (*m_VerifyParent)();

  plArrayPtr<plAbstractMessageHandler*> m_MessageHandlers;
  plSmallArray<plAbstractMessageHandler*, 1, plStaticAllocatorWrapper> m_DynamicMessageHandlers; // do not track this data, it won't be deallocated before shutdown

  plArrayPtr<plMessageSenderInfo> m_MessageSenders;
  plSmallArray<const plRTTI*, 7, plStaticAllocatorWrapper> m_ParentHierarchy;

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Reflection);

  /// \brief Assigns the given plugin name to every plRTTI instance that has no plugin assigned yet.
  static void AssignPlugin(plStringView sPluginName);

  static void SanityCheckType(plRTTI* pType);

  /// \brief Handles events by plPlugin, to figure out which types were provided by which plugin
  static void PluginEventHandler(const plPluginEvent& EventData);
};

PLASMA_DECLARE_FLAGS_OPERATORS(plRTTI::ForEachOptions);


// ***********************************
// ***** Object Allocator Struct *****


/// \brief The interface for an allocator that creates instances of reflected types.
struct PLASMA_FOUNDATION_DLL plRTTIAllocator
{
  virtual ~plRTTIAllocator();

  /// \brief Returns whether the type that is represented by this allocator, can be dynamically allocated at runtime.
  virtual bool CanAllocate() const { return true; } // [tested]

  /// \brief Allocates one instance.
  template <typename T>
  plInternal::NewInstance<T> Allocate(plAllocatorBase* pAllocator = nullptr)
  {
    return AllocateInternal(pAllocator).Cast<T>();
  }

  /// \brief Clones the given instance.
  template <typename T>
  plInternal::NewInstance<T> Clone(const void* pObject, plAllocatorBase* pAllocator = nullptr)
  {
    return CloneInternal(pObject, pAllocator).Cast<T>();
  }

  /// \brief Deallocates the given instance.
  virtual void Deallocate(void* pObject, plAllocatorBase* pAllocator = nullptr) = 0; // [tested]

private:
  virtual plInternal::NewInstance<void> AllocateInternal(plAllocatorBase* pAllocator) = 0;
  virtual plInternal::NewInstance<void> CloneInternal(const void* pObject, plAllocatorBase* pAllocator)
  {
    PLASMA_REPORT_FAILURE("Cloning is not supported by this allocator.");
    return plInternal::NewInstance<void>(nullptr, pAllocator);
  }
};

/// \brief Dummy Allocator for types that should not be allocatable through the reflection system.
struct PLASMA_FOUNDATION_DLL plRTTINoAllocator : public plRTTIAllocator
{
  /// \brief Returns false, because this type of allocator is used for classes that shall not be allocated dynamically.
  virtual bool CanAllocate() const override { return false; } // [tested]

  /// \brief Will trigger an assert.
  virtual plInternal::NewInstance<void> AllocateInternal(plAllocatorBase* pAllocator) override // [tested]
  {
    PLASMA_REPORT_FAILURE("This function should never be called.");
    return plInternal::NewInstance<void>(nullptr, pAllocator);
  }

  /// \brief Will trigger an assert.
  virtual void Deallocate(void* pObject, plAllocatorBase* pAllocator) override // [tested]
  {
    PLASMA_REPORT_FAILURE("This function should never be called.");
  }
};

/// \brief Default implementation of plRTTIAllocator that allocates instances via the given allocator.
template <typename CLASS, typename AllocatorWrapper = plDefaultAllocatorWrapper>
struct plRTTIDefaultAllocator : public plRTTIAllocator
{
  /// \brief Returns a new instance that was allocated with the given allocator.
  virtual plInternal::NewInstance<void> AllocateInternal(plAllocatorBase* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    return PLASMA_NEW(pAllocator, CLASS);
  }

  /// \brief Clones the given instance with the given allocator.
  virtual plInternal::NewInstance<void> CloneInternal(const void* pObject, plAllocatorBase* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    return CloneImpl(pObject, pAllocator, plTraitInt<std::is_copy_constructible<CLASS>::value>());
  }

  /// \brief Deletes the given instance with the given allocator.
  virtual void Deallocate(void* pObject, plAllocatorBase* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    CLASS* pPointer = static_cast<CLASS*>(pObject);
    PLASMA_DELETE(pAllocator, pPointer);
  }

private:
  plInternal::NewInstance<void> CloneImpl(const void* pObject, plAllocatorBase* pAllocator, plTraitInt<0>)
  {
    PLASMA_REPORT_FAILURE("Clone failed since the type is not copy constructible");
    return plInternal::NewInstance<void>(nullptr, pAllocator);
  }

  plInternal::NewInstance<void> CloneImpl(const void* pObject, plAllocatorBase* pAllocator, plTraitInt<1>)
  {
    return PLASMA_NEW(pAllocator, CLASS, *static_cast<const CLASS*>(pObject));
  }
};
