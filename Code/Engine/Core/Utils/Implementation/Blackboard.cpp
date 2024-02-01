#include <Core/CorePCH.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plBlackboardEntryFlags, 1)
  PL_BITFLAGS_CONSTANTS(plBlackboardEntryFlags::Save, plBlackboardEntryFlags::OnChangeEvent,
    plBlackboardEntryFlags::UserFlag0, plBlackboardEntryFlags::UserFlag1, plBlackboardEntryFlags::UserFlag2, plBlackboardEntryFlags::UserFlag3, plBlackboardEntryFlags::UserFlag4, plBlackboardEntryFlags::UserFlag5, plBlackboardEntryFlags::UserFlag6, plBlackboardEntryFlags::UserFlag7)
PL_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plBlackboard, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_GetOrCreateGlobal, In, "Name")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardNamesEnum"))),
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_FindGlobal, In, "Name")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardNamesEnum"))),

    PL_SCRIPT_FUNCTION_PROPERTY(GetName),
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_SetEntryValue, In, "Name", In, "Value")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    PL_SCRIPT_FUNCTION_PROPERTY(GetEntryValue, In, "Name", In, "Fallback")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    PL_SCRIPT_FUNCTION_PROPERTY(IncrementEntryValue, In, "Name")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    PL_SCRIPT_FUNCTION_PROPERTY(DecrementEntryValue, In, "Name")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    PL_SCRIPT_FUNCTION_PROPERTY(GetBlackboardChangeCounter),
    PL_SCRIPT_FUNCTION_PROPERTY(GetBlackboardEntryChangeCounter)
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_SUBSYSTEM_DECLARATION(Core, Blackboard)

  ON_CORESYSTEMS_SHUTDOWN
  {
    PL_LOCK(plBlackboard::s_GlobalBlackboardsMutex);
    plBlackboard::s_GlobalBlackboards.Clear();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

// static
plMutex plBlackboard::s_GlobalBlackboardsMutex;
plHashTable<plHashedString, plSharedPtr<plBlackboard>> plBlackboard::s_GlobalBlackboards;

// static
plSharedPtr<plBlackboard> plBlackboard::Create(plAllocator* pAllocator /*= plFoundation::GetDefaultAllocator()*/)
{
  return PL_NEW(pAllocator, plBlackboard, false);
}

// static
plSharedPtr<plBlackboard> plBlackboard::GetOrCreateGlobal(const plHashedString& sBlackboardName, plAllocator* pAllocator /*= plFoundation::GetDefaultAllocator()*/)
{
  PL_LOCK(s_GlobalBlackboardsMutex);

  auto it = s_GlobalBlackboards.Find(sBlackboardName);

  if (it.IsValid())
  {
    return it.Value();
  }

  plSharedPtr<plBlackboard> pShrd = PL_NEW(pAllocator, plBlackboard, true);
  pShrd->m_sName = sBlackboardName;
  s_GlobalBlackboards.Insert(sBlackboardName, pShrd);

  return pShrd;
}

// static
plSharedPtr<plBlackboard> plBlackboard::FindGlobal(const plTempHashedString& sBlackboardName)
{
  PL_LOCK(s_GlobalBlackboardsMutex);

  plSharedPtr<plBlackboard> pBlackboard;
  s_GlobalBlackboards.TryGetValue(sBlackboardName, pBlackboard);
  return pBlackboard;
}

plBlackboard::plBlackboard(bool bIsGlobal)
{
  m_bIsGlobal = bIsGlobal;
}

plBlackboard::~plBlackboard() = default;

void plBlackboard::SetName(plStringView sName)
{
  PL_LOCK(s_GlobalBlackboardsMutex);
  m_sName.Assign(sName);
}

void plBlackboard::RemoveEntry(const plHashedString& sName)
{
  if (m_Entries.Remove(sName))
  {
    ++m_uiBlackboardChangeCounter;
  }
}

void plBlackboard::RemoveAllEntries()
{
  if (m_Entries.IsEmpty() == false)
  {
    ++m_uiBlackboardChangeCounter;
  }

  m_Entries.Clear();
}

void plBlackboard::ImplSetEntryValue(const plHashedString& sName, Entry& entry, const plVariant& value)
{
  if (entry.m_Value != value)
  {
    ++m_uiBlackboardEntryChangeCounter;
    ++entry.m_uiChangeCounter;

    if (entry.m_Flags.IsSet(plBlackboardEntryFlags::OnChangeEvent))
    {
      EntryEvent e;
      e.m_sName = sName;
      e.m_OldValue = entry.m_Value;
      e.m_pEntry = &entry;

      entry.m_Value = value;

      m_EntryEvents.Broadcast(e, 1); // limited recursion is allowed
    }
    else
    {
      entry.m_Value = value;
    }
  }
}

void plBlackboard::SetEntryValue(plStringView sName, const plVariant& value)
{
  const plTempHashedString sNameTH(sName);

  auto itEntry = m_Entries.Find(sNameTH);

  if (!itEntry.IsValid())
  {
    plHashedString sNameHS;
    sNameHS.Assign(sName);
    m_Entries[sNameHS].m_Value = value;

    ++m_uiBlackboardChangeCounter;
  }
  else
  {
    ImplSetEntryValue(itEntry.Key(), itEntry.Value(), value);
  }
}

void plBlackboard::SetEntryValue(const plHashedString& sName, const plVariant& value)
{
  auto itEntry = m_Entries.Find(sName);

  if (!itEntry.IsValid())
  {
    m_Entries[sName].m_Value = value;

    ++m_uiBlackboardChangeCounter;
  }
  else
  {
    ImplSetEntryValue(itEntry.Key(), itEntry.Value(), value);
  }
}

void plBlackboard::Reflection_SetEntryValue(plStringView sName, const plVariant& value)
{
  SetEntryValue(sName, value);
}

bool plBlackboard::HasEntry(const plTempHashedString& sName) const
{
  return m_Entries.Find(sName).IsValid();
}

plResult plBlackboard::SetEntryFlags(const plTempHashedString& sName, plBitflags<plBlackboardEntryFlags> flags)
{
  auto itEntry = m_Entries.Find(sName);
  if (!itEntry.IsValid())
    return PL_FAILURE;

  itEntry.Value().m_Flags = flags;
  return PL_SUCCESS;
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

  return PL_SUCCESS;
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

    SetEntryValue(name, value);
    SetEntryFlags(name, flags).AssertSuccess();
  }

  return PL_SUCCESS;
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

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plBlackboardCondition, plNoBase, 1, plRTTIDefaultAllocator<plBlackboardCondition>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("EntryName", m_sEntryName)->AddAttributes(new plDynamicStringEnumAttribute("BlackboardKeysEnum")),
    PL_ENUM_MEMBER_PROPERTY("Operator", plComparisonOperator, m_Operator),
    PL_MEMBER_PROPERTY("ComparisonValue", m_fComparisonValue),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
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
  return PL_SUCCESS;
}

plResult plBlackboardCondition::Deserialize(plStreamReader& inout_stream)
{
  const plTypeVersion uiVersion = inout_stream.ReadVersion(s_BlackboardConditionVersion);
  PL_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sEntryName;
  inout_stream >> m_Operator;
  inout_stream >> m_fComparisonValue;
  return PL_SUCCESS;
}

PL_STATICLINK_FILE(Core, Core_Utils_Implementation_Blackboard);
