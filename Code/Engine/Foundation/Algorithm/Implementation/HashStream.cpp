#include <Foundation/FoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>

#define XXH_INLINE_ALL
#include <Foundation/ThirdParty/xxHash/xxhash.h>

plHashStreamWriter32::plHashStreamWriter32(plUInt32 uiSeed)
{
  m_pState = XXH32_createState();
  PLASMA_VERIFY(XXH_OK == XXH32_reset((XXH32_state_t*)m_pState, uiSeed), "");
}

plHashStreamWriter32::~plHashStreamWriter32()
{
  XXH32_freeState((XXH32_state_t*)m_pState);
}

plResult plHashStreamWriter32::WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return PLASMA_FAILURE;

  if (XXH_OK == XXH32_update((XXH32_state_t*)m_pState, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}

plUInt32 plHashStreamWriter32::GetHashValue() const
{
  return XXH32_digest((XXH32_state_t*)m_pState);
}


plHashStreamWriter64::plHashStreamWriter64(plUInt64 uiSeed)
{
  m_pState = XXH64_createState();
  PLASMA_VERIFY(XXH_OK == XXH64_reset((XXH64_state_t*)m_pState, uiSeed), "");
}

plHashStreamWriter64::~plHashStreamWriter64()
{
  XXH64_freeState((XXH64_state_t*)m_pState);
}

plResult plHashStreamWriter64::WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return PLASMA_FAILURE;

  if (XXH_OK == XXH64_update((XXH64_state_t*)m_pState, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}

plUInt64 plHashStreamWriter64::GetHashValue() const
{
  return XXH64_digest((XXH64_state_t*)m_pState);
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Algorithm_Implementation_HashStream);
