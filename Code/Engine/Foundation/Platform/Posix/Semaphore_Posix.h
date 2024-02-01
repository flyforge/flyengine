#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Threading/Semaphore.h>

#include <Foundation/Strings/StringBuilder.h>

#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>

PL_WARNING_PUSH()
// On OSX sem_destroy and sem_init are deprecated
PL_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")

plSemaphore::plSemaphore() = default;

plSemaphore::~plSemaphore()
{
  if (m_hSemaphore.m_pNamedOrUnnamed != nullptr)
  {
    if (m_hSemaphore.m_pNamed != nullptr)
    {
      sem_close(m_hSemaphore.m_pNamed);
    }
    else
    {
      sem_destroy(&m_hSemaphore.m_Unnamed);
    }
  }
}

plResult plSemaphore::Create(plUInt32 uiInitialTokenCount, plStringView sSharedName /*= nullptr*/)
{
  if (sSharedName.IsEmpty())
  {
    // create an unnamed semaphore

    if (sem_init(&m_hSemaphore.m_Unnamed, 0, uiInitialTokenCount) != 0)
    {
      return PL_FAILURE;
    }

    m_hSemaphore.m_pNamedOrUnnamed = &m_hSemaphore.m_Unnamed;
  }
  else
  {
    // create a named semaphore

    // documentation is unclear about access rights, just throwing everything at it for good measure
    plStringBuilder tmp;
    m_hSemaphore.m_pNamed = sem_open(sSharedName.GetData(tmp), O_CREAT | O_EXCL, S_IRWXU | S_IRWXO | S_IRWXG, uiInitialTokenCount);

    if (m_hSemaphore.m_pNamed == nullptr)
    {
      return PL_FAILURE;
    }

    m_hSemaphore.m_pNamedOrUnnamed = m_hSemaphore.m_pNamed;
  }

  return PL_SUCCESS;
}

plResult plSemaphore::Open(plStringView sSharedName)
{
  PL_ASSERT_DEV(!sSharedName.IsEmpty(), "Name of semaphore to open mustn't be empty.");

  // open a named semaphore

  plStringBuilder tmp;
  m_hSemaphore.m_pNamed = sem_open(sSharedName.GetData(tmp), 0);

  if (m_hSemaphore.m_pNamed == nullptr)
  {
    return PL_FAILURE;
  }

  m_hSemaphore.m_pNamedOrUnnamed = m_hSemaphore.m_pNamed;

  return PL_SUCCESS;
}

void plSemaphore::AcquireToken()
{
  PL_VERIFY(sem_wait(m_hSemaphore.m_pNamedOrUnnamed) == 0, "Semaphore token acquisition failed.");
}

void plSemaphore::ReturnToken()
{
  PL_VERIFY(sem_post(m_hSemaphore.m_pNamedOrUnnamed) == 0, "Returning a semaphore token failed, most likely due to a AcquireToken() / ReturnToken() mismatch.");
}

plResult plSemaphore::TryAcquireToken()
{
  // documentation is unclear whether one needs to check errno, or not
  // assuming that this will return 0 only when trywait got a token

  if (sem_trywait(m_hSemaphore.m_pNamedOrUnnamed) == 0)
    return PL_SUCCESS;

  return PL_FAILURE;
}

PL_WARNING_POP()
