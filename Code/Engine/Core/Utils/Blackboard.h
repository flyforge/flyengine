#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Variant.h>

class plStreamReader;
class plStreamWriter;

/// \brief Flags for entries in plBlackboard.
struct PL_CORE_DLL plBlackboardEntryFlags
{
  using StorageType = plUInt16;

  enum Enum
  {
    None = 0,
    Save = PL_BIT(0),          ///< Include the entry during serialization
    OnChangeEvent = PL_BIT(1), ///< Broadcast the 'ValueChanged' event when this entry's value is modified

    UserFlag0 = PL_BIT(7),
    UserFlag1 = PL_BIT(8),
    UserFlag2 = PL_BIT(9),
    UserFlag3 = PL_BIT(10),
    UserFlag4 = PL_BIT(11),
    UserFlag5 = PL_BIT(12),
    UserFlag6 = PL_BIT(13),
    UserFlag7 = PL_BIT(14),

    Invalid = PL_BIT(15),

    Default = None
  };

  struct Bits
  {
    StorageType Save : 1;
    StorageType OnChangeEvent : 1;
    StorageType Reserved : 5;
    StorageType UserFlag0 : 1;
    StorageType UserFlag1 : 1;
    StorageType UserFlag2 : 1;
    StorageType UserFlag3 : 1;
    StorageType UserFlag4 : 1;
    StorageType UserFlag5 : 1;
    StorageType UserFlag6 : 1;
    StorageType UserFlag7 : 1;
    StorageType Invalid : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plBlackboardEntryFlags);
PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plBlackboardEntryFlags);


/// \brief A blackboard is a key/value store that provides OnChange events to be informed when a value changes.
///
/// Blackboards are used to gather typically small pieces of data. Some systems write the data, other systems read it.
/// Through the blackboard, arbitrary systems can interact.
///
/// For example this is commonly used in game AI, where some system gathers interesting pieces of data about the environment,
/// and then NPCs might use that information to make decisions.
class PL_CORE_DLL plBlackboard : public plRefCounted
{
private:
  plBlackboard(bool bIsGlobal);

public:
  ~plBlackboard();

  bool IsGlobalBlackboard() const { return m_bIsGlobal; }

  /// \brief Factory method to create a new blackboard.
  ///
  /// Since blackboards use shared ownership we need to make sure that blackboards are created in plCore.dll.
  /// Some compilers (MSVC) create local v-tables which can become stale if a blackboard was registered as global but the DLL
  /// which created the blackboard is already unloaded.
  ///
  /// See https://groups.google.com/g/microsoft.public.vc.language/c/atSh_2VSc2w/m/EgJ3r_7OzVUJ?pli=1
  static plSharedPtr<plBlackboard> Create(plAllocator* pAllocator = plFoundation::GetDefaultAllocator());

  /// \brief Factory method to get access to a globally registered blackboard.
  ///
  /// If a blackboard with that name was already created globally before, its reference is returned.
  /// Otherwise it will be created and permanently registered under that name.
  /// Global blackboards cannot be removed. Although you can change their name via "SetName()",
  /// the name under which they are registered globally will not change.
  ///
  /// If at some point you want to "remove" a global blackboard, instead call UnregisterAllEntries() to
  /// clear all its values.
  static plSharedPtr<plBlackboard> GetOrCreateGlobal(const plHashedString& sBlackboardName, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());

  /// \brief Finds a global blackboard with the given name.
  static plSharedPtr<plBlackboard> FindGlobal(const plTempHashedString& sBlackboardName);

  /// \brief Changes the name of the blackboard.
  ///
  /// \note For global blackboards this has no effect under which name they are found. A global blackboard continues to
  /// be found by the name under which it was originally registered.
  void SetName(plStringView sName);
  const char* GetName() const { return m_sName; }
  const plHashedString& GetNameHashed() const { return m_sName; }

  struct Entry
  {
    plVariant m_Value;
    plBitflags<plBlackboardEntryFlags> m_Flags;

    /// The change counter is increased every time the entry's value changes.
    /// Read this and compare it to a previous known value, to detect whether the value was changed since the last check.
    plUInt32 m_uiChangeCounter = 0;
  };

  struct EntryEvent
  {
    plHashedString m_sName;
    plVariant m_OldValue;
    const Entry* m_pEntry;
  };

  /// \brief Removes the named entry. Does nothing, if no such entry exists.
  void RemoveEntry(const plHashedString& sName);

  ///  \brief Removes all entries.
  void RemoveAllEntries();

  /// \brief Returns whether an entry with the given name already exists.
  bool HasEntry(const plTempHashedString& sName) const;

