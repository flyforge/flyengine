#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Tag.h>

static plTagRegistry s_GlobalRegistry;

plTagRegistry::plTagRegistry() = default;

plTagRegistry& plTagRegistry::GetGlobalRegistry()
{
  return s_GlobalRegistry;
}

const plTag& plTagRegistry::RegisterTag(plStringView sTagString)
{
  plHashedString TagString;
  TagString.Assign(sTagString);

  return RegisterTag(TagString);
}

const plTag& plTagRegistry::RegisterTag(const plHashedString& sTagString)
{
  PL_LOCK(m_TagRegistryMutex);

  // Early out if the tag is already registered
  const plTag* pResult = GetTagByName(sTagString);

  if (pResult != nullptr)
    return *pResult;

  const plUInt32 uiNextTagIndex = m_TagsByIndex.GetCount();

  // Build temp tag
  plTag TempTag;
  TempTag.m_uiBlockIndex = uiNextTagIndex / (sizeof(plTagSetBlockStorage) * 8);
  TempTag.m_uiBitIndex = uiNextTagIndex - (TempTag.m_uiBlockIndex * sizeof(plTagSetBlockStorage) * 8);
  TempTag.m_sTagString = sTagString;

  // Store the tag
  auto it = m_RegisteredTags.Insert(sTagString, TempTag);

  m_TagsByIndex.PushBack(&it.Value());

  plLog::Debug("Registered Tag '{0}'", sTagString);
  return *m_TagsByIndex.PeekBack();
}

const plTag* plTagRegistry::GetTagByName(const plTempHashedString& sTagString) const
{
  PL_LOCK(m_TagRegistryMutex);

  auto It = m_RegisteredTags.Find(sTagString);
  if (It.IsValid())
  {
    return &It.Value();
  }

  return nullptr;
}

const plTag* plTagRegistry::GetTagByMurmurHash(plUInt32 uiMurmurHash) const
{
  PL_LOCK(m_TagRegistryMutex);

  for (plTag* pTag : m_TagsByIndex)
  {
    if (plHashingUtils::MurmurHash32String(pTag->GetTagString()) == uiMurmurHash)
    {
      return pTag;
    }
  }

  return nullptr;
}

const plTag* plTagRegistry::GetTagByIndex(plUInt32 uiIndex) const
{
  PL_LOCK(m_TagRegistryMutex);
  return m_TagsByIndex[uiIndex];
}

plUInt32 plTagRegistry::GetNumTags() const
{
  PL_LOCK(m_TagRegistryMutex);
  return m_TagsByIndex.GetCount();
}

plResult plTagRegistry::Load(plStreamReader& inout_stream)
{
  PL_LOCK(m_TagRegistryMutex);

  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion != 1)
  {
    plLog::Error("Invalid plTagRegistry version {0}", uiVersion);
    return PL_FAILURE;
  }

  plUInt32 uiNumTags = 0;
  inout_stream >> uiNumTags;

  if (uiNumTags > 16 * 1024)
  {
    plLog::Error("plTagRegistry::Load, unreasonable amount of tags {0}, cancelling load.", uiNumTags);
    return PL_FAILURE;
  }

  plStringBuilder temp;
  for (plUInt32 i = 0; i < uiNumTags; ++i)
  {
    inout_stream >> temp;

    RegisterTag(temp);
  }

  return PL_SUCCESS;
}


