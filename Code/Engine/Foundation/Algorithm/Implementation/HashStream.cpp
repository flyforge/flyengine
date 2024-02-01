#include <Foundation/FoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>

PL_WARNING_PUSH()
PL_WARNING_DISABLE_CLANG("-Wunused-function")

#define XXH_INLINE_ALL
#include <Foundation/ThirdParty/xxHash/xxhash.h>

PL_WARNING_POP()

plHashStreamWriter32::plHashStreamWriter32(plUInt32 uiSeed)
{
  m_pState = XXH32_createState();
  PL_VERIFY(XXH_OK == XXH32_reset((XXH32_state_t*)m_pState, uiSeed), "");
}

plHashStreamWriter32::~plHashStreamWriter32()
{
  XXH32_freeState((XXH32_state_t*)m_pState);
}

plResult plHashStreamWriter32::WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return PL_FAILURE;

  if (XXH_OK == XXH32_update((XXH32_state_t*)m_pState, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return PL_SUCCESS;

  return PL_FAILURE;
}

plUInt32 plHashStreamWriter32::GetHashValue() const
{
  return XXH32_digest((XXH32_state_t*)m_pState);
}


plHashStreamWriter64::plHashStreamWriter64(plUInt64 uiSeed)
{
  m_pState = XXH64_createState();
  PL_VERIFY(XXH_OK == XXH64_reset((XXH64_state_t*)m_pState, uiSeed), "");
}

plHashStreamWriter64::~plHashStreamWriter64()
{
  XXH64_freeState((XXH64_state_t*)m_pState);
}

plResult plHashStreamWriter64::WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return PL_FAILURE;

  if (XXH_OK == XXH64_update((XXH64_state_t*)m_pState, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return PL_SUCCESS;

  return PL_FAILURE;
}

plUInt64 plHashStreamWriter64::GetHashValue() const
{
  return XXH64_digest((XXH64_state_t*)m_pState);
}


