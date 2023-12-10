#include <Core/CorePCH.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_BITFLAGS(plBlackboardEntryFlags, 1)
  PLASMA_BITFLAGS_CONSTANTS(plBlackboardEntryFlags::Save, plBlackboardEntryFlags::OnChangeEvent,
    plBlackboardEntryFlags::UserFlag0, plBlackboardEntryFlags::UserFlag1, plBlackboardEntryFlags::UserFlag2, plBlackboardEntryFlags::UserFlag3, plBlackboardEntryFlags::UserFlag4, plBlackboardEntryFlags::UserFlag5, plBlackboardEntryFlags::UserFlag6, plBlackboardEntryFlags::UserFlag7)
PLASMA_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plBlackboard, plNoBase, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Reflection_GetOrCreateGlobal, In, "Name")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardNamesEnum"))),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Reflection_FindGlobal, In, "Name")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardNamesEnum"))),

    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetName),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Reflection_SetEntryValue, In, "Name", In, "Value")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetEntryValue, In, "Name", In, "Fallback")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(IncrementEntryValue, In, "Name")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(DecrementEntryValue, In, "Name")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetBlackboardChangeCounter),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetBlackboardEntryChangeCounter)
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Core, Blackboard)

  ON_CORESYSTEMS_SHUTDOWN
  {
    PLASMA_LOCK(plBlackboard::s_GlobalBlackboardsMutex);
    plBlackboard::s_GlobalBlackboards.Clear();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

// static
plMutex plBlackboard::s_GlobalBlackboardsMutex;
plHashTable<plHashedString, plSharedPtr<plBlackboard>> plBlackboard::s_GlobalBlackboards;

// static
plSharedPtr<plBlackboard> plBlackboard::Create(plAllocatorBase* pAllocator /*= plFoundation::GetDefaultAllocator()*/)
{
  return PLASMA_NEW(pAllocator, plBlackboard);
}

// static
plSharedPtr<plBlackboard> plBlackboard::GetOrCreateGlobal(const plHashedString& sBlackboardName, plAllocatorBase* pAllocator /*= plFoundation::GetDefaultAllocator()*/)
{
  PLASMA_LOCK(s_GlobalBlackboardsMutex);

  auto it = s_GlobalBlackboards.Find(sBlackboardName);

  if (it.IsValid())
  {
    return it.Value();
  }

  plSharedPtr<plBlackboard> pShrd = PLASMA_NEW(pAllocator, plBlackboard);
  pShrd->m_sName = sBlackboardName;
  s_GlobalBlackboards.Insert(sBlackboardName, pShrd);

  return pShrd;
}

// static
plSharedPtr<plBlackboard> plBlackboard::FindGlobal(const plTempHashedString& sBlackboardName)
{
  PLASMA_LOCK(s_GlobalBlackboardsMutex);

  plSharedPtr<plBlackboard> pBlackboard;
  s_GlobalBlackboards.TryGetValue(sBlackboardName, pBlackboard);
  return pBlackboard;
}

plBlackboard::plBlackboard() = default;
plBlackboard::~plBlackboard() = default;

void plBlackboard::SetName(plStringView sName)
{
  PLASMA_LOCK(s_GlobalBlackboardsMutex);
  m_sName.Assign(sName);
}

void plBlackboard::RegisterEntry(const plHashedString& sName, const plVariant& initialValue, plBitflags<plBlackboardEntryFlags> flags /*= plBlackboardEntryFlags::None*/)
{
  PLASMA_ASSERT_ALWAYS(!flags.IsSet(plBlackboardEntryFlags::Invalid), "The invalid flag is reserved for internal use.");

  bool bExisted = false;
  Entry& entry = m_Entries.FindOrAdd(sName, &bExisted);

  if (!bExisted || entry.m_Flags != flags)
  {
    ++m_uiBlackboardChangeCounter;
    entry.m_Flags |= flags;
  }

  if (!bExisted && entry.m_Value != initialValue)
  {
    // broadcasts the change event, in case we overwrite an existing entry
    SetEntryValue(sName, initialValue).IgnoreResult();
  }
}

void plBlackboard::UnregisterEntry(const plHashedString& sName)
{
  if (m_Entries.Remove(sName))
  {
    ++m_uiBlackboardChangeCounter;
  }
}

void plBlackboard::UnregisterAllEntries()
{
  if (m_Entries.IsEmpty() == false)
  {
    ++m_uiBlackboardChangeCounter;
  }

  m_Entries.Clear();
}

plResult plBlackboard::SetEntryValue(const plTempHashedString& sName, const plVariant& value, bool bForce /*= false*/)
{
  auto itEntry = m_Entries.Find(sName);

  if (!itEntry.IsValid())
  {
    return PLASMA_FAILURE;
  }

  Entry& entry = itEntry.Value();

  if (!bForce && entry.m_Value == value)
    return PLASMA_SUCCESS;

  ++m_uiBlackboardEntryChangeCounter;
  ++entry.m_uiChangeCounter;

  if (entry.m_Flags.IsSet(plBlackboardEntryFlags::OnChangeEvent))
  {
    EntryEvent e;
    e.m_sName = itEntry.Key();
    e.m_OldValue = entry.m_Value;
    e.m_pEntry = &entry;

    entry.m_Value = value;

    m_EntryEvents.Broadcast(e, 1); // limited recursion is allowed
  }
  else
  {
    entry.m_Value = value;
  }

  return PLASMA_SUCCESS;
}

const plBlackboard::Entry* plBlackboard::GetEntry(const plTempHashedString& sName) const
{
  auto itEntry = m_Entries.Find(sName);

  if (!itEntry.IsValid())
    return nullptr;

  return &itEntry.Value();
}

plVariant plBlackboard::GetEntryValue(const plTempHashedString& sName, const plVariant& fallback /*= plVariant()*/) const
{
  auto pEntry = m_Entries.GetValue(sName);
  return pEntry != nullptr ? pEntry->m_Value : fallback;
}

plVariant plBlackboard::IncrementEntryValue(const plTempHashedString& sName)
{
  auto pEntry = m_Entries.GetValue(sName);
  if (pEntry != nullptr && pEntry->m_Value.IsNumber())
  {
    plVariant one = plVariant(1).ConvertTo(pEntry->m_Value.GetType());
    pEntry->m_Value = pEntry->m_Value + one;
    return pEntry->m_Value;
  }

  return plVariant();
}

plVariant plBlackboard::DecrementEntryValue(const plTempHashedString& sName)
{
  auto pEntry = m_Entries.GetValue(sName);
  if (pEntry != nullptr && pEntry->m_Value.IsNumber())
  {
    plVariant one = plVariant(1).ConvertTo(pEntry->m_Value.GetType());
    pEntry->m_Value = pEntry->m_Value - one;
    return pEntry->m_Value;
  }

  return plVariant();
}

plBitflags<plBlackboardEntryFlags> plBlackboard::GetEntryFlags(const plTempHashedString& sName) const
{
  auto itEntry = m_Entries.Find(sName);

  if (!itEntry.IsValid())
  {
    return plBlackboardEntryFlags::Invalid;
  }

  return itEntry.Value().m_Flags;
}

plResult plBlackboard::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  plUInt32 uiEntries = 0;

  for (auto it : m_Entries)
  {
    if (it.Value().m_Flags.IsSet(plBlackboardEntryFlags::Save))
    {
      ++uiEntries;
    }
  }

  inout_stream << uiEntries;

  for (auto it : m_Entries)
  {
    const Entry& e = it.Value();

    if (e.m_Flags.IsSet(plBlackboardEntryFlags::Save))
    {
      inout_stream << it.Key();
      inout_stream << e.m_Flags;
      inout_stream << e.m_Value;
    }
  }

  return PLASMA_SUCCESS;
}

