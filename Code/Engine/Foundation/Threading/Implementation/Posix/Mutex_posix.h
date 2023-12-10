
PLASMA_ALWAYS_INLINE plMutex::plMutex()
{
  pthread_mutexattr_t mutexAttributes;
  pthread_mutexattr_init(&mutexAttributes);
  pthread_mutexattr_settype(&mutexAttributes, PTHREAD_MUTEX_RECURSIVE);

  pthread_mutex_init(&m_hHandle, &mutexAttributes);

  pthread_mutexattr_destroy(&mutexAttributes);
}

PLASMA_ALWAYS_INLINE plMutex::~plMutex()
{
  pthread_mutex_destroy(&m_hHandle);
}

PLASMA_ALWAYS_INLINE void plMutex::Lock()
{
  pthread_mutex_lock(&m_hHandle);
  ++m_iLockCount;
}

PLASMA_ALWAYS_INLINE plResult plMutex::TryLock()
{
  if (pthread_mutex_trylock(&m_hHandle) == 0)
  {
    ++m_iLockCount;
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}
PLASMA_ALWAYS_INLINE void plMutex::Unlock()
{
  --m_iLockCount;
  pthread_mutex_unlock(&m_hHandle);
}
