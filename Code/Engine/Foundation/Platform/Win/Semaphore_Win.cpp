#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Basics/Platform/Win/MinWindows.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Threading/Semaphore.h>

plSemaphore::plSemaphore()
{
  m_hSemaphore = nullptr;
}

plSemaphore::~plSemaphore()
{
  if (m_hSemaphore != nullptr)
  {
    CloseHandle(m_hSemaphore);
    m_hSemaphore = nullptr;
  }
}

plResult plSemaphore::Create(plUInt32 uiInitialTokenCount, plStringView sSharedName /*= nullptr*/)
{
  PL_ASSERT_DEV(m_hSemaphore == nullptr, "Semaphore can't be recreated.");

  LPSECURITY_ATTRIBUTES secAttr = nullptr; // default
  const DWORD flags = 0;                   // reserved but unused
  const DWORD access = STANDARD_RIGHTS_ALL | SEMAPHORE_MODIFY_STATE /* needed for ReleaseSemaphore */;

  if (sSharedName.IsEmpty())
  {
    // create an unnamed semaphore

    m_hSemaphore = CreateSemaphoreExW(secAttr, uiInitialTokenCount, plMath::MaxValue<plInt32>(), nullptr, flags, access);
  }
  else
  {
    // create a named semaphore in the 'Local' namespace
    // these are visible session wide, ie. all processes by the same user account can see these, but not across users

    const plStringBuilder semaphoreName("Local\\", sSharedName);

    m_hSemaphore = CreateSemaphoreExW(secAttr, uiInitialTokenCount, plMath::MaxValue<plInt32>(), plStringWChar(semaphoreName).GetData(), flags, access);
  }

  if (m_hSemaphore == nullptr)
  {
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plSemaphore::Open(plStringView sSharedName)
{
  PL_ASSERT_DEV(m_hSemaphore == nullptr, "Semaphore can't be recreated.");

  const DWORD access = SYNCHRONIZE /* needed for WaitForSingleObject */ | SEMAPHORE_MODIFY_STATE /* needed for ReleaseSemaphore */;
  const BOOL inheriteHandle = FALSE;

  PL_ASSERT_DEV(!sSharedName.IsEmpty(), "Name of semaphore to open mustn't be empty.");

  const plStringBuilder semaphoreName("Local\\", sSharedName);

  m_hSemaphore = OpenSemaphoreW(access, inheriteHandle, plStringWChar(semaphoreName).GetData());

  if (m_hSemaphore == nullptr)
  {
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

void plSemaphore::AcquireToken()
{
  PL_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");
  PL_VERIFY(WaitForSingleObject(m_hSemaphore, INFINITE) == WAIT_OBJECT_0, "Semaphore token acquisition failed.");
}

void plSemaphore::ReturnToken()
{
  PL_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");
  PL_VERIFY(ReleaseSemaphore(m_hSemaphore, 1, nullptr) != 0, "Returning a semaphore token failed, most likely due to a AcquireToken() / ReturnToken() mismatch.");
}

plResult plSemaphore::TryAcquireToken()
{
  PL_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");

  const plUInt32 res = WaitForSingleObject(m_hSemaphore, 0 /* timeout of zero milliseconds */);

  if (res == WAIT_OBJECT_0)
  {
    return PL_SUCCESS;
  }

  PL_ASSERT_DEV(res == WAIT_OBJECT_0 || res == WAIT_TIMEOUT, "Semaphore TryAcquireToken (WaitForSingleObject) failed with error code {}.", res);

  return PL_FAILURE;
}

#endif


