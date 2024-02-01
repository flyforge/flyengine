#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

struct HashedStringData
{
  plMutex m_Mutex;
  plHashedString::StringStorage m_Storage;
  plHashedString::HashedType m_Empty;
};

static HashedStringData* s_pHSData;

PL_MSVC_ANALYSIS_WARNING_PUSH
PL_MSVC_ANALYSIS_WARNING_DISABLE(6011) // Disable warning for null pointer dereference as InitHashedString() will ensure that s_pHSData is set

// static
plHashedString::HashedType plHashedString::AddHashedString(plStringView sString, plUInt64 uiHash)
{
  if (s_pHSData == nullptr)
    InitHashedString();

  PL_LOCK(s_pHSData->m_Mutex);

  // try to find the existing string
  bool bExisted = false;
  auto ret = s_pHSData->m_Storage.FindOrAdd(uiHash, &bExisted);

  // if it already exists, just increase the refcount
  if (bExisted)
  {
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
    if (ret.Value().m_sString != sString)
    {
      // TODO: I think this should be a more serious issue
      plLog::Error("Hash collision encountered: Strings \"{}\" and \"{}\" both hash to {}.", plArgSensitive(ret.Value().m_sString), plArgSensitive(sString), uiHash);
    }
#endif

#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
    ret.Value().m_iRefCount.Increment();
#endif
  }
  else
  {
    plHashedString::HashedData& d = ret.Value();
#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
    d.m_iRefCount = 1;
#endif
    d.m_sString = sString;
  }

  return ret;
}

PL_MSVC_ANALYSIS_WARNING_POP

// static
void plHashedString::InitHashedString()
{
  if (s_pHSData != nullptr)
    return;

  alignas(PL_ALIGNMENT_OF(HashedStringData)) static plUInt8 HashedStringDataBuffer[sizeof(HashedStringData)];
  s_pHSData = new (HashedStringDataBuffer) HashedStringData();

  // makes sure the empty string exists for the default constructor to use
  s_pHSData->m_Empty = AddHashedString("", plHashingUtils::StringHash(""));

#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
  // this one should never get deleted, so make sure its refcount is 2
  s_pHSData->m_Empty.Value().m_iRefCount.Increment();
#endif
}

#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
plUInt32 plHashedString::ClearUnusedStrings()
{
  PL_LOCK(s_pHSData->m_Mutex);

  plUInt32 uiDeleted = 0;

  for (auto it = s_pHSData->m_Storage.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_iRefCount == 0)
    {
      it = s_pHSData->m_Storage.Remove(it);
      ++uiDeleted;
    }
    else
      ++it;
  }

  return uiDeleted;
}
#endif

PL_MSVC_ANALYSIS_WARNING_PUSH
PL_MSVC_ANALYSIS_WARNING_DISABLE(6011) // Disable warning for null pointer dereference as InitHashedString() will ensure that s_pHSData is set

plHashedString::plHashedString()
{
  PL_CHECK_AT_COMPILETIME_MSG(sizeof(m_Data) == sizeof(void*), "The hashed string data should only be as large as one pointer.");
  PL_CHECK_AT_COMPILETIME_MSG(sizeof(*this) == sizeof(void*), "The hashed string data should only be as large as one pointer.");

  // only insert the empty string once, after that, we can just use it without the need for the mutex
  if (s_pHSData == nullptr)
    InitHashedString();

  m_Data = s_pHSData->m_Empty;
#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
  m_Data.Value().m_iRefCount.Increment();
#endif
}

PL_MSVC_ANALYSIS_WARNING_POP

bool plHashedString::IsEmpty() const
{
  return m_Data == s_pHSData->m_Empty;
}

void plHashedString::Clear()
{
#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
  if (m_Data != s_pHSData->m_Empty)
  {
    HashedType tmp = m_Data;

    m_Data = s_pHSData->m_Empty;
    m_Data.Value().m_iRefCount.Increment();

    tmp.Value().m_iRefCount.Decrement();
  }
#else
  m_Data = s_pHSData->m_Empty;
#endif
}