  /// \brief Sets the value of the named entry. If the entry doesn't exist, yet, it will be created with default flags.
  ///
  /// If the 'OnChangeEvent' flag is set for this entry, OnEntryEvent() will be broadcast.
  /// However, if the new value is no different to the old, no event will be broadcast.
  ///
  /// For new entries, no OnEntryEvent() is sent.
  ///
  /// For best efficiency, cache the entry name in an plHashedString and use the other overload of this function.
  /// DO NOT RECREATE the plHashedString every time, though.
  void SetEntryValue(plStringView sName, const plVariant& value);

  /// \brief Overload of SetEntryValue() that takes an plHashedString rather than an plStringView.
  ///
  /// Using this function is more efficient, if you access the blackboard often, but you must ensure
  /// to only create the plHashedString once and cache it for reuse.
  /// Assigning a value to an plHashedString is an expensive operation, so if you do not cache the string,
  /// prefer to use the other overload.
  void SetEntryValue(const plHashedString& sName, const plVariant& value);

  /// \brief Returns a pointer to the named entry, or nullptr if no such entry was registered.
  const Entry* GetEntry(const plTempHashedString& sName) const;

  /// \brief Returns the flags of the named entry, or plBlackboardEntryFlags::Invalid, if no such entry was registered.
  plBitflags<plBlackboardEntryFlags> GetEntryFlags(const plTempHashedString& sName) const;

  /// \brief Sets the flags of an existing entry. Returns PL_FAILURE, if it wasn't created via SetEntryValue() or SetEntryValue() before.
  plResult SetEntryFlags(const plTempHashedString& sName, plBitflags<plBlackboardEntryFlags> flags);

  /// \brief Returns the value of the named entry, or the fallback plVariant, if no such entry was registered.
  plVariant GetEntryValue(const plTempHashedString& sName, const plVariant& fallback = plVariant()) const;

  /// \brief Increments the value of the named entry. Returns the incremented value or an invalid variant if the entry does not exist or is not a number type.
  plVariant IncrementEntryValue(const plTempHashedString& sName);

  /// \brief Decrements the value of the named entry. Returns the decremented value or an invalid variant if the entry does not exist or is not a number type.
  plVariant DecrementEntryValue(const plTempHashedString& sName);

  /// \brief Grants read access to the entire map of entries.
  const plHashTable<plHashedString, Entry>& GetAllEntries() const { return m_Entries; }

  /// \brief Allows you to register to the OnEntryEvent. This is broadcast whenever an entry is modified that has the flag plBlackboardEntryFlags::OnChangeEvent.
  const plEvent<EntryEvent>& OnEntryEvent() const { return m_EntryEvents; }

  /// \brief This counter is increased every time an entry is added or removed (but not when it is modified).
  ///
  /// Comparing this value to a previous known value allows to quickly detect whether the set of entries has changed.
  plUInt32 GetBlackboardChangeCounter() const { return m_uiBlackboardChangeCounter; }

  /// \brief This counter is increased every time any entry's value is modified.
  ///
  /// Comparing this value to a previous known value allows to quickly detect whether any entry has changed recently.
  plUInt32 GetBlackboardEntryChangeCounter() const { return m_uiBlackboardEntryChangeCounter; }

  /// \brief Stores all entries that have the 'Save' flag in the stream.
  plResult Serialize(plStreamWriter& inout_stream) const;

  /// \brief Restores entries from the stream.
  ///
  /// If the blackboard already contains entries, the deserialized data is ADDED to the blackboard.
  /// If deserialized entries overlap with existing ones, the deserialized entries will overwrite the existing ones (both values and flags).
  plResult Deserialize(plStreamReader& inout_stream);

private:
  PL_ALLOW_PRIVATE_PROPERTIES(plBlackboard);

  static plBlackboard* Reflection_GetOrCreateGlobal(const plHashedString& sName);
  static plBlackboard* Reflection_FindGlobal(plTempHashedString sName);
  void Reflection_SetEntryValue(plStringView sName, const plVariant& value);

  void ImplSetEntryValue(const plHashedString& sName, Entry& entry, const plVariant& value);

  bool m_bIsGlobal = false;
  plHashedString m_sName;
  plEvent<EntryEvent> m_EntryEvents;
  plUInt32 m_uiBlackboardChangeCounter = 0;
  plUInt32 m_uiBlackboardEntryChangeCounter = 0;
  plHashTable<plHashedString, Entry> m_Entries;

  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, Blackboard);
  static plMutex s_GlobalBlackboardsMutex;
  static plHashTable<plHashedString, plSharedPtr<plBlackboard>> s_GlobalBlackboards;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plBlackboard);

//////////////////////////////////////////////////////////////////////////

struct PL_CORE_DLL plBlackboardCondition
{
  plHashedString m_sEntryName;
  double m_fComparisonValue = 0.0;
  plEnum<plComparisonOperator> m_Operator;

  bool IsConditionMet(const plBlackboard& blackboard) const;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plBlackboardCondition);