plResult plBlackboard::Deserialize(plStreamReader& inout_stream)
{
  inout_stream.ReadVersion(1);

  plUInt32 uiEntries = 0;
  inout_stream >> uiEntries;

  for (plUInt32 e = 0; e < uiEntries; ++e)
  {
    plHashedString name;
    inout_stream >> name;

    plBitflags<plBlackboardEntryFlags> flags;
    inout_stream >> flags;

    plVariant value;
    inout_stream >> value;

    RegisterEntry(name, value, flags);
  }

  return PLASMA_SUCCESS;
}

// static
plBlackboard* plBlackboard::Reflection_GetOrCreateGlobal(const plHashedString& sName)
{
  return GetOrCreateGlobal(sName).Borrow();
}

// static
plBlackboard* plBlackboard::Reflection_FindGlobal(plTempHashedString sName)
{
  return FindGlobal(sName);
}

void plBlackboard::Reflection_RegisterEntry(const plHashedString& sName, const plVariant& initialValue, bool bSave, bool bOnChangeEvent)
{
  plBitflags<plBlackboardEntryFlags> flags;
  flags.AddOrRemove(plBlackboardEntryFlags::Save, bSave);
  flags.AddOrRemove(plBlackboardEntryFlags::OnChangeEvent, bOnChangeEvent);

  RegisterEntry(sName, initialValue, flags);
}

bool plBlackboard::Reflection_SetEntryValue(plTempHashedString sName, const plVariant& value)
{
  return SetEntryValue(sName, value).Succeeded();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plBlackboardCondition, plNoBase, 1, plRTTIDefaultAllocator<plBlackboardCondition>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("EntryName", GetEntryName, SetEntryName),
    PLASMA_ENUM_MEMBER_PROPERTY("Operator", plComparisonOperator, m_Operator),
    PLASMA_MEMBER_PROPERTY("ComparisonValue", m_fComparisonValue),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool plBlackboardCondition::IsConditionMet(const plBlackboard& blackboard) const
{
  auto pEntry = blackboard.GetEntry(m_sEntryName);
  if (pEntry != nullptr && pEntry->m_Value.IsNumber())
  {
    double fEntryValue = pEntry->m_Value.ConvertTo<double>();
    return plComparisonOperator::Compare(m_Operator, fEntryValue, m_fComparisonValue);
  }

  return false;
}

constexpr plTypeVersion s_BlackboardConditionVersion = 1;

plResult plBlackboardCondition::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_BlackboardConditionVersion);

  inout_stream << m_sEntryName;
  inout_stream << m_Operator;
  inout_stream << m_fComparisonValue;
  return PLASMA_SUCCESS;
}

plResult plBlackboardCondition::Deserialize(plStreamReader& inout_stream)
{
  const plTypeVersion uiVersion = inout_stream.ReadVersion(s_BlackboardConditionVersion);
  PLASMA_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sEntryName;
  inout_stream >> m_Operator;
  inout_stream >> m_fComparisonValue;
  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(Core, Core_Utils_Implementation_Blackboard);
